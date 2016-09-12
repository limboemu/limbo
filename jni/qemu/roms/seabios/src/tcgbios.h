#ifndef TCGBIOS_H
#define TCGBIOS_H

#include "types.h"

/* Define for section 12.3 */
#define TCG_PC_OK                       0x0
#define TCG_PC_TPMERROR                 0x1
#define TCG_PC_LOGOVERFLOW              0x2
#define TCG_PC_UNSUPPORTED              0x3

#define TPM_ALG_SHA                     0x4

#define TCG_MAGIC                       0x41504354L
#define TCG_VERSION_MAJOR               1
#define TCG_VERSION_MINOR               2

#define TPM_OK                          0x0
#define TPM_RET_BASE                    0x1
#define TCG_GENERAL_ERROR               (TPM_RET_BASE + 0x0)
#define TCG_TPM_IS_LOCKED               (TPM_RET_BASE + 0x1)
#define TCG_NO_RESPONSE                 (TPM_RET_BASE + 0x2)
#define TCG_INVALID_RESPONSE            (TPM_RET_BASE + 0x3)
#define TCG_INVALID_ACCESS_REQUEST      (TPM_RET_BASE + 0x4)
#define TCG_FIRMWARE_ERROR              (TPM_RET_BASE + 0x5)
#define TCG_INTEGRITY_CHECK_FAILED      (TPM_RET_BASE + 0x6)
#define TCG_INVALID_DEVICE_ID           (TPM_RET_BASE + 0x7)
#define TCG_INVALID_VENDOR_ID           (TPM_RET_BASE + 0x8)
#define TCG_UNABLE_TO_OPEN              (TPM_RET_BASE + 0x9)
#define TCG_UNABLE_TO_CLOSE             (TPM_RET_BASE + 0xa)
#define TCG_RESPONSE_TIMEOUT            (TPM_RET_BASE + 0xb)
#define TCG_INVALID_COM_REQUEST         (TPM_RET_BASE + 0xc)
#define TCG_INVALID_ADR_REQUEST         (TPM_RET_BASE + 0xd)
#define TCG_WRITE_BYTE_ERROR            (TPM_RET_BASE + 0xe)
#define TCG_READ_BYTE_ERROR             (TPM_RET_BASE + 0xf)
#define TCG_BLOCK_WRITE_TIMEOUT         (TPM_RET_BASE + 0x10)
#define TCG_CHAR_WRITE_TIMEOUT          (TPM_RET_BASE + 0x11)
#define TCG_CHAR_READ_TIMEOUT           (TPM_RET_BASE + 0x12)
#define TCG_BLOCK_READ_TIMEOUT          (TPM_RET_BASE + 0x13)
#define TCG_TRANSFER_ABORT              (TPM_RET_BASE + 0x14)
#define TCG_INVALID_DRV_FUNCTION        (TPM_RET_BASE + 0x15)
#define TCG_OUTPUT_BUFFER_TOO_SHORT     (TPM_RET_BASE + 0x16)
#define TCG_FATAL_COM_ERROR             (TPM_RET_BASE + 0x17)
#define TCG_INVALID_INPUT_PARA          (TPM_RET_BASE + 0x18)
#define TCG_TCG_COMMAND_ERROR           (TPM_RET_BASE + 0x19)
#define TCG_INTERFACE_SHUTDOWN          (TPM_RET_BASE + 0x20)
//define TCG_PC_UNSUPPORTED             (TPM_RET_BASE + 0x21)
#define TCG_PC_TPM_NOT_PRESENT          (TPM_RET_BASE + 0x22)
#define TCG_PC_TPM_DEACTIVATED          (TPM_RET_BASE + 0x23)


#define TPM_ORD_SelfTestFull             0x00000050
#define TPM_ORD_ForceClear               0x0000005d
#define TPM_ORD_GetCapability            0x00000065
#define TPM_ORD_PhysicalEnable           0x0000006f
#define TPM_ORD_PhysicalDisable          0x00000070
#define TPM_ORD_SetOwnerInstall          0x00000071
#define TPM_ORD_PhysicalSetDeactivated   0x00000072
#define TPM_ORD_Startup                  0x00000099
#define TPM_ORD_PhysicalPresence         0x4000000a
#define TPM_ORD_Extend                   0x00000014
#define TPM_ORD_SHA1Start                0x000000a0
#define TPM_ORD_SHA1Update               0x000000a1
#define TPM_ORD_SHA1Complete             0x000000a2
#define TSC_ORD_ResetEstablishmentBit    0x4000000b


#define TPM_ST_CLEAR                     0x1
#define TPM_ST_STATE                     0x2
#define TPM_ST_DEACTIVATED               0x3


/* TPM command error codes */
#define TPM_INVALID_POSTINIT             0x26
#define TPM_BAD_LOCALITY                 0x3d

/* TPM command tags */
#define TPM_TAG_RQU_CMD                  0x00c1

/* interrupt identifiers (al register) */
enum irq_ids {
    TCG_StatusCheck = 0,
    TCG_HashLogExtendEvent = 1,
    TCG_PassThroughToTPM = 2,
    TCG_ShutdownPreBootInterface = 3,
    TCG_HashLogEvent = 4,
    TCG_HashAll = 5,
    TCG_TSS = 6,
    TCG_CompactHashLogExtendEvent = 7,
};

/* event types: 10.4.1 / table 11 */
#define EV_POST_CODE             1
#define EV_SEPARATOR             4
#define EV_ACTION                5
#define EV_EVENT_TAG             6
#define EV_COMPACT_HASH         12
#define EV_IPL                  13
#define EV_IPL_PARTITION_DATA   14


#define STATUS_FLAG_SHUTDOWN        (1 << 0)

#define SHA1_BUFSIZE                20


struct iovec
{
    size_t length;
    void   *data;
};


/* Input and Output blocks for the TCG BIOS commands */

struct hleei_short
{
    u16   ipblength;
    u16   reserved;
    const void *hashdataptr;
    u32   hashdatalen;
    u32   pcrindex;
    const void *logdataptr;
    u32   logdatalen;
} PACKED;


struct hleei_long
{
    u16   ipblength;
    u16   reserved;
    void *hashdataptr;
    u32   hashdatalen;
    u32   pcrindex;
    u32   reserved2;
    void *logdataptr;
    u32   logdatalen;
} PACKED;


struct hleeo
{
    u16    opblength;
    u16    reserved;
    u32    eventnumber;
    u8     digest[SHA1_BUFSIZE];
} PACKED;


struct pttti
{
    u16    ipblength;
    u16    reserved;
    u16    opblength;
    u16    reserved2;
    u8     tpmopin[0];
} PACKED;


struct pttto
{
    u16    opblength;
    u16    reserved;
    u8     tpmopout[0];
};


struct hlei
{
    u16    ipblength;
    u16    reserved;
    const void  *hashdataptr;
    u32    hashdatalen;
    u32    pcrindex;
    u32    logeventtype;
    const void  *logdataptr;
    u32    logdatalen;
} PACKED;


struct hleo
{
    u16    opblength;
    u16    reserved;
    u32    eventnumber;
} PACKED;


struct hai
{
    u16    ipblength;
    u16    reserved;
    const void  *hashdataptr;
    u32    hashdatalen;
    u32    algorithmid;
} PACKED;


struct ti
{
    u16    ipblength;
    u16    reserved;
    u16    opblength;
    u16    reserved2;
    u8     tssoperandin[0];
} PACKED;


struct to
{
    u16    opblength;
    u16    reserved;
    u8     tssoperandout[0];
} PACKED;


struct pcpes
{
    u32    pcrindex;
    u32    eventtype;
    u8     digest[SHA1_BUFSIZE];
    u32    eventdatasize;
    u32    event;
} PACKED;

struct pcctes
{
    u32 eventid;
    u32 eventdatasize;
    u8  digest[SHA1_BUFSIZE];
} PACKED;

struct pcctes_romex
{
    u32 eventid;
    u32 eventdatasize;
    u16 reserved;
    u16 pfa;
    u8  digest[SHA1_BUFSIZE];
} PACKED;


#define TPM_REQ_HEADER \
    u16    tag; \
    u32    totlen; \
    u32    ordinal;

#define TPM_REQ_HEADER_SIZE  (sizeof(u16) + sizeof(u32) + sizeof(u32))

#define TPM_RSP_HEADER \
    u16    tag; \
    u32    totlen; \
    u32    errcode;

#define TPM_RSP_HEADER_SIZE  (sizeof(u16) + sizeof(u32) + sizeof(u32))

struct tpm_req_header {
    TPM_REQ_HEADER;
} PACKED;


struct tpm_rsp_header {
    TPM_RSP_HEADER;
} PACKED;


struct tpm_req_extend {
    TPM_REQ_HEADER
    u32    pcrindex;
    u8     digest[SHA1_BUFSIZE];
} PACKED;


struct tpm_rsp_extend {
    TPM_RSP_HEADER
    u8     digest[SHA1_BUFSIZE];
} PACKED;


struct tpm_req_getcap_perm_flags {
    TPM_REQ_HEADER
    u32    capArea;
    u32    subCapSize;
    u32    subCap;
} PACKED;


struct tpm_permanent_flags {
    u16    tag;
    u8     flags[20];
} PACKED;


enum permFlagsIndex {
    PERM_FLAG_IDX_DISABLE = 0,
    PERM_FLAG_IDX_OWNERSHIP,
    PERM_FLAG_IDX_DEACTIVATED,
    PERM_FLAG_IDX_READPUBEK,
    PERM_FLAG_IDX_DISABLEOWNERCLEAR,
    PERM_FLAG_IDX_ALLOW_MAINTENANCE,
    PERM_FLAG_IDX_PHYSICAL_PRESENCE_LIFETIME_LOCK,
    PERM_FLAG_IDX_PHYSICAL_PRESENCE_HW_ENABLE,
};


struct tpm_res_getcap_perm_flags {
    TPM_RSP_HEADER
    u32    size;
    struct tpm_permanent_flags perm_flags;
} PACKED;


struct tpm_res_getcap_ownerauth {
    TPM_RSP_HEADER
    u32    size;
    u8     flag;
} PACKED;


struct tpm_res_getcap_timeouts {
    TPM_RSP_HEADER
    u32    size;
    u32    timeouts[4];
} PACKED;


struct tpm_res_getcap_durations {
    TPM_RSP_HEADER
    u32    size;
    u32    durations[3];
} PACKED;


struct tpm_res_sha1start {
    TPM_RSP_HEADER
    u32    max_num_bytes;
} PACKED;


struct tpm_res_sha1complete {
    TPM_RSP_HEADER
    u8     hash[20];
} PACKED;

struct pttti_extend {
    struct pttti pttti;
    struct tpm_req_extend req;
} PACKED;


struct pttto_extend {
    struct pttto pttto;
    struct tpm_rsp_extend rsp;
} PACKED;


enum ipltype {
    IPL_BCV = 0,
    IPL_EL_TORITO_1,
    IPL_EL_TORITO_2
};


struct bregs;
void tpm_interrupt_handler32(struct bregs *regs);

void tpm_setup(void);
void tpm_prepboot(void);
void tpm_s3_resume(void);
u32 tpm_add_bcv(u32 bootdrv, const u8 *addr, u32 length);
u32 tpm_add_cdrom(u32 bootdrv, const u8 *addr, u32 length);
u32 tpm_add_cdrom_catalog(const u8 *addr, u32 length);
u32 tpm_option_rom(const void *addr, u32 len);

#endif /* TCGBIOS_H */
