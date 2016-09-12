#ifndef __USB_UAS_H
#define __USB_UAS_H

struct disk_op_s;
int uas_process_op(struct disk_op_s *op);
struct usbdevice_s;
int usb_uas_setup(struct usbdevice_s *usbdev);

#endif /* __USB_UAS_H */
