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
 */

#include "errno.h"
#include "byteswap.h"
#include "etherboot.h"
#include "ipxe/io.h"
#include "ipxe/iomap.h"
#include "ipxe/pci.h"
#include "ipxe/reboot.h"
#include "ipxe/virtio-pci.h"
#include "ipxe/virtio-ring.h"

int vp_find_vq(unsigned int ioaddr, int queue_index,
               struct vring_virtqueue *vq)
{
   struct vring * vr = &vq->vring;
   u16 num;

   /* select the queue */

   outw(queue_index, ioaddr + VIRTIO_PCI_QUEUE_SEL);

   /* check if the queue is available */

   num = inw(ioaddr + VIRTIO_PCI_QUEUE_NUM);
   if (!num) {
           DBG("VIRTIO-PCI ERROR: queue size is 0\n");
           return -1;
   }

   if (num > MAX_QUEUE_NUM) {
           DBG("VIRTIO-PCI ERROR: queue size %d > %d\n", num, MAX_QUEUE_NUM);
           return -1;
   }

   /* check if the queue is already active */

   if (inl(ioaddr + VIRTIO_PCI_QUEUE_PFN)) {
           DBG("VIRTIO-PCI ERROR: queue already active\n");
           return -1;
   }

   vq->queue_index = queue_index;

   /* initialize the queue */

   vring_init(vr, num, (unsigned char*)&vq->queue);

   /* activate the queue
    *
    * NOTE: vr->desc is initialized by vring_init()
    */

   outl((unsigned long)virt_to_phys(vr->desc) >> PAGE_SHIFT,
        ioaddr + VIRTIO_PCI_QUEUE_PFN);

   return num;
}

#define CFG_POS(vdev, field) \
    (vdev->cfg_cap_pos + offsetof(struct virtio_pci_cfg_cap, field))

static void prep_pci_cfg_cap(struct virtio_pci_modern_device *vdev,
                             struct virtio_pci_region *region,
                             size_t offset, u32 length)
{
    pci_write_config_byte(vdev->pci, CFG_POS(vdev, cap.bar), region->bar);
    pci_write_config_dword(vdev->pci, CFG_POS(vdev, cap.length), length);
    pci_write_config_dword(vdev->pci, CFG_POS(vdev, cap.offset),
        (intptr_t)(region->base + offset));
}

void vpm_iowrite8(struct virtio_pci_modern_device *vdev,
                  struct virtio_pci_region *region, u8 data, size_t offset)
{
    switch (region->flags & VIRTIO_PCI_REGION_TYPE_MASK) {
    case VIRTIO_PCI_REGION_MEMORY:
        writeb(data, region->base + offset);
        break;
    case VIRTIO_PCI_REGION_PORT:
        outb(data, region->base + offset);
        break;
    case VIRTIO_PCI_REGION_PCI_CONFIG:
        prep_pci_cfg_cap(vdev, region, offset, 1);
        pci_write_config_byte(vdev->pci, CFG_POS(vdev, pci_cfg_data), data);
        break;
    default:
        assert(0);
        break;
    }
}

void vpm_iowrite16(struct virtio_pci_modern_device *vdev,
                   struct virtio_pci_region *region, u16 data, size_t offset)
{
    data = cpu_to_le16(data);
    switch (region->flags & VIRTIO_PCI_REGION_TYPE_MASK) {
    case VIRTIO_PCI_REGION_MEMORY:
        writew(data, region->base + offset);
        break;
    case VIRTIO_PCI_REGION_PORT:
        outw(data, region->base + offset);
        break;
    case VIRTIO_PCI_REGION_PCI_CONFIG:
        prep_pci_cfg_cap(vdev, region, offset, 2);
        pci_write_config_word(vdev->pci, CFG_POS(vdev, pci_cfg_data), data);
        break;
    default:
        assert(0);
        break;
    }
}

void vpm_iowrite32(struct virtio_pci_modern_device *vdev,
                   struct virtio_pci_region *region, u32 data, size_t offset)
{
    data = cpu_to_le32(data);
    switch (region->flags & VIRTIO_PCI_REGION_TYPE_MASK) {
    case VIRTIO_PCI_REGION_MEMORY:
        writel(data, region->base + offset);
        break;
    case VIRTIO_PCI_REGION_PORT:
        outl(data, region->base + offset);
        break;
    case VIRTIO_PCI_REGION_PCI_CONFIG:
        prep_pci_cfg_cap(vdev, region, offset, 4);
        pci_write_config_dword(vdev->pci, CFG_POS(vdev, pci_cfg_data), data);
        break;
    default:
        assert(0);
        break;
    }
}

u8 vpm_ioread8(struct virtio_pci_modern_device *vdev,
               struct virtio_pci_region *region, size_t offset)
{
    uint8_t data;
    switch (region->flags & VIRTIO_PCI_REGION_TYPE_MASK) {
    case VIRTIO_PCI_REGION_MEMORY:
        data = readb(region->base + offset);
        break;
    case VIRTIO_PCI_REGION_PORT:
        data = inb(region->base + offset);
        break;
    case VIRTIO_PCI_REGION_PCI_CONFIG:
        prep_pci_cfg_cap(vdev, region, offset, 1);
        pci_read_config_byte(vdev->pci, CFG_POS(vdev, pci_cfg_data), &data);
        break;
    default:
        assert(0);
        data = 0;
        break;
    }
    return data;
}

u16 vpm_ioread16(struct virtio_pci_modern_device *vdev,
                 struct virtio_pci_region *region, size_t offset)
{
    uint16_t data;
    switch (region->flags & VIRTIO_PCI_REGION_TYPE_MASK) {
    case VIRTIO_PCI_REGION_MEMORY:
        data = readw(region->base + offset);
        break;
    case VIRTIO_PCI_REGION_PORT:
        data = inw(region->base + offset);
        break;
    case VIRTIO_PCI_REGION_PCI_CONFIG:
        prep_pci_cfg_cap(vdev, region, offset, 2);
        pci_read_config_word(vdev->pci, CFG_POS(vdev, pci_cfg_data), &data);
        break;
    default:
        assert(0);
        data = 0;
        break;
    }
    return le16_to_cpu(data);
}

u32 vpm_ioread32(struct virtio_pci_modern_device *vdev,
                 struct virtio_pci_region *region, size_t offset)
{
    uint32_t data;
    switch (region->flags & VIRTIO_PCI_REGION_TYPE_MASK) {
    case VIRTIO_PCI_REGION_MEMORY:
        data = readw(region->base + offset);
        break;
    case VIRTIO_PCI_REGION_PORT:
        data = inw(region->base + offset);
        break;
    case VIRTIO_PCI_REGION_PCI_CONFIG:
        prep_pci_cfg_cap(vdev, region, offset, 4);
        pci_read_config_dword(vdev->pci, CFG_POS(vdev, pci_cfg_data), &data);
        break;
    default:
        assert(0);
        data = 0;
        break;
    }
    return le32_to_cpu(data);
}

int virtio_pci_find_capability(struct pci_device *pci, uint8_t cfg_type)
{
    int pos;
    uint8_t type, bar;

    for (pos = pci_find_capability(pci, PCI_CAP_ID_VNDR);
         pos > 0;
         pos = pci_find_next_capability(pci, pos, PCI_CAP_ID_VNDR)) {

        pci_read_config_byte(pci, pos + offsetof(struct virtio_pci_cap,
            cfg_type), &type);
        pci_read_config_byte(pci, pos + offsetof(struct virtio_pci_cap,
            bar), &bar);

        /* Ignore structures with reserved BAR values */
        if (bar > 0x5) {
            continue;
        }

        if (type == cfg_type) {
            return pos;
        }
    }
    return 0;
}

int virtio_pci_map_capability(struct pci_device *pci, int cap, size_t minlen,
                              u32 align, u32 start, u32 size,
                              struct virtio_pci_region *region)
{
    u8 bar;
    u32 offset, length, base_raw;
    unsigned long base;

    pci_read_config_byte(pci, cap + offsetof(struct virtio_pci_cap, bar), &bar);
    pci_read_config_dword(pci, cap + offsetof(struct virtio_pci_cap, offset),
                          &offset);
    pci_read_config_dword(pci, cap + offsetof(struct virtio_pci_cap, length),
                          &length);

    if (length <= start) {
        DBG("VIRTIO-PCI bad capability len %d (>%d expected)\n", length, start);
        return -EINVAL;
    }
    if (length - start < minlen) {
        DBG("VIRTIO-PCI bad capability len %d (>=%zd expected)\n", length, minlen);
        return -EINVAL;
    }
    length -= start;
    if (start + offset < offset) {
        DBG("VIRTIO-PCI map wrap-around %d+%d\n", start, offset);
        return -EINVAL;
    }
    offset += start;
    if (offset & (align - 1)) {
        DBG("VIRTIO-PCI offset %d not aligned to %d\n", offset, align);
        return -EINVAL;
    }
    if (length > size) {
        length = size;
    }

    if (minlen + offset < minlen ||
        minlen + offset > pci_bar_size(pci, PCI_BASE_ADDRESS(bar))) {
        DBG("VIRTIO-PCI map virtio %zd@%d out of range on bar %i length %ld\n",
            minlen, offset,
            bar, pci_bar_size(pci, PCI_BASE_ADDRESS(bar)));
        return -EINVAL;
    }

    region->base = NULL;
    region->length = length;
    region->bar = bar;

    base = pci_bar_start(pci, PCI_BASE_ADDRESS(bar));
    if (base) {
        pci_read_config_dword(pci, PCI_BASE_ADDRESS(bar), &base_raw);

        if (base_raw & PCI_BASE_ADDRESS_SPACE_IO) {
            /* Region accessed using port I/O */
            region->base = (void *)(base + offset);
            region->flags = VIRTIO_PCI_REGION_PORT;
        } else {
            /* Region mapped into memory space */
            region->base = ioremap(base + offset, length);
            region->flags = VIRTIO_PCI_REGION_MEMORY;
        }
    }
    if (!region->base) {
        /* Region accessed via PCI config space window */
	    region->base = (void *)(intptr_t)offset;
        region->flags = VIRTIO_PCI_REGION_PCI_CONFIG;
    }
    return 0;
}

void virtio_pci_unmap_capability(struct virtio_pci_region *region)
{
    unsigned region_type = region->flags & VIRTIO_PCI_REGION_TYPE_MASK;
    if (region_type == VIRTIO_PCI_REGION_MEMORY) {
        iounmap(region->base);
    }
}

void vpm_notify(struct virtio_pci_modern_device *vdev,
                struct vring_virtqueue *vq)
{
    vpm_iowrite16(vdev, &vq->notification, (u16)vq->queue_index, 0);
}

int vpm_find_vqs(struct virtio_pci_modern_device *vdev,
                 unsigned nvqs, struct vring_virtqueue *vqs)
{
    unsigned i;
    struct vring_virtqueue *vq;
    u16 size, off;
    u32 notify_offset_multiplier;
    int err;

    if (nvqs > vpm_ioread16(vdev, &vdev->common, COMMON_OFFSET(num_queues))) {
        return -ENOENT;
    }

    /* Read notify_off_multiplier from config space. */
    pci_read_config_dword(vdev->pci,
        vdev->notify_cap_pos + offsetof(struct virtio_pci_notify_cap,
        notify_off_multiplier),
        &notify_offset_multiplier);

    for (i = 0; i < nvqs; i++) {
        /* Select the queue we're interested in */
        vpm_iowrite16(vdev, &vdev->common, (u16)i, COMMON_OFFSET(queue_select));

        /* Check if queue is either not available or already active. */
        size = vpm_ioread16(vdev, &vdev->common, COMMON_OFFSET(queue_size));
        /* QEMU has a bug where queues don't revert to inactive on device
         * reset. Skip checking the queue_enable field until it is fixed.
         */
        if (!size /*|| vpm_ioread16(vdev, &vdev->common.queue_enable)*/)
            return -ENOENT;

        if (size & (size - 1)) {
            DBG("VIRTIO-PCI %p: bad queue size %d", vdev, size);
            return -EINVAL;
        }

        vq = &vqs[i];
        vq->queue_index = i;

        /* get offset of notification word for this vq */
        off = vpm_ioread16(vdev, &vdev->common, COMMON_OFFSET(queue_notify_off));
        vq->vring.num = size;

        vring_init(&vq->vring, size, (unsigned char *)vq->queue);

        /* activate the queue */
        vpm_iowrite16(vdev, &vdev->common, size, COMMON_OFFSET(queue_size));

        vpm_iowrite64(vdev, &vdev->common, virt_to_phys(vq->vring.desc),
                      COMMON_OFFSET(queue_desc_lo),
                      COMMON_OFFSET(queue_desc_hi));
        vpm_iowrite64(vdev, &vdev->common, virt_to_phys(vq->vring.avail),
                      COMMON_OFFSET(queue_avail_lo),
                      COMMON_OFFSET(queue_avail_hi));
        vpm_iowrite64(vdev, &vdev->common, virt_to_phys(vq->vring.used),
                      COMMON_OFFSET(queue_used_lo),
                      COMMON_OFFSET(queue_used_hi));

        err = virtio_pci_map_capability(vdev->pci,
            vdev->notify_cap_pos, 2, 2,
            off * notify_offset_multiplier, 2,
            &vq->notification);
        if (err) {
            goto err_map_notify;
        }
    }

    /* Select and activate all queues. Has to be done last: once we do
     * this, there's no way to go back except reset.
     */
    for (i = 0; i < nvqs; i++) {
        vq = &vqs[i];
        vpm_iowrite16(vdev, &vdev->common, (u16)vq->queue_index,
                      COMMON_OFFSET(queue_select));
        vpm_iowrite16(vdev, &vdev->common, 1, COMMON_OFFSET(queue_enable));
    }
    return 0;

err_map_notify:
    /* Undo the virtio_pci_map_capability calls. */
    while (i-- > 0) {
        virtio_pci_unmap_capability(&vqs[i].notification);
    }
    return err;
}
