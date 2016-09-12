// Virtio block boot support.
//
// Copyright (C) 2010 Red Hat Inc.
//
// Authors:
//  Gleb Natapov <gnatapov@redhat.com>
//
// This file may be distributed under the terms of the GNU LGPLv3 license.

#include "biosvar.h" // GET_GLOBALFLAT
#include "config.h" // CONFIG_*
#include "block.h" // struct drive_s
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
#include "virtio-blk.h"

struct virtiodrive_s {
    struct drive_s drive;
    struct vring_virtqueue *vq;
    struct vp_device vp;
};

static int
virtio_blk_op(struct disk_op_s *op, int write)
{
    struct virtiodrive_s *vdrive_gf =
        container_of(op->drive_gf, struct virtiodrive_s, drive);
    struct vring_virtqueue *vq = vdrive_gf->vq;
    struct virtio_blk_outhdr hdr = {
        .type = write ? VIRTIO_BLK_T_OUT : VIRTIO_BLK_T_IN,
        .ioprio = 0,
        .sector = op->lba,
    };
    u8 status = VIRTIO_BLK_S_UNSUPP;
    struct vring_list sg[] = {
        {
            .addr       = (void*)(&hdr),
            .length     = sizeof(hdr),
        },
        {
            .addr       = op->buf_fl,
            .length     = vdrive_gf->drive.blksize * op->count,
        },
        {
            .addr       = (void*)(&status),
            .length     = sizeof(status),
        },
    };

    /* Add to virtqueue and kick host */
    if (write)
        vring_add_buf(vq, sg, 2, 1, 0, 0);
    else
        vring_add_buf(vq, sg, 1, 2, 0, 0);
    vring_kick(&vdrive_gf->vp, vq, 1);

    /* Wait for reply */
    while (!vring_more_used(vq))
        usleep(5);

    /* Reclaim virtqueue element */
    vring_get_buf(vq, NULL);

    /* Clear interrupt status register.  Avoid leaving interrupts stuck if
     * VRING_AVAIL_F_NO_INTERRUPT was ignored and interrupts were raised.
     */
    vp_get_isr(&vdrive_gf->vp);

    return status == VIRTIO_BLK_S_OK ? DISK_RET_SUCCESS : DISK_RET_EBADTRACK;
}

int
virtio_blk_process_op(struct disk_op_s *op)
{
    if (! CONFIG_VIRTIO_BLK)
        return 0;
    switch (op->command) {
    case CMD_READ:
        return virtio_blk_op(op, 0);
    case CMD_WRITE:
        return virtio_blk_op(op, 1);
    default:
        return default_process_op(op);
    }
}

static void
init_virtio_blk(struct pci_device *pci)
{
    u16 bdf = pci->bdf;
    u8 status = VIRTIO_CONFIG_S_ACKNOWLEDGE | VIRTIO_CONFIG_S_DRIVER;
    dprintf(1, "found virtio-blk at %x:%x\n", pci_bdf_to_bus(bdf),
            pci_bdf_to_dev(bdf));
    struct virtiodrive_s *vdrive = malloc_fseg(sizeof(*vdrive));
    if (!vdrive) {
        warn_noalloc();
        return;
    }
    memset(vdrive, 0, sizeof(*vdrive));
    vdrive->drive.type = DTYPE_VIRTIO_BLK;
    vdrive->drive.cntl_id = bdf;

    vp_init_simple(&vdrive->vp, pci);
    if (vp_find_vq(&vdrive->vp, 0, &vdrive->vq) < 0 ) {
        dprintf(1, "fail to find vq for virtio-blk %x:%x\n",
                pci_bdf_to_bus(bdf), pci_bdf_to_dev(bdf));
        goto fail;
    }

    if (vdrive->vp.use_modern) {
        struct vp_device *vp = &vdrive->vp;
        u64 features = vp_get_features(vp);
        u64 version1 = 1ull << VIRTIO_F_VERSION_1;
        u64 blk_size = 1ull << VIRTIO_BLK_F_BLK_SIZE;
        if (!(features & version1)) {
            dprintf(1, "modern device without virtio_1 feature bit: %x:%x\n",
                    pci_bdf_to_bus(bdf), pci_bdf_to_dev(bdf));
            goto fail;
        }

        features = features & (version1 | blk_size);
        vp_set_features(vp, features);
        status |= VIRTIO_CONFIG_S_FEATURES_OK;
        vp_set_status(vp, status);
        if (!(vp_get_status(vp) & VIRTIO_CONFIG_S_FEATURES_OK)) {
            dprintf(1, "device didn't accept features: %x:%x\n",
                    pci_bdf_to_bus(bdf), pci_bdf_to_dev(bdf));
            goto fail;
        }

        vdrive->drive.sectors =
            vp_read(&vp->device, struct virtio_blk_config, capacity);
        if (features & blk_size) {
            vdrive->drive.blksize =
                vp_read(&vp->device, struct virtio_blk_config, blk_size);
        } else {
            vdrive->drive.blksize = DISK_SECTOR_SIZE;
        }
        if (vdrive->drive.blksize != DISK_SECTOR_SIZE) {
            dprintf(1, "virtio-blk %x:%x block size %d is unsupported\n",
                    pci_bdf_to_bus(bdf), pci_bdf_to_dev(bdf),
                    vdrive->drive.blksize);
            goto fail;
        }
        dprintf(3, "virtio-blk %x:%x blksize=%d sectors=%u\n",
                pci_bdf_to_bus(bdf), pci_bdf_to_dev(bdf),
                vdrive->drive.blksize, (u32)vdrive->drive.sectors);

        vdrive->drive.pchs.cylinder =
            vp_read(&vp->device, struct virtio_blk_config, cylinders);
        vdrive->drive.pchs.head =
            vp_read(&vp->device, struct virtio_blk_config, heads);
        vdrive->drive.pchs.sector =
            vp_read(&vp->device, struct virtio_blk_config, sectors);
    } else {
        struct virtio_blk_config cfg;
        vp_get_legacy(&vdrive->vp, 0, &cfg, sizeof(cfg));

        u64 f = vp_get_features(&vdrive->vp);
        vdrive->drive.blksize = (f & (1 << VIRTIO_BLK_F_BLK_SIZE)) ?
            cfg.blk_size : DISK_SECTOR_SIZE;

        vdrive->drive.sectors = cfg.capacity;
        dprintf(3, "virtio-blk %x:%x blksize=%d sectors=%u\n",
                pci_bdf_to_bus(bdf), pci_bdf_to_dev(bdf),
                vdrive->drive.blksize, (u32)vdrive->drive.sectors);

        if (vdrive->drive.blksize != DISK_SECTOR_SIZE) {
            dprintf(1, "virtio-blk %x:%x block size %d is unsupported\n",
                    pci_bdf_to_bus(bdf), pci_bdf_to_dev(bdf),
                    vdrive->drive.blksize);
            goto fail;
        }
        vdrive->drive.pchs.cylinder = cfg.cylinders;
        vdrive->drive.pchs.head = cfg.heads;
        vdrive->drive.pchs.sector = cfg.sectors;
    }

    char *desc = znprintf(MAXDESCSIZE, "Virtio disk PCI:%x:%x",
                          pci_bdf_to_bus(bdf), pci_bdf_to_dev(bdf));

    boot_add_hd(&vdrive->drive, desc, bootprio_find_pci_device(pci));

    status |= VIRTIO_CONFIG_S_DRIVER_OK;
    vp_set_status(&vdrive->vp, status);
    return;

fail:
    vp_reset(&vdrive->vp);
    free(vdrive->vq);
    free(vdrive);
}

void
virtio_blk_setup(void)
{
    ASSERT32FLAT();
    if (! CONFIG_VIRTIO_BLK)
        return;

    dprintf(3, "init virtio-blk\n");

    struct pci_device *pci;
    foreachpci(pci) {
        if (pci->vendor != PCI_VENDOR_ID_REDHAT_QUMRANET ||
            (pci->device != PCI_DEVICE_ID_VIRTIO_BLK_09 &&
             pci->device != PCI_DEVICE_ID_VIRTIO_BLK_10))
            continue;
        init_virtio_blk(pci);
    }
}
