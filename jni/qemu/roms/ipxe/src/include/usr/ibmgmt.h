#ifndef _USR_IBMGMT_H
#define _USR_IBMGMT_H

/** @file
 *
 * Infiniband device management
 *
 */

FILE_LICENCE ( GPL2_OR_LATER_OR_UBDL );

struct ib_device;

extern void ibstat ( struct ib_device *ibdev );

#endif /* _USR_IBMGMT_H */
