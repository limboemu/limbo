#ifndef _IPXE_NTP_H
#define _IPXE_NTP_H

/** @file
 *
 * Network Time Protocol
 *
 */

FILE_LICENCE ( GPL2_OR_LATER_OR_UBDL );

#include <stdint.h>
#include <ipxe/in.h>
#include <ipxe/interface.h>

/** NTP port */
#define NTP_PORT 123

/** An NTP short-format timestamp */
struct ntp_short {
	/** Seconds */
	uint16_t seconds;
	/** Fraction of a second */
	uint16_t fraction;
} __attribute__ (( packed ));

/** An NTP timestamp */
struct ntp_timestamp {
	/** Seconds */
	uint32_t seconds;
	/** Fraction of a second */
	uint32_t fraction;
} __attribute__ (( packed ));

/** An NTP reference identifier */
union ntp_id {
	/** Textual identifier */
	char text[4];
	/** IPv4 address */
	struct in_addr in;
	/** Opaque integer */
	uint32_t opaque;
};

/** An NTP header */
struct ntp_header {
	/** Flags */
	uint8_t flags;
	/** Stratum */
	uint8_t stratum;
	/** Polling rate */
	int8_t poll;
	/** Precision */
	int8_t precision;
	/** Root delay */
	struct ntp_short delay;
	/** Root dispersion */
	struct ntp_short dispersion;
	/** Reference clock identifier */
	union ntp_id id;
	/** Reference timestamp */
	struct ntp_timestamp reference;
	/** Originate timestamp */
	struct ntp_timestamp originate;
	/** Receive timestamp */
	struct ntp_timestamp receive;
	/** Transmit timestamp */
	struct ntp_timestamp transmit;
} __attribute__ (( packed ));

/** Leap second indicator: unknown */
#define NTP_FL_LI_UNKNOWN 0xc0

/** NTP version: 1 */
#define NTP_FL_VN_1 0x20

/** NTP mode: client */
#define NTP_FL_MODE_CLIENT 0x03

/** NTP mode: server */
#define NTP_FL_MODE_SERVER 0x04

/** NTP mode mask */
#define NTP_FL_MODE_MASK 0x07

/** NTP timestamp for start of Unix epoch */
#define NTP_EPOCH 2208988800UL

/** NTP fraction of a second magic value
 *
 * This is a policy decision.
 */
#define NTP_FRACTION_MAGIC 0x69505845UL

/** NTP minimum retransmission timeout
 *
 * This is a policy decision.
 */
#define NTP_MIN_TIMEOUT ( 1 * TICKS_PER_SEC )

/** NTP maximum retransmission timeout
 *
 * This is a policy decision.
 */
#define NTP_MAX_TIMEOUT ( 10 * TICKS_PER_SEC )

extern int start_ntp ( struct interface *job, const char *hostname );

#endif /* _IPXE_NTP_H */
