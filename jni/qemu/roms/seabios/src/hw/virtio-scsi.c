// Virtio SCSI boot support.
//
// Copyright (C) 2012 Red Hat Inc.
//
// Authors:
//  Paolo Bonzini <pbonzini@redhat.com>
//
// This file may be distributed under the terms of the GNU LGPLv3 license.

#include "biosvar.h" // GET_GLOBALFLAT
#include "block.h" // struct drive_s
#include "blockcmd.h" // scsi_drive_setup
#include "config.h" // CONFIG_*
#include "malloc.h" // free
#include "output.h" // dprintf
#include "pci.h" // foreachpci
#include "pci_ids.h" // PCI_DEVICE_ID_VIRTIO_BLK
#include "pci_regs.h" // PCI_VENDOR_ID
#include "std/disk.h" // DISK_RET_SUCCESS
#include "string.h" // memset
#include "util.h" // usleep
#include "virtio-pci.h"
#include "virtio-ring.h"
#include "virtio-scsi.h"

struct virtio_lun_s {
    struct drive_s drive;
    struct pci_device *pci;
    struct vring_virtqueue *vq;
    struct vp_device *vp;
    u16 target;
    u16 lun;
};

int
virtio_scsi_process_op(struct disk_op_s *op)
{
    if (! CONFIG_VIRTIO_SCSI)
        return 0;
    struct virtio_lun_s *vlun =
        container_of(op->drive_gf, struct virtio_lun_s, drive);
    struct vp_device *vp = vlun->vp;
    struct vring_virtqueue *vq = vlun->vq;
    struct virtio_scsi_req_cmd req;
    struct virtio_scsi_resp_cmd resp;
    struct vring_list sg[3];

    memset(&req, 0, sizeof(req));
    int blocksize = scsi_fill_cmd(op, req.cdb, 16);
    if (blocksize < 0)
        return default_process_op(op);
    req.lun[0] = 1;
    req.lun[1] = vlun->target;
    req.lun[2] = (vlun->lun >> 8) | 0x40;
    req.lun[3] = (vlun->lun & 0xff);

    u32 len = op->count * blocksize;
    int datain = scsi_is_read(op);
    int in_num = (datain ? 2 : 1);
    int out_num = (len ? 3 : 2) - in_num;

    sg[0].addr   = (void*)(&req);
    sg[0].length = sizeof(req);

    sg[out_num].addr   = (void*)(&resp);
    sg[out_num].length = sizeof(resp);

    if (len) {
        int data_idx = (datain ? 2 : 1);
        sg[data_idx].addr   = op->buf_fl;
        sg[data_idx].length = len;
    }

    /* Add to virtqueue and kick host */
    vring_add_buf(vq, sg, out_num, in_num, 0, 0);
    vring_kick(vp, vq, 1);

    /* Wait for reply */
    while (!vring_more_used(vq))
        usleep(5);

    /* Reclaim virtqueue element */
    vring_get_buf(vq, NULL);

    /* Clear interrupt status register.  Avoid leaving interrupts stuck if
     * VRING_AVAIL_F_NO_INTERRUPT was ignored and interrupts were raised.
     */
    vp_get_isr(vp);

    if (resp.response == VIRTIO_SCSI_S_OK && resp.status == 0) {
        return DISK_RET_SUCCESS;
    }
    return DISK_RET_EBADTRACK;
}

static int
virtio_scsi_add_lun(struct pci_device *pci, struct vp_device *vp,
                    struct vring_virtqueue *vq, u16 target, u16 lun)
{
    struct virtio_lun_s *vlun = malloc_fseg(sizeof(*vlun));
    if (!vlun) {
        warn_noalloc();
        return -1;
    }
    memset(vlun, 0, sizeof(*vlun));
    vlun->drive.type = DTYPE_VIRTIO_SCSI;
    vlun->drive.cntl_id = pci->bdf;
    vlun->pci = pci;
    vlun->vp = vp;
    vlun->vq = vq;
    vlun->target = target;
    vlun->lun = lun;

    int prio = bootprio_find_scsi_device(pci, target, lun);
    int ret = scsi_drive_setup(&vlun->drive, "virtio-scsi", prio);
    if (ret)
        goto fail;
    return 0;

fail:
    free(vlun);
    return -1;
}

static int
virtio_scsi_scan_target(struct pci_device *pci, struct vp_device *vp,
                        struct vring_virtqueue *vq, u16 target)
{
    /* TODO: send REPORT LUNS.  For now, only LUN 0 is recognized.  */
    int ret = virtio_scsi_add_lun(pci, vp, vq, target, 0);
    return ret < 0 ? 0 : 1;
}

static void
init_virtio_scsi(struct pci_device *pci)
{
    u16 bdf = pci->bdf;
    dprintf(1, "found virtio-scsi at %x:%x\n", pci_bdf_to_bus(bdf),
            pci_bdf_to_dev(bdf));
    struct vring_virtqueue *vq = NULL;
    struct vp_device *vp = malloc_high(sizeof(*vp));
    if (!vp) {
        warn_noalloc();
        return;
    }
    vp_init_simple(vp, pci);
    u8 status = VIRTIO_CONFIG_S_ACKNOWLEDGE | VIRTIO_CONFIG_S_DRIVER;

    if (vp->use_modern) {
        u64 features = vp_get_features(vp);
        u64 version1 = 1ull << VIRTIO_F_VERSION_1;
        if (!(features & version1)) {
            dprintf(1, "modern device without virtio_1 feature bit: %x:%x\n",
                    pci_bdf_to_bus(bdf), pci_bdf_to_dev(bdf));
            goto fail;
        }

        vp_set_features(vp, version1);
        status |= VIRTIO_CONFIG_S_FEATURES_OK;
        vp_set_status(vp, status);
        if (!(vp_get_status(vp) & VIRTIO_CONFIG_S_FEATURES_OK)) {
            dprintf(1, "device didn't accept features: %x:%x\n",
                    pci_bdf_to_bus(bdf), pci_bdf_to_dev(bdf));
            goto fail;
        }
    }

    if (vp_find_vq(vp, 2, &vq) < 0 ) {
        dprintf(1, "fail to find vq for virtio-scsi %x:%x\n",
                pci_bdf_to_bus(bdf), pci_bdf_to_dev(bdf));
        goto fail;
    }

    status |= VIRTIO_CONFIG_S_DRIVER_OK;
    vp_set_status(vp, status);

    int i, tot;
    for (tot = 0, i = 0; i < 256; i++)
        tot += virtio_scsi_scan_target(pci, vp, vq, i);

    if (!tot)
        goto fail;

    return;

fail:
    vp_reset(vp);
    free(vp);
    free(vq);
}

void
virtio_scsi_setup(void)
{
    ASSERT32FLAT();
    if (! CONFIG_VIRTIO_SCSI)
        return;

    dprintf(3, "init virtio-scsi\n");

    struct pci_device *pci;
    foreachpci(pci) {
        if (pci->vendor != PCI_VENDOR_ID_REDHAT_QUMRANET ||
            (pci->device != PCI_DEVICE_ID_VIRTIO_SCSI_09 &&
             pci->device != PCI_DEVICE_ID_VIRTIO_SCSI_10))
            continue;
        init_virtio_scsi(pci);
    }
}
