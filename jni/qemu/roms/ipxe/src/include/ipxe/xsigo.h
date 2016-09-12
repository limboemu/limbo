#ifndef _IPXE_XSIGO_H
#define _IPXE_XSIGO_H

/** @file
 *
 * Xsigo virtual Ethernet devices
 *
 */

FILE_LICENCE ( GPL2_OR_LATER_OR_UBDL );

#include <stdint.h>
#include <ipxe/infiniband.h>
#include <ipxe/eoib.h>

/** Xsigo directory service record name */
#define XDS_SERVICE_NAME "XSIGOXDS"

/** Xsigo configuration manager service ID */
#define XCM_SERVICE_ID { 0x00, 0x00, 0x00, 0x00, 0x02, 0x13, 0x97, 0x01 }

/** Xsigo management class */
#define XSIGO_MGMT_CLASS 0x0b

/** Xsigo management class version */
#define XSIGO_MGMT_CLASS_VERSION 2

/** Xsigo configuration manager request MAD */
#define XSIGO_ATTR_XCM_REQUEST 0xb002

/** Generic operating system type */
#define XSIGO_OS_TYPE_GENERIC 0x40

/** Xsigo virtual Ethernet broadcast GID prefix */
#define XVE_PREFIX 0xff15101cUL

/** Xsigo resource types */
enum xsigo_resource_type {
	/** Virtual Ethernet resource type */
	XSIGO_RESOURCE_XVE = ( 1 << 6 ),
	/** Absence-of-high-availability "resource" type */
	XSIGO_RESOURCE_NO_HA = ( 1 << 4 ),
};

/** A Xsigo server identifier */
struct xsigo_server_id {
	/** Virtual machine ID */
	uint32_t vm;
	/** Port GUID */
	union ib_guid guid;
} __attribute__ (( packed ));

/** A Xsigo configuration manager identifier */
struct xsigo_manager_id {
	/** Port GUID */
	union ib_guid guid;
	/** LID */
	uint16_t lid;
	/** Reserved */
	uint8_t reserved[10];
} __attribute__ (( packed ));

/** A Xsigo configuration manager request MAD */
struct xsigo_managers_request {
	/** MAD header */
	struct ib_mad_hdr mad_hdr;
	/** Reserved */
	uint8_t reserved0[32];
	/** Server ID */
	struct xsigo_server_id server;
	/** Hostname */
	char hostname[ 65 /* Seriously, guys? */ ];
	/** OS version */
	char os_version[32];
	/** CPU architecture */
	char arch[16];
	/** OS type */
	uint8_t os_type;
	/** Reserved */
	uint8_t reserved1[3];
	/** Firmware version */
	uint64_t firmware_version;
	/** Hardware version */
	uint32_t hardware_version;
	/** Driver version */
	uint32_t driver_version;
	/** System ID */
	union ib_gid system_id;
	/** Resource types */
	uint16_t resources;
	/** Reserved */
	uint8_t reserved2[2];
	/** Build version */
	char build[16];
	/** Reserved */
	uint8_t reserved3[19];
} __attribute__ (( packed ));

/** Resource types are present */
#define XSIGO_RESOURCES_PRESENT 0x8000

/** A Xsigo configuration manager reply MAD */
struct xsigo_managers_reply {
	/** MAD header */
	struct ib_mad_hdr mad_hdr;
	/** Reserved */
	uint8_t reserved0[32];
	/** Server ID */
	struct xsigo_server_id server;
	/** Number of XCM records */
	uint8_t count;
	/** Version */
	uint8_t version;
	/** Reserved */
	uint8_t reserved1[2];
	/** Managers */
	struct xsigo_manager_id manager[8];
	/** Reserved */
	uint8_t reserved2[24];
} __attribute__ (( packed ));

/** A Xsigo MAD */
union xsigo_mad {
	/** Generic MAD */
	union ib_mad mad;
	/** Configuration manager request */
	struct xsigo_managers_request request;
	/** Configuration manager reply */
	struct xsigo_managers_reply reply;
} __attribute__ (( packed ));

/** An XSMP node identifier */
struct xsmp_node_id {
	/** Auxiliary ID (never used) */
	uint32_t aux;
	/** Port GUID */
	union ib_guid guid;
} __attribute__ (( packed ));

/** An XSMP message header */
struct xsmp_message_header {
	/** Message type */
	uint8_t type;
	/** Reason code */
	uint8_t code;
	/** Length */
	uint16_t len;
	/** Sequence number */
	uint32_t seq;
	/** Source node ID */
	struct xsmp_node_id src;
	/** Destination node ID */
	struct xsmp_node_id dst;
} __attribute__ (( packed ));

/** XSMP message types */
enum xsmp_message_type {
	/** Session message type */
	XSMP_TYPE_SESSION = 1,
	/** Virtual Ethernet message type */
	XSMP_TYPE_XVE = 6,
};

/** An XSMP session message */
struct xsmp_session_message {
	/** Message header */
	struct xsmp_message_header hdr;
	/** Message type */
	uint8_t type;
	/** Reason code */
	uint8_t code;
	/** Length (excluding message header) */
	uint16_t len;
	/** Operating system type */
	uint8_t os_type;
	/** Reserved */
	uint8_t reserved0;
	/** Resource types */
	uint16_t resources;
	/** Driver version */
	uint32_t driver_version;
	/** Required chassis version */
	uint32_t chassis_version;
	/** Boot flags */
	uint32_t boot;
	/** Firmware version */
	uint64_t firmware_version;
	/** Hardware version */
	uint32_t hardware_version;
	/** Vendor part ID */
	uint32_t vendor;
	/** Protocol version */
	uint32_t xsmp_version;
	/** Chassis name */
	char chassis[32];
	/** Session name */
	char session[32];
	/** Reserved */
	uint8_t reserved1[120];
} __attribute__ (( packed ));

/** XSMP session message types */
enum xsmp_session_type {
	/** Keepalive message */
	XSMP_SESSION_TYPE_HELLO = 1,
	/** Initial registration message */
	XSMP_SESSION_TYPE_REGISTER = 2,
	/** Registration confirmation message */
	XSMP_SESSION_TYPE_CONFIRM = 3,
	/** Registration rejection message */
	XSMP_SESSION_TYPE_REJECT = 4,
	/** Shutdown message */
	XSMP_SESSION_TYPE_SHUTDOWN = 5,
};

/** XSMP boot flags */
enum xsmp_session_boot {
	/** PXE boot */
	XSMP_BOOT_PXE = ( 1 << 0 ),
};

/** XSMP virtual Ethernet channel adapter parameters */
struct xsmp_xve_ca {
	/** Subnet prefix (little-endian) */
	union ib_guid prefix_le;
	/** Control queue pair number */
	uint32_t ctrl;
	/** Data queue pair number */
	uint32_t data;
	/** Partition key */
	uint16_t pkey;
	/** Queue key */
	uint16_t qkey;
} __attribute__ (( packed ));

/** XSMP virtual Ethernet MAC address */
struct xsmp_xve_mac {
	/** High 16 bits */
	uint16_t high;
	/** Low 32 bits */
	uint32_t low;
} __attribute__ (( packed ));

/** An XSMP virtual Ethernet message */
struct xsmp_xve_message {
	/** Message header */
	struct xsmp_message_header hdr;
	/** Message type */
	uint8_t type;
	/** Reason code */
	uint8_t code;
	/** Length (excluding message header) */
	uint16_t len;
	/** Update bitmask */
	uint32_t update;
	/** Resource identifier */
	union ib_guid resource;
	/** TCA GUID (little-endian) */
	union ib_guid guid_le;
	/** TCA LID */
	uint16_t lid;
	/** MAC address (little-endian) */
	struct xsmp_xve_mac mac_le;
	/** Rate */
	uint16_t rate;
	/** Administrative state (non-zero = "up") */
	uint16_t state;
	/** Encapsulation (apparently obsolete and unused) */
	uint16_t encap;
	/** MTU */
	uint16_t mtu;
	/** Installation flags (apparently obsolete and unused) */
	uint32_t install;
	/** Interface name */
	char name[16];
	/** Service level */
	uint16_t sl;
	/** Flow control enabled (apparently obsolete and unused) */
	uint16_t flow;
	/** Committed rate (in Mbps) */
	uint16_t committed_mbps;
	/** Peak rate (in Mbps) */
	uint16_t peak_mbps;
	/** Committed burst size (in bytes) */
	uint32_t committed_burst;
	/** Peak burst size (in bytes) */
	uint32_t peak_burst;
	/** VMware index */
	uint8_t vmware;
	/** Reserved */
	uint8_t reserved0;
	/** Multipath flags */
	uint16_t multipath;
	/** Multipath group name */
	char group[48];
	/** Link aggregation flag */
	uint8_t agg;
	/** Link aggregation policy */
	uint8_t policy;
	/** Network ID */
	uint32_t network;
	/** Mode */
	uint8_t mode;
	/** Uplink type */
	uint8_t uplink;
	/** Target channel adapter parameters */
	struct xsmp_xve_ca tca;
	/** Host channel adapter parameters */
	struct xsmp_xve_ca hca;
	/** Reserved */
	uint8_t reserved1[336];
} __attribute__ (( packed ));

/** XSMP virtual Ethernet message types */
enum xsmp_xve_type {
	/** Install virtual NIC */
	XSMP_XVE_TYPE_INSTALL = 1,
	/** Delete virtual NIC */
	XSMP_XVE_TYPE_DELETE = 2,
	/** Update virtual NIC */
	XSMP_XVE_TYPE_UPDATE = 3,
	/** Set operational state up */
	XSMP_XVE_TYPE_OPER_UP = 6,
	/** Set operational state down */
	XSMP_XVE_TYPE_OPER_DOWN = 7,
	/** Get operational state */
	XSMP_XVE_TYPE_OPER_REQ = 15,
	/** Virtual NIC is ready */
	XSMP_XVE_TYPE_READY = 20,
};

/** XSMP virtual Ethernet message codes */
enum xsmp_xve_code {
	/* Something went wrong */
	XSMP_XVE_CODE_ERROR = 0x84,
};

/** XSMP virtual Ethernet update bitmask */
enum xsmp_xve_update {
	/** Update MTU */
	XSMP_XVE_UPDATE_MTU = ( 1 << 2 ),
	/** Update administrative state */
	XSMP_XVE_UPDATE_STATE = ( 1 << 6 ),
	/** Update gateway to mark as down */
	XSMP_XVE_UPDATE_GW_DOWN = ( 1 << 30 ),
	/** Update gateway information */
	XSMP_XVE_UPDATE_GW_CHANGE = ( 1 << 31 ),
};

/** XSMP virtual Ethernet modes */
enum xsmp_xve_mode {
	/** Reliable Connected */
	XSMP_XVE_MODE_RC = 1,
	/** Unreliable Datagram */
	XSMP_XVE_MODE_UD = 2,
};

/** XSMP virtual Ethernet uplink types */
enum xsmp_xve_uplink {
	/** No uplink */
	XSMP_XVE_NO_UPLINK = 1,
	/** Has uplink */
	XSMP_XVE_UPLINK = 2,
};

/** An XSMP message */
union xsmp_message {
	/** Message header */
	struct xsmp_message_header hdr;
	/** Session message */
	struct xsmp_session_message sess;
	/** Virtual Ethernet message */
	struct xsmp_xve_message xve;
};

/** Delay between attempts to open the Infiniband device
 *
 * This is a policy decision.
 */
#define XSIGO_OPEN_RETRY_DELAY ( 2 * TICKS_PER_SEC )

/** Delay between unsuccessful discovery attempts
 *
 * This is a policy decision.
 */
#define XSIGO_DISCOVERY_FAILURE_DELAY ( 10 * TICKS_PER_SEC )

/** Delay between successful discovery attempts
 *
 * This is a policy decision.
 */
#define XSIGO_DISCOVERY_SUCCESS_DELAY ( 20 * TICKS_PER_SEC )

/** Delay between keepalive requests
 *
 * This is a policy decision.
 */
#define XSIGO_KEEPALIVE_INTERVAL ( 10 * TICKS_PER_SEC )

/** Maximum time to wait for a keepalive response
 *
 * This is a policy decision.
 */
#define XSIGO_KEEPALIVE_MAX_WAIT ( 2 * TICKS_PER_SEC )

#endif /* _IPXE_XSIGO_H */
