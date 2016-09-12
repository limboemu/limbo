/* virtio-pci.c - pci interface for virtio interface
 *
 * (c) Copyright 2008 Bull S.A.S.
 *
 *  Author: Laurent Vivier <Laurent.Vivier@bull.net>
 *
 * some parts from Linux Virtio PCI driver
 *
 *  Copyright IBM Corp. 2007
 *  Authors: Anthony Liguori  <aliguori@us.ibm.com>
 *
 *  Adopted for Seabios: Gleb Natapov <gleb@redhat.com>
 *
 * This work is licensed under the terms of the GNU LGPLv3
 * See the COPYING file in the top-level directory.
 */

#include "config.h" // CONFIG_DEBUG_LEVEL
#include "malloc.h" // free
#include "output.h" // dprintf
#include "pci.h" // pci_config_readl
#include "pci_regs.h" // PCI_BASE_ADDRESS_0
#include "string.h" // memset
#include "virtio-pci.h"
#include "virtio-ring.h"

u64 vp_get_features(struct vp_device *vp)
{
    u32 f0, f1;

    if (vp->use_modern) {
        vp_write(&vp->common, virtio_pci_common_cfg, device_feature_select, 0);
        f0 = vp_read(&vp->common, virtio_pci_common_cfg, device_feature);
        vp_write(&vp->common, virtio_pci_common_cfg, device_feature_select, 1);
        f1 = vp_read(&vp->common, virtio_pci_common_cfg, device_feature);
    } else {
        f0 = vp_read(&vp->legacy, virtio_pci_legacy, host_features);
        f1 = 0;
    }
    return ((u64)f1 << 32) | f0;
}

void vp_set_features(struct vp_device *vp, u64 features)
{
    u32 f0, f1;

    f0 = features;
    f1 = features >> 32;

    if (vp->use_modern) {
        vp_write(&vp->common, virtio_pci_common_cfg, guest_feature_select, 0);
        vp_write(&vp->common, virtio_pci_common_cfg, guest_feature, f0);
        vp_write(&vp->common, virtio_pci_common_cfg, guest_feature_select, 1);
        vp_write(&vp->common, virtio_pci_common_cfg, guest_feature, f1);
    } else {
        vp_write(&vp->legacy, virtio_pci_legacy, guest_features, f0);
    }
}

u8 vp_get_status(struct vp_device *vp)
{
    if (vp->use_modern) {
        return vp_read(&vp->common, virtio_pci_common_cfg, device_status);
    } else {
        return vp_read(&vp->legacy, virtio_pci_legacy, status);
    }
}

void vp_set_status(struct vp_device *vp, u8 status)
{
    if (status == 0)        /* reset */
        return;
    if (vp->use_modern) {
        vp_write(&vp->common, virtio_pci_common_cfg, device_status, status);
    } else {
        vp_write(&vp->legacy, virtio_pci_legacy, status, status);
    }
}

u8 vp_get_isr(struct vp_device *vp)
{
    if (vp->use_modern) {
        return vp_read(&vp->isr, virtio_pci_isr, isr);
    } else {
        return vp_read(&vp->legacy, virtio_pci_legacy, isr);
    }
}

void vp_reset(struct vp_device *vp)
{
    if (vp->use_modern) {
        vp_write(&vp->common, virtio_pci_common_cfg, device_status, 0);
        vp_read(&vp->isr, virtio_pci_isr, isr);
    } else {
        vp_write(&vp->legacy, virtio_pci_legacy, status, 0);
        vp_read(&vp->legacy, virtio_pci_legacy, isr);
    }
}

void vp_notify(struct vp_device *vp, struct vring_virtqueue *vq)
{
    if (vp->use_modern) {
        u32 addr = vp->notify.addr +
            vq->queue_notify_off *
            vp->notify_off_multiplier;
        if (vp->notify.is_io) {
            outw(vq->queue_index, addr);
        } else {
            writew((void*)addr, vq->queue_index);
        }
        dprintf(9, "vp notify %x (%d) -- 0x%x\n",
                addr, 2, vq->queue_index);
    } else {
        vp_write(&vp->legacy, virtio_pci_legacy, queue_notify, vq->queue_index);
    }
}

int vp_find_vq(struct vp_device *vp, int queue_index,
               struct vring_virtqueue **p_vq)
{
   u16 num;

   ASSERT32FLAT();
   struct vring_virtqueue *vq = *p_vq = memalign_high(PAGE_SIZE, sizeof(*vq));
   if (!vq) {
       warn_noalloc();
       goto fail;
   }
   memset(vq, 0, sizeof(*vq));


   /* select the queue */
   if (vp->use_modern) {
       vp_write(&vp->common, virtio_pci_common_cfg, queue_select, queue_index);
   } else {
       vp_write(&vp->legacy, virtio_pci_legacy, queue_sel, queue_index);
   }

   /* check if the queue is available */
   if (vp->use_modern) {
       num = vp_read(&vp->common, virtio_pci_common_cfg, queue_size);
       if (num > MAX_QUEUE_NUM) {
           vp_write(&vp->common, virtio_pci_common_cfg, queue_size,
                    MAX_QUEUE_NUM);
           num = vp_read(&vp->common, virtio_pci_common_cfg, queue_size);
       }
   } else {
       num = vp_read(&vp->legacy, virtio_pci_legacy, queue_num);
   }
   if (!num) {
       dprintf(1, "ERROR: queue size is 0\n");
       goto fail;
   }
   if (num > MAX_QUEUE_NUM) {
       dprintf(1, "ERROR: queue size %d > %d\n", num, MAX_QUEUE_NUM);
       goto fail;
   }

   /* check if the queue is already active */
   if (vp->use_modern) {
       if (vp_read(&vp->common, virtio_pci_common_cfg, queue_enable)) {
           dprintf(1, "ERROR: queue already active\n");
           goto fail;
       }
   } else {
       if (vp_read(&vp->legacy, virtio_pci_legacy, queue_pfn)) {
           dprintf(1, "ERROR: queue already active\n");
           goto fail;
       }
   }
   vq->queue_index = queue_index;

   /* initialize the queue */
   struct vring * vr = &vq->vring;
   vring_init(vr, num, (unsigned char*)&vq->queue);

   /* activate the queue
    *
    * NOTE: vr->desc is initialized by vring_init()
    */

   if (vp->use_modern) {
       vp_write(&vp->common, virtio_pci_common_cfg, queue_desc_lo,
                (unsigned long)virt_to_phys(vr->desc));
       vp_write(&vp->common, virtio_pci_common_cfg, queue_desc_hi, 0);
       vp_write(&vp->common, virtio_pci_common_cfg, queue_avail_lo,
                (unsigned long)virt_to_phys(vr->avail));
       vp_write(&vp->common, virtio_pci_common_cfg, queue_avail_hi, 0);
       vp_write(&vp->common, virtio_pci_common_cfg, queue_used_lo,
                (unsigned long)virt_to_phys(vr->used));
       vp_write(&vp->common, virtio_pci_common_cfg, queue_used_hi, 0);
       vp_write(&vp->common, virtio_pci_common_cfg, queue_enable, 1);
       vq->queue_notify_off = vp_read(&vp->common, virtio_pci_common_cfg,
                                      queue_notify_off);
   } else {
       vp_write(&vp->legacy, virtio_pci_legacy, queue_pfn,
                (unsigned long)virt_to_phys(vr->desc) >> PAGE_SHIFT);
   }
   return num;

fail:
   free(vq);
   *p_vq = NULL;
   return -1;
}

void vp_init_simple(struct vp_device *vp, struct pci_device *pci)
{
    u8 cap = pci_find_capability(pci, PCI_CAP_ID_VNDR, 0);
    struct vp_cap *vp_cap;
    u32 addr, offset, mul;
    u8 type;

    memset(vp, 0, sizeof(*vp));
    while (cap != 0) {
        type = pci_config_readb(pci->bdf, cap +
                                offsetof(struct virtio_pci_cap, cfg_type));
        switch (type) {
        case VIRTIO_PCI_CAP_COMMON_CFG:
            vp_cap = &vp->common;
            break;
        case VIRTIO_PCI_CAP_NOTIFY_CFG:
            vp_cap = &vp->notify;
            mul = offsetof(struct virtio_pci_notify_cap, notify_off_multiplier);
            vp->notify_off_multiplier = pci_config_readl(pci->bdf, cap + mul);
            break;
        case VIRTIO_PCI_CAP_ISR_CFG:
            vp_cap = &vp->isr;
            break;
        case VIRTIO_PCI_CAP_DEVICE_CFG:
            vp_cap = &vp->device;
            break;
        default:
            vp_cap = NULL;
            break;
        }
        if (vp_cap && !vp_cap->cap) {
            vp_cap->cap = cap;
            vp_cap->bar = pci_config_readb(pci->bdf, cap +
                                           offsetof(struct virtio_pci_cap, bar));
            offset = pci_config_readl(pci->bdf, cap +
                                      offsetof(struct virtio_pci_cap, offset));
            addr = pci_config_readl(pci->bdf, PCI_BASE_ADDRESS_0 + 4 * vp_cap->bar);
            if (addr & PCI_BASE_ADDRESS_SPACE_IO) {
                vp_cap->is_io = 1;
                addr &= PCI_BASE_ADDRESS_IO_MASK;
            } else {
                vp_cap->is_io = 0;
                addr &= PCI_BASE_ADDRESS_MEM_MASK;
            }
            vp_cap->addr = addr + offset;
            dprintf(3, "pci dev %x:%x virtio cap at 0x%x type %d "
                    "bar %d at 0x%08x off +0x%04x [%s]\n",
                    pci_bdf_to_bus(pci->bdf), pci_bdf_to_dev(pci->bdf),
                    vp_cap->cap, type, vp_cap->bar, addr, offset,
                    vp_cap->is_io ? "io" : "mmio");
        }

        cap = pci_find_capability(pci, PCI_CAP_ID_VNDR, cap);
    }

    if (vp->common.cap && vp->notify.cap && vp->isr.cap && vp->device.cap) {
        dprintf(1, "pci dev %x:%x using modern (1.0) virtio mode\n",
                pci_bdf_to_bus(pci->bdf), pci_bdf_to_dev(pci->bdf));
        vp->use_modern = 1;
    } else {
        dprintf(1, "pci dev %x:%x using legacy (0.9.5) virtio mode\n",
                pci_bdf_to_bus(pci->bdf), pci_bdf_to_dev(pci->bdf));
        vp->legacy.bar = 0;
        vp->legacy.addr = pci_config_readl(pci->bdf, PCI_BASE_ADDRESS_0) &
            PCI_BASE_ADDRESS_IO_MASK;
        vp->legacy.is_io = 1;
    }

    vp_reset(vp);
    pci_config_maskw(pci->bdf, PCI_COMMAND, 0, PCI_COMMAND_MASTER);
    vp_set_status(vp, VIRTIO_CONFIG_S_ACKNOWLEDGE |
                  VIRTIO_CONFIG_S_DRIVER );
}
