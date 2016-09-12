/******************************************************************************
 * Copyright (c) 2013 IBM Corporation
 * All rights reserved.
 * This program and the accompanying materials
 * are made available under the terms of the BSD License
 * which accompanies this distribution, and is available at
 * http://www.opensource.org/licenses/bsd-license.php
 *
 * Contributors:
 *     IBM Corporation - initial implementation
 *****************************************************************************/
/*
 * All functions concerning interface to slof
 */

#include <string.h>
#include "helpers.h"
#include "usb-core.h"
#include "paflof.h"

#undef SLOF_DEBUG
//#define SLOF_DEBUG
#ifdef SLOF_DEBUG
#define dprintf(_x ...) do { printf(_x); } while(0)
#else
#define dprintf(_x ...)
#endif

static int slof_usb_handle(struct usb_dev *dev)
{
	struct slof_usb_dev sdev;
	sdev.port = dev->port;
	sdev.addr = dev->addr;
	sdev.hcitype = dev->hcidev->type;
	sdev.num  = dev->hcidev->num;
	sdev.udev  = dev;

	if (dev->class == DEV_HID_KEYB) {
		dprintf("Keyboard %ld %ld\n", dev->hcidev->type, dev->hcidev->num);
		sdev.devtype = DEVICE_KEYBOARD;
		forth_push((long)&sdev);
		forth_eval("s\" dev-keyb.fs\" INCLUDED");
	} else if (dev->class == DEV_HID_MOUSE) {
		dprintf("Mouse %ld %ld\n", dev->hcidev->type, dev->hcidev->num);
		sdev.devtype = DEVICE_MOUSE;
		forth_push((long)&sdev);
		forth_eval("s\" dev-mouse.fs\" INCLUDED");
	} else if ((dev->class >> 16 & 0xFF) == 8) {
		dprintf("MASS Storage device %ld %ld\n", dev->hcidev->type, dev->hcidev->num);
		sdev.devtype = DEVICE_DISK;
		forth_push((long)&sdev);
		forth_eval("s\" dev-storage.fs\" INCLUDED");
	} else if (dev->class == DEV_HUB) {
		dprintf("Generic hub device %ld %ld\n", dev->hcidev->type,
			dev->hcidev->num);
		sdev.devtype = DEVICE_HUB;
		forth_push((long)&sdev);
		forth_eval("s\" dev-hub.fs\" INCLUDED");
	}
	return true;
}

void usb_slof_populate_new_device(struct usb_dev *dev)
{
	switch (usb_get_intf_class(dev->class)) {
	case 3:
		dprintf("HID found %06X\n", dev->class);
		slof_usb_handle(dev);
		break;
	case 8:
		dprintf("MASS STORAGE found %d %06X\n", dev->intf_num,
			dev->class);
		if ((dev->class & 0x50) != 0x50) { /* Bulk-only supported */
			printf("Device not supported %06X\n", dev->class);
			break;
		}

		if (!usb_msc_reset(dev)) {
			printf("%s: bulk reset failed\n", __func__);
			break;
		}
		SLOF_msleep(100);
		slof_usb_handle(dev);
		break;
	case 9:
		dprintf("HUB found\n");
		slof_usb_handle(dev);
		break;
	default:
		printf("USB Interface class -%x- Not supported\n", dev->class);
		break;
	}
}
