#ifndef _VIRTIO_PCI_H_
# define _VIRTIO_PCI_H_

/* A 32-bit r/o bitmask of the features supported by the host */
#define VIRTIO_PCI_HOST_FEATURES        0

/* A 32-bit r/w bitmask of features activated by the guest */
#define VIRTIO_PCI_GUEST_FEATURES       4

/* A 32-bit r/w PFN for the currently selected queue */
#define VIRTIO_PCI_QUEUE_PFN            8

/* A 16-bit r/o queue size for the currently selected queue */
#define VIRTIO_PCI_QUEUE_NUM            12

/* A 16-bit r/w queue selector */
#define VIRTIO_PCI_QUEUE_SEL            14

/* A 16-bit r/w queue notifier */
#define VIRTIO_PCI_QUEUE_NOTIFY         16

/* An 8-bit device status register.  */
#define VIRTIO_PCI_STATUS               18

/* An 8-bit r/o interrupt status register.  Reading the value will return the
 * current contents of the ISR and will also clear it.  This is effectively
 * a read-and-acknowledge. */
#define VIRTIO_PCI_ISR                  19

/* The bit of the ISR which indicates a device configuration change. */
#define VIRTIO_PCI_ISR_CONFIG           0x2

/* The remaining space is defined by each driver as the per-driver
 * configuration space */
#define VIRTIO_PCI_CONFIG               20

/* Virtio ABI version, this must match exactly */
#define VIRTIO_PCI_ABI_VERSION          0

/* PCI capability types: */
#define VIRTIO_PCI_CAP_COMMON_CFG       1  /* Common configuration */
#define VIRTIO_PCI_CAP_NOTIFY_CFG       2  /* Notifications */
#define VIRTIO_PCI_CAP_ISR_CFG          3  /* ISR access */
#define VIRTIO_PCI_CAP_DEVICE_CFG       4  /* Device specific configuration */
#define VIRTIO_PCI_CAP_PCI_CFG          5  /* PCI configuration access */

#define __u8       uint8_t
#define __le16     uint16_t
#define __le32     uint32_t
#define __le64     uint64_t

/* This is the PCI capability header: */
struct virtio_pci_cap {
    __u8 cap_vndr;    /* Generic PCI field: PCI_CAP_ID_VNDR */
    __u8 cap_next;    /* Generic PCI field: next ptr. */
    __u8 cap_len;     /* Generic PCI field: capability length */
    __u8 cfg_type;    /* Identifies the structure. */
    __u8 bar;         /* Where to find it. */
    __u8 padding[3];  /* Pad to full dword. */
    __le32 offset;    /* Offset within bar. */
    __le32 length;    /* Length of the structure, in bytes. */
};

struct virtio_pci_notify_cap {
    struct virtio_pci_cap cap;
    __le32 notify_off_multiplier; /* Multiplier for queue_notify_off. */
};

struct virtio_pci_cfg_cap {
    struct virtio_pci_cap cap;
    __u8 pci_cfg_data[4]; /* Data for BAR access. */
};

/* Fields in VIRTIO_PCI_CAP_COMMON_CFG: */
struct virtio_pci_common_cfg {
    /* About the whole device. */
    __le32 device_feature_select; /* read-write */
    __le32 device_feature;        /* read-only */
    __le32 guest_feature_select;  /* read-write */
    __le32 guest_feature;         /* read-write */
    __le16 msix_config;           /* read-write */
    __le16 num_queues;            /* read-only */
    __u8 device_status;           /* read-write */
    __u8 config_generation;       /* read-only */

    /* About a specific virtqueue. */
    __le16 queue_select;          /* read-write */
    __le16 queue_size;            /* read-write, power of 2. */
    __le16 queue_msix_vector;     /* read-write */
    __le16 queue_enable;          /* read-write */
    __le16 queue_notify_off;      /* read-only */
    __le32 queue_desc_lo;         /* read-write */
    __le32 queue_desc_hi;         /* read-write */
    __le32 queue_avail_lo;        /* read-write */
    __le32 queue_avail_hi;        /* read-write */
    __le32 queue_used_lo;         /* read-write */
    __le32 queue_used_hi;         /* read-write */
};

/* Virtio 1.0 PCI region descriptor. We support memory mapped I/O, port I/O,
 * and PCI config space access via the cfg PCI capability as a fallback. */
struct virtio_pci_region {
    void *base;
    size_t length;
    u8 bar;

/* How to interpret the base field */
#define VIRTIO_PCI_REGION_TYPE_MASK  0x00000003
/* The base field is a memory address */
#define VIRTIO_PCI_REGION_MEMORY     0x00000001
/* The base field is a port address */
#define VIRTIO_PCI_REGION_PORT       0x00000002
/* The base field is an offset within the PCI bar */
#define VIRTIO_PCI_REGION_PCI_CONFIG 0x00000003
    unsigned flags;
};

/* Virtio 1.0 device state */
struct virtio_pci_modern_device {
    struct pci_device *pci;

    /* VIRTIO_PCI_CAP_PCI_CFG position */
    int cfg_cap_pos;

    /* VIRTIO_PCI_CAP_COMMON_CFG data */
    struct virtio_pci_region common;

    /* VIRTIO_PCI_CAP_DEVICE_CFG data */
    struct virtio_pci_region device;

    /* VIRTIO_PCI_CAP_ISR_CFG data */
    struct virtio_pci_region isr;

    /* VIRTIO_PCI_CAP_NOTIFY_CFG data */
    int notify_cap_pos;
};

static inline u32 vp_get_features(unsigned int ioaddr)
{
   return inl(ioaddr + VIRTIO_PCI_HOST_FEATURES);
}

static inline void vp_set_features(unsigned int ioaddr, u32 features)
{
        outl(features, ioaddr + VIRTIO_PCI_GUEST_FEATURES);
}

static inline void vp_get(unsigned int ioaddr, unsigned offset,
                     void *buf, unsigned len)
{
   u8 *ptr = buf;
   unsigned i;

   for (i = 0; i < len; i++)
           ptr[i] = inb(ioaddr + VIRTIO_PCI_CONFIG + offset + i);
}

static inline u8 vp_get_status(unsigned int ioaddr)
{
   return inb(ioaddr + VIRTIO_PCI_STATUS);
}

static inline void vp_set_status(unsigned int ioaddr, u8 status)
{
   if (status == 0)        /* reset */
           return;
   outb(status, ioaddr + VIRTIO_PCI_STATUS);
}

static inline u8 vp_get_isr(unsigned int ioaddr)
{
   return inb(ioaddr + VIRTIO_PCI_ISR);
}

static inline void vp_reset(unsigned int ioaddr)
{
   outb(0, ioaddr + VIRTIO_PCI_STATUS);
   (void)inb(ioaddr + VIRTIO_PCI_ISR);
}

static inline void vp_notify(unsigned int ioaddr, int queue_index)
{
   outw(queue_index, ioaddr + VIRTIO_PCI_QUEUE_NOTIFY);
}

static inline void vp_del_vq(unsigned int ioaddr, int queue_index)
{
   /* select the queue */

   outw(queue_index, ioaddr + VIRTIO_PCI_QUEUE_SEL);

   /* deactivate the queue */

   outl(0, ioaddr + VIRTIO_PCI_QUEUE_PFN);
}

struct vring_virtqueue;

int vp_find_vq(unsigned int ioaddr, int queue_index,
               struct vring_virtqueue *vq);

/* Virtio 1.0 I/O routines abstract away the three possible HW access
 * mechanisms - memory, port I/O, and PCI cfg space access. Also built-in
 * are endianness conversions - to LE on write and from LE on read. */

void vpm_iowrite8(struct virtio_pci_modern_device *vdev,
                  struct virtio_pci_region *region, u8 data, size_t offset);

void vpm_iowrite16(struct virtio_pci_modern_device *vdev,
                   struct virtio_pci_region *region, u16 data, size_t offset);

void vpm_iowrite32(struct virtio_pci_modern_device *vdev,
                   struct virtio_pci_region *region, u32 data, size_t offset);

static inline void vpm_iowrite64(struct virtio_pci_modern_device *vdev,
                                 struct virtio_pci_region *region,
                                 u64 data, size_t offset_lo, size_t offset_hi)
{
    vpm_iowrite32(vdev, region, (u32)data, offset_lo);
    vpm_iowrite32(vdev, region, data >> 32, offset_hi);
}

u8 vpm_ioread8(struct virtio_pci_modern_device *vdev,
               struct virtio_pci_region *region, size_t offset);

u16 vpm_ioread16(struct virtio_pci_modern_device *vdev,
                 struct virtio_pci_region *region, size_t offset);

u32 vpm_ioread32(struct virtio_pci_modern_device *vdev,
                 struct virtio_pci_region *region, size_t offset);

/* Virtio 1.0 device manipulation routines */

#define COMMON_OFFSET(field) offsetof(struct virtio_pci_common_cfg, field)

static inline void vpm_reset(struct virtio_pci_modern_device *vdev)
{
    vpm_iowrite8(vdev, &vdev->common, 0, COMMON_OFFSET(device_status));
    while (vpm_ioread8(vdev, &vdev->common, COMMON_OFFSET(device_status)))
        mdelay(1);
}

static inline u8 vpm_get_status(struct virtio_pci_modern_device *vdev)
{
    return vpm_ioread8(vdev, &vdev->common, COMMON_OFFSET(device_status));
}

static inline void vpm_add_status(struct virtio_pci_modern_device *vdev,
                                  u8 status)
{
    u8 curr_status = vpm_ioread8(vdev, &vdev->common, COMMON_OFFSET(device_status));
    vpm_iowrite8(vdev, &vdev->common,
                 curr_status | status, COMMON_OFFSET(device_status));
}

static inline u64 vpm_get_features(struct virtio_pci_modern_device *vdev)
{
    u32 features_lo, features_hi;

    vpm_iowrite32(vdev, &vdev->common, 0, COMMON_OFFSET(device_feature_select));
    features_lo = vpm_ioread32(vdev, &vdev->common, COMMON_OFFSET(device_feature));
    vpm_iowrite32(vdev, &vdev->common, 1, COMMON_OFFSET(device_feature_select));
    features_hi = vpm_ioread32(vdev, &vdev->common, COMMON_OFFSET(device_feature));

    return ((u64)features_hi << 32) | features_lo;
}

static inline void vpm_set_features(struct virtio_pci_modern_device *vdev,
                                    u64 features)
{
    u32 features_lo = (u32)features;
    u32 features_hi = features >> 32;

    vpm_iowrite32(vdev, &vdev->common, 0, COMMON_OFFSET(guest_feature_select));
    vpm_iowrite32(vdev, &vdev->common, features_lo, COMMON_OFFSET(guest_feature));
    vpm_iowrite32(vdev, &vdev->common, 1, COMMON_OFFSET(guest_feature_select));
    vpm_iowrite32(vdev, &vdev->common, features_hi, COMMON_OFFSET(guest_feature));
}

static inline void vpm_get(struct virtio_pci_modern_device *vdev,
                           unsigned offset, void *buf, unsigned len)
{
    u8 *ptr = buf;
    unsigned i;

    for (i = 0; i < len; i++)
        ptr[i] = vpm_ioread8(vdev, &vdev->device, offset + i);
}

static inline u8 vpm_get_isr(struct virtio_pci_modern_device *vdev)
{
    return vpm_ioread8(vdev, &vdev->isr, 0);
}

void vpm_notify(struct virtio_pci_modern_device *vdev,
                struct vring_virtqueue *vq);

int vpm_find_vqs(struct virtio_pci_modern_device *vdev,
                 unsigned nvqs, struct vring_virtqueue *vqs);

int virtio_pci_find_capability(struct pci_device *pci, uint8_t cfg_type);

int virtio_pci_map_capability(struct pci_device *pci, int cap, size_t minlen,
                              u32 align, u32 start, u32 size,
                              struct virtio_pci_region *region);

void virtio_pci_unmap_capability(struct virtio_pci_region *region);
#endif /* _VIRTIO_PCI_H_ */
