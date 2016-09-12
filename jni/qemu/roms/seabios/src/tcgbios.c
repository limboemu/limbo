//  Implementation of the TCG BIOS extension according to the specification
//  described in specs found at
//  http://www.trustedcomputinggroup.org/resources/pc_client_work_group_specific_implementation_specification_for_conventional_bios
//
//  Copyright (C) 2006-2011, 2014, 2015 IBM Corporation
//
//  Authors:
//      Stefan Berger <stefanb@linux.vnet.ibm.com>
//
// This file may be distributed under the terms of the GNU LGPLv3 license.


#include "config.h"

#include "types.h"
#include "byteorder.h" // cpu_to_*
#include "hw/tpm_drivers.h" // tpm_drivers[]
#include "farptr.h" // MAKE_FLATPTR
#include "string.h" // checksum
#include "tcgbios.h"// tpm_*, prototypes
#include "util.h" // printf, get_keystroke
#include "output.h" // dprintf
#include "std/acpi.h"  // RSDP_SIGNATURE, rsdt_descriptor
#include "bregs.h" // struct bregs
#include "sha1.h" // sha1
#include "fw/paravirt.h" // runningOnXen
#include "std/smbios.h"

static const u8 Startup_ST_CLEAR[] = { 0x00, TPM_ST_CLEAR };
static const u8 Startup_ST_STATE[] = { 0x00, TPM_ST_STATE };

static const u8 PhysicalPresence_CMD_ENABLE[]  = { 0x00, 0x20 };
static const u8 PhysicalPresence_CMD_DISABLE[] = { 0x01, 0x00 };
static const u8 PhysicalPresence_PRESENT[]     = { 0x00, 0x08 };
static const u8 PhysicalPresence_NOT_PRESENT_LOCK[] = { 0x00, 0x14 };

static const u8 CommandFlag_FALSE[1] = { 0x00 };
static const u8 CommandFlag_TRUE[1]  = { 0x01 };

static const u8 GetCapability_Permanent_Flags[] = {
    0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x04,
    0x00, 0x00, 0x01, 0x08
};

static const u8 GetCapability_OwnerAuth[] = {
    0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x04,
    0x00, 0x00, 0x01, 0x11
};

static const u8 GetCapability_Timeouts[] = {
    0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x04,
    0x00, 0x00, 0x01, 0x15
};

static const u8 GetCapability_Durations[] = {
    0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x04,
    0x00, 0x00, 0x01, 0x20
};

static u8 evt_separator[] = {0xff,0xff,0xff,0xff};


#define RSDP_CAST(ptr)   ((struct rsdp_descriptor *)ptr)

/* local function prototypes */

static u32 tpm_calling_int19h(void);
static u32 tpm_add_event_separators(void);
static u32 tpm_start_option_rom_scan(void);
static u32 tpm_smbios_measure(void);

/* helper functions */

static inline void *input_buf32(struct bregs *regs)
{
    return MAKE_FLATPTR(regs->es, regs->di);
}

static inline void *output_buf32(struct bregs *regs)
{
    return MAKE_FLATPTR(regs->ds, regs->si);
}


typedef struct {
    u8            tpm_probed:1;
    u8            tpm_found:1;
    u8            tpm_working:1;
    u8            if_shutdown:1;
    u8            tpm_driver_to_use:4;
} tpm_state_t;


static tpm_state_t tpm_state = {
    .tpm_driver_to_use = TPM_INVALID_DRIVER,
};


/********************************************************
  Extensions for TCG-enabled BIOS
 *******************************************************/


static u32
is_tpm_present(void)
{
    u32 rc = 0;
    unsigned int i;

    for (i = 0; i < TPM_NUM_DRIVERS; i++) {
        struct tpm_driver *td = &tpm_drivers[i];
        if (td->probe() != 0) {
            td->init();
            tpm_state.tpm_driver_to_use = i;
            rc = 1;
            break;
        }
    }

    return rc;
}

static void
probe_tpm(void)
{
    if (!tpm_state.tpm_probed) {
        tpm_state.tpm_probed = 1;
        tpm_state.tpm_found = (is_tpm_present() != 0);
        tpm_state.tpm_working = tpm_state.tpm_found;
    }
}

static int
has_working_tpm(void)
{
    probe_tpm();

    return tpm_state.tpm_working;
}

static struct tcpa_descriptor_rev2 *
find_tcpa_by_rsdp(struct rsdp_descriptor *rsdp)
{
    u32 ctr = 0;
    struct tcpa_descriptor_rev2 *tcpa = NULL;
    struct rsdt_descriptor *rsdt;
    u32 length;
    u16 off;

    rsdt   = (struct rsdt_descriptor *)rsdp->rsdt_physical_address;
    if (!rsdt)
        return NULL;

    length = rsdt->length;
    off = offsetof(struct rsdt_descriptor, entry);

    while ((off + sizeof(rsdt->entry[0])) <= length) {
        /* try all pointers to structures */
        tcpa = (struct tcpa_descriptor_rev2 *)(int)rsdt->entry[ctr];

        /* valid TCPA ACPI table ? */
        if (tcpa->signature == TCPA_SIGNATURE &&
            checksum((u8 *)tcpa, tcpa->length) == 0)
            break;

        tcpa = NULL;
        off += sizeof(rsdt->entry[0]);
        ctr++;
    }

    return tcpa;
}


static struct tcpa_descriptor_rev2 *
find_tcpa_table(void)
{
    struct tcpa_descriptor_rev2 *tcpa = NULL;
    struct rsdp_descriptor *rsdp = RsdpAddr;

    if (rsdp)
        tcpa = find_tcpa_by_rsdp(rsdp);
    else
        tpm_state.if_shutdown = 1;

    if (!rsdp)
        dprintf(DEBUG_tcg,
                "TCGBIOS: RSDP was NOT found! -- Disabling interface.\n");
    else if (!tcpa)
        dprintf(DEBUG_tcg, "TCGBIOS: TCPA ACPI was NOT found!\n");

    return tcpa;
}


static u8 *
get_lasa_base_ptr(u32 *log_area_minimum_length)
{
    u8 *log_area_start_address = 0;
    struct tcpa_descriptor_rev2 *tcpa = find_tcpa_table();

    if (tcpa) {
        log_area_start_address = (u8 *)(long)tcpa->log_area_start_address;
        if (log_area_minimum_length)
            *log_area_minimum_length = tcpa->log_area_minimum_length;
    }

    return log_area_start_address;
}


/* clear the ACPI log */
static void
reset_acpi_log(void)
{
    u32 log_area_minimum_length;
    u8 *log_area_start_address = get_lasa_base_ptr(&log_area_minimum_length);

    if (log_area_start_address)
        memset(log_area_start_address, 0x0, log_area_minimum_length);
}


/*
   initialize the TCPA ACPI subsystem; find the ACPI tables and determine
   where the TCPA table is.
 */
static void
tpm_acpi_init(void)
{
    tpm_state.if_shutdown = 0;
    tpm_state.tpm_probed = 0;
    tpm_state.tpm_found = 0;
    tpm_state.tpm_working = 0;

    if (!has_working_tpm()) {
        tpm_state.if_shutdown = 1;
        return;
    }

    reset_acpi_log();
}


static u32
transmit(u8 locty, const struct iovec iovec[],
         u8 *respbuffer, u32 *respbufferlen,
         enum tpmDurationType to_t)
{
    u32 rc = 0;
    u32 irc;
    struct tpm_driver *td;
    unsigned int i;

    if (tpm_state.tpm_driver_to_use == TPM_INVALID_DRIVER)
        return TCG_FATAL_COM_ERROR;

    td = &tpm_drivers[tpm_state.tpm_driver_to_use];

    irc = td->activate(locty);
    if (irc != 0) {
        /* tpm could not be activated */
        return TCG_FATAL_COM_ERROR;
    }

    for (i = 0; iovec[i].length; i++) {
        irc = td->senddata(iovec[i].data,
                           iovec[i].length);
        if (irc != 0)
            return TCG_FATAL_COM_ERROR;
    }

    irc = td->waitdatavalid();
    if (irc != 0)
        return TCG_FATAL_COM_ERROR;

    irc = td->waitrespready(to_t);
    if (irc != 0)
        return TCG_FATAL_COM_ERROR;

    irc = td->readresp(respbuffer,
                       respbufferlen);
    if (irc != 0)
        return TCG_FATAL_COM_ERROR;

    td->ready();

    return rc;
}


/*
 * Send a TPM command with the given ordinal. Append the given buffer
 * containing all data in network byte order to the command (this is
 * the custom part per command) and expect a response of the given size.
 * If a buffer is provided, the response will be copied into it.
 */
static u32
build_and_send_cmd_od(u8 locty, u32 ordinal, const u8 *append, u32 append_size,
                      u8 *resbuffer, u32 return_size, u32 *returnCode,
                      const u8 *otherdata, u32 otherdata_size,
                      enum tpmDurationType to_t)
{
#define MAX_APPEND_SIZE   sizeof(GetCapability_Timeouts)
#define MAX_RESPONSE_SIZE sizeof(struct tpm_res_getcap_perm_flags)
    u32 rc;
    u8 ibuffer[TPM_REQ_HEADER_SIZE + MAX_APPEND_SIZE];
    u8 obuffer[MAX_RESPONSE_SIZE];
    struct tpm_req_header *trqh = (struct tpm_req_header *)ibuffer;
    struct tpm_rsp_header *trsh = (struct tpm_rsp_header *)obuffer;
    struct iovec iovec[3];
    u32 obuffer_len = sizeof(obuffer);
    u32 idx = 1;

    if (append_size > MAX_APPEND_SIZE ||
        return_size > MAX_RESPONSE_SIZE) {
        dprintf(DEBUG_tcg, "TCGBIOS: size of requested buffers too big.");
        return TCG_FIRMWARE_ERROR;
    }

    iovec[0].data   = trqh;
    iovec[0].length = TPM_REQ_HEADER_SIZE + append_size;

    if (otherdata) {
        iovec[1].data   = (void *)otherdata;
        iovec[1].length = otherdata_size;
        idx = 2;
    }

    iovec[idx].data   = NULL;
    iovec[idx].length = 0;

    memset(ibuffer, 0x0, sizeof(ibuffer));
    memset(obuffer, 0x0, sizeof(obuffer));

    trqh->tag     = cpu_to_be16(TPM_TAG_RQU_CMD);
    trqh->totlen  = cpu_to_be32(TPM_REQ_HEADER_SIZE + append_size +
                                otherdata_size);
    trqh->ordinal = cpu_to_be32(ordinal);

    if (append_size)
        memcpy((char *)trqh + sizeof(*trqh),
               append, append_size);

    rc = transmit(locty, iovec, obuffer, &obuffer_len, to_t);
    if (rc)
        return rc;

    *returnCode = be32_to_cpu(trsh->errcode);

    if (resbuffer)
        memcpy(resbuffer, trsh, return_size);

    return 0;
}


static u32
build_and_send_cmd(u8 locty, u32 ordinal, const u8 *append, u32 append_size,
                   u8 *resbuffer, u32 return_size, u32 *returnCode,
                   enum tpmDurationType to_t)
{
    return build_and_send_cmd_od(locty, ordinal, append, append_size,
                                 resbuffer, return_size, returnCode,
                                 NULL, 0, to_t);
}


static u32
determine_timeouts(void)
{
    u32 rc;
    u32 returnCode;
    struct tpm_res_getcap_timeouts timeouts;
    struct tpm_res_getcap_durations durations;
    struct tpm_driver *td = &tpm_drivers[tpm_state.tpm_driver_to_use];
    u32 i;

    rc = build_and_send_cmd(0, TPM_ORD_GetCapability,
                            GetCapability_Timeouts,
                            sizeof(GetCapability_Timeouts),
                            (u8 *)&timeouts, sizeof(timeouts),
                            &returnCode, TPM_DURATION_TYPE_SHORT);

    dprintf(DEBUG_tcg, "TCGBIOS: Return code from TPM_GetCapability(Timeouts)"
            " = 0x%08x\n", returnCode);

    if (rc || returnCode)
        goto err_exit;

    rc = build_and_send_cmd(0, TPM_ORD_GetCapability,
                            GetCapability_Durations,
                            sizeof(GetCapability_Durations),
                            (u8 *)&durations, sizeof(durations),
                            &returnCode, TPM_DURATION_TYPE_SHORT);

    dprintf(DEBUG_tcg, "TCGBIOS: Return code from TPM_GetCapability(Durations)"
            " = 0x%08x\n", returnCode);

    if (rc || returnCode)
        goto err_exit;

    for (i = 0; i < 3; i++)
        durations.durations[i] = be32_to_cpu(durations.durations[i]);

    for (i = 0; i < 4; i++)
        timeouts.timeouts[i] = be32_to_cpu(timeouts.timeouts[i]);

    dprintf(DEBUG_tcg, "TCGBIOS: timeouts: %u %u %u %u\n",
            timeouts.timeouts[0],
            timeouts.timeouts[1],
            timeouts.timeouts[2],
            timeouts.timeouts[3]);

    dprintf(DEBUG_tcg, "TCGBIOS: durations: %u %u %u\n",
            durations.durations[0],
            durations.durations[1],
            durations.durations[2]);


    td->set_timeouts(timeouts.timeouts, durations.durations);

    return 0;

err_exit:
    dprintf(DEBUG_tcg, "TCGBIOS: TPM malfunctioning (line %d).\n", __LINE__);

    tpm_state.tpm_working = 0;
    if (rc)
        return rc;
    return TCG_TCG_COMMAND_ERROR;
}


static u32
tpm_startup(void)
{
    u32 rc;
    u32 returnCode;

    if (!has_working_tpm())
        return TCG_GENERAL_ERROR;

    dprintf(DEBUG_tcg, "TCGBIOS: Starting with TPM_Startup(ST_CLEAR)\n");
    rc = build_and_send_cmd(0, TPM_ORD_Startup,
                            Startup_ST_CLEAR, sizeof(Startup_ST_CLEAR),
                            NULL, 0, &returnCode, TPM_DURATION_TYPE_SHORT);

    dprintf(DEBUG_tcg, "Return code from TPM_Startup = 0x%08x\n",
            returnCode);

    if (CONFIG_COREBOOT) {
        /* with other firmware on the system the TPM may already have been
         * initialized
         */
        if (returnCode == TPM_INVALID_POSTINIT)
            returnCode = 0;
    }

    if (rc || returnCode)
        goto err_exit;

    rc = build_and_send_cmd(0, TPM_ORD_SelfTestFull, NULL, 0,
                            NULL, 0, &returnCode, TPM_DURATION_TYPE_LONG);

    dprintf(DEBUG_tcg, "Return code from TPM_SelfTestFull = 0x%08x\n",
            returnCode);

    if (rc || returnCode)
        goto err_exit;

    rc = build_and_send_cmd(3, TSC_ORD_ResetEstablishmentBit, NULL, 0,
                            NULL, 0, &returnCode, TPM_DURATION_TYPE_SHORT);

    dprintf(DEBUG_tcg, "Return code from TSC_ResetEstablishmentBit = 0x%08x\n",
            returnCode);

    if (rc || (returnCode != 0 && returnCode != TPM_BAD_LOCALITY))
        goto err_exit;

    rc = determine_timeouts();
    if (rc)
        goto err_exit;

    rc = tpm_smbios_measure();
    if (rc)
        goto err_exit;

    rc = tpm_start_option_rom_scan();
    if (rc)
        goto err_exit;

    return 0;

err_exit:
    dprintf(DEBUG_tcg, "TCGBIOS: TPM malfunctioning (line %d).\n", __LINE__);

    tpm_state.tpm_working = 0;
    if (rc)
        return rc;
    return TCG_TCG_COMMAND_ERROR;
}


void
tpm_setup(void)
{
    if (!CONFIG_TCGBIOS)
        return;

    tpm_acpi_init();
    if (runningOnXen())
        return;

    tpm_startup();
}


void
tpm_prepboot(void)
{
    u32 rc;
    u32 returnCode;

    if (!CONFIG_TCGBIOS)
        return;

    if (!has_working_tpm())
        return;

    rc = build_and_send_cmd(0, TPM_ORD_PhysicalPresence,
                            PhysicalPresence_CMD_ENABLE,
                            sizeof(PhysicalPresence_CMD_ENABLE),
                            NULL, 0, &returnCode, TPM_DURATION_TYPE_SHORT);
    if (rc || returnCode)
        goto err_exit;

    rc = build_and_send_cmd(0, TPM_ORD_PhysicalPresence,
                            PhysicalPresence_NOT_PRESENT_LOCK,
                            sizeof(PhysicalPresence_NOT_PRESENT_LOCK),
                            NULL, 0, &returnCode, TPM_DURATION_TYPE_SHORT);
    if (rc || returnCode)
        goto err_exit;

    rc = tpm_calling_int19h();
    if (rc)
        goto err_exit;

    rc = tpm_add_event_separators();
    if (rc)
        goto err_exit;

    return;

err_exit:
    dprintf(DEBUG_tcg, "TCGBIOS: TPM malfunctioning (line %d).\n", __LINE__);

    tpm_state.tpm_working = 0;
}

static int
is_valid_pcpes(struct pcpes *pcpes)
{
    return (pcpes->eventtype != 0);
}


static u8 *
get_lasa_last_ptr(u16 *entry_count, u8 **log_area_start_address_next)
{
    struct pcpes *pcpes;
    u32 log_area_minimum_length = 0;
    u8 *log_area_start_address_base =
        get_lasa_base_ptr(&log_area_minimum_length);
    u8 *log_area_start_address_last = NULL;
    u8 *end = log_area_start_address_base + log_area_minimum_length;
    u32 size;

    if (entry_count)
        *entry_count = 0;

    if (!log_area_start_address_base)
        return NULL;

    while (log_area_start_address_base < end) {
        pcpes = (struct pcpes *)log_area_start_address_base;
        if (!is_valid_pcpes(pcpes))
            break;
        if (entry_count)
            (*entry_count)++;
        size = pcpes->eventdatasize + offsetof(struct pcpes, event);
        log_area_start_address_last = log_area_start_address_base;
        log_area_start_address_base += size;
    }

    if (log_area_start_address_next)
        *log_area_start_address_next = log_area_start_address_base;

    return log_area_start_address_last;
}


static u32
tpm_sha1_calc(const u8 *data, u32 length, u8 *hash)
{
    u32 rc;
    u32 returnCode;
    struct tpm_res_sha1start start;
    struct tpm_res_sha1complete complete;
    u32 blocks = length / 64;
    u32 rest = length & 0x3f;
    u32 numbytes, numbytes_no;
    u32 offset = 0;

    rc = build_and_send_cmd(0, TPM_ORD_SHA1Start,
                            NULL, 0,
                            (u8 *)&start, sizeof(start),
                            &returnCode, TPM_DURATION_TYPE_SHORT);

    if (rc || returnCode)
        goto err_exit;

    while (blocks > 0) {

        numbytes = be32_to_cpu(start.max_num_bytes);
        if (numbytes > blocks * 64)
             numbytes = blocks * 64;

        numbytes_no = cpu_to_be32(numbytes);

        rc = build_and_send_cmd_od(0, TPM_ORD_SHA1Update,
                                   (u8 *)&numbytes_no, sizeof(numbytes_no),
                                   NULL, 0, &returnCode,
                                   &data[offset], numbytes,
                                   TPM_DURATION_TYPE_SHORT);

        if (rc || returnCode)
            goto err_exit;

        offset += numbytes;
        blocks -= (numbytes / 64);
    }

    numbytes_no = cpu_to_be32(rest);

    rc = build_and_send_cmd_od(0, TPM_ORD_SHA1Complete,
                              (u8 *)&numbytes_no, sizeof(numbytes_no),
                              (u8 *)&complete, sizeof(complete),
                              &returnCode,
                              &data[offset], rest, TPM_DURATION_TYPE_SHORT);

    if (rc || returnCode)
        goto err_exit;

    memcpy(hash, complete.hash, sizeof(complete.hash));

    return 0;

err_exit:
    dprintf(DEBUG_tcg, "TCGBIOS: TPM SHA1 malfunctioning.\n");

    tpm_state.tpm_working = 0;
    if (rc)
        return rc;
    return TCG_TCG_COMMAND_ERROR;
}


static u32
sha1_calc(const u8 *data, u32 length, u8 *hash)
{
    if (length < tpm_drivers[tpm_state.tpm_driver_to_use].sha1threshold)
        return tpm_sha1_calc(data, length, hash);

    return sha1(data, length, hash);
}


/*
 * Extend the ACPI log with the given entry by copying the
 * entry data into the log.
 * Input
 *  Pointer to the structure to be copied into the log
 *
 * Output:
 *  lower 16 bits of return code contain entry number
 *  if entry number is '0', then upper 16 bits contain error code.
 */
static u32
tpm_extend_acpi_log(void *entry_ptr, u16 *entry_count)
{
    u32 log_area_minimum_length, size;
    u8 *log_area_start_address_base =
        get_lasa_base_ptr(&log_area_minimum_length);
    u8 *log_area_start_address_next = NULL;
    struct pcpes *pcpes = (struct pcpes *)entry_ptr;

    get_lasa_last_ptr(entry_count, &log_area_start_address_next);

    dprintf(DEBUG_tcg, "TCGBIOS: LASA_BASE = %p, LASA_NEXT = %p\n",
            log_area_start_address_base, log_area_start_address_next);

    if (log_area_start_address_next == NULL || log_area_minimum_length == 0)
        return TCG_PC_LOGOVERFLOW;

    size = pcpes->eventdatasize + offsetof(struct pcpes, event);

    if ((log_area_start_address_next + size - log_area_start_address_base) >
        log_area_minimum_length) {
        dprintf(DEBUG_tcg, "TCGBIOS: LOG OVERFLOW: size = %d\n", size);
        return TCG_PC_LOGOVERFLOW;
    }

    memcpy(log_area_start_address_next, entry_ptr, size);

    (*entry_count)++;

    return 0;
}


static u32
is_preboot_if_shutdown(void)
{
    return tpm_state.if_shutdown;
}


static u32
shutdown_preboot_interface(void)
{
    u32 rc = 0;

    if (!is_preboot_if_shutdown()) {
        tpm_state.if_shutdown = 1;
    } else {
        rc = TCG_INTERFACE_SHUTDOWN;
    }

    return rc;
}


static void
tpm_shutdown(void)
{
    reset_acpi_log();
    shutdown_preboot_interface();
}


static u32
pass_through_to_tpm(struct pttti *pttti, struct pttto *pttto)
{
    u32 rc = 0;
    u32 resbuflen = 0;
    struct tpm_req_header *trh;
    u8 locty = 0;
    struct iovec iovec[2];
    const u32 *tmp;

    if (is_preboot_if_shutdown()) {
        rc = TCG_INTERFACE_SHUTDOWN;
        goto err_exit;
    }

    trh = (struct tpm_req_header *)pttti->tpmopin;

    if (pttti->ipblength < sizeof(struct pttti) + TPM_REQ_HEADER_SIZE ||
        pttti->opblength < sizeof(struct pttto) ||
        be32_to_cpu(trh->totlen)  + sizeof(struct pttti) > pttti->ipblength ) {
        rc = TCG_INVALID_INPUT_PARA;
        goto err_exit;
    }

    resbuflen = pttti->opblength - offsetof(struct pttto, tpmopout);

    iovec[0].data   = pttti->tpmopin;
    tmp = (const u32 *)&((u8 *)iovec[0].data)[2];
    iovec[0].length = cpu_to_be32(*tmp);

    iovec[1].data   = NULL;
    iovec[1].length = 0;

    rc = transmit(locty, iovec, pttto->tpmopout, &resbuflen,
                  TPM_DURATION_TYPE_LONG /* worst case */);
    if (rc)
        goto err_exit;

    pttto->opblength = offsetof(struct pttto, tpmopout) + resbuflen;
    pttto->reserved  = 0;

err_exit:
    if (rc != 0) {
        pttto->opblength = 4;
        pttto->reserved = 0;
    }

    return rc;
}


static u32
tpm_extend(u8 *hash, u32 pcrindex)
{
    u32 rc;
    struct pttto_extend pttto;
    struct pttti_extend pttti = {
        .pttti = {
            .ipblength = sizeof(struct pttti_extend),
            .opblength = sizeof(struct pttto_extend),
        },
        .req = {
            .tag      = cpu_to_be16(0xc1),
            .totlen   = cpu_to_be32(sizeof(pttti.req)),
            .ordinal  = cpu_to_be32(TPM_ORD_Extend),
            .pcrindex = cpu_to_be32(pcrindex),
        },
    };

    memcpy(pttti.req.digest, hash, sizeof(pttti.req.digest));

    rc = pass_through_to_tpm(&pttti.pttti, &pttto.pttto);

    if (rc == 0) {
        if (pttto.pttto.opblength < TPM_RSP_HEADER_SIZE ||
            pttto.pttto.opblength !=
                sizeof(struct pttto) + be32_to_cpu(pttto.rsp.totlen) ||
            be16_to_cpu(pttto.rsp.tag) != 0xc4) {
            rc = TCG_FATAL_COM_ERROR;
        }
    }

    if (rc)
        tpm_shutdown();

    return rc;
}


static u32
hash_all(const struct hai *hai, u8 *hash)
{
    if (is_preboot_if_shutdown() != 0)
        return TCG_INTERFACE_SHUTDOWN;

    if (hai->ipblength != sizeof(struct hai) ||
        hai->hashdataptr == 0 ||
        hai->hashdatalen == 0 ||
        hai->algorithmid != TPM_ALG_SHA)
        return TCG_INVALID_INPUT_PARA;

    return sha1_calc((const u8 *)hai->hashdataptr, hai->hashdatalen, hash);
}


static u32
hash_log_event(const struct hlei *hlei, struct hleo *hleo)
{
    u32 rc = 0;
    u16 size;
    struct pcpes *pcpes;
    u16 entry_count;

    if (is_preboot_if_shutdown() != 0) {
        rc = TCG_INTERFACE_SHUTDOWN;
        goto err_exit;
    }

    size = hlei->ipblength;
    if (size != sizeof(*hlei)) {
        rc = TCG_INVALID_INPUT_PARA;
        goto err_exit;
    }

    pcpes = (struct pcpes *)hlei->logdataptr;

    if (pcpes->pcrindex >= 24 ||
        pcpes->pcrindex  != hlei->pcrindex ||
        pcpes->eventtype != hlei->logeventtype) {
        rc = TCG_INVALID_INPUT_PARA;
        goto err_exit;
    }

    if ((hlei->hashdataptr != 0) && (hlei->hashdatalen != 0)) {
        rc = sha1_calc((const u8 *)hlei->hashdataptr,
                       hlei->hashdatalen, pcpes->digest);
        if (rc)
            return rc;
    }

    rc = tpm_extend_acpi_log((void *)hlei->logdataptr, &entry_count);
    if (rc)
        goto err_exit;

    /* updating the log was fine */
    hleo->opblength = sizeof(struct hleo);
    hleo->reserved  = 0;
    hleo->eventnumber = entry_count;

err_exit:
    if (rc != 0) {
        hleo->opblength = 2;
        hleo->reserved = 0;
    }

    return rc;
}


static u32
hash_log_extend_event(const struct hleei_short *hleei_s, struct hleeo *hleeo)
{
    u32 rc = 0;
    struct hleo hleo;
    struct hleei_long *hleei_l = (struct hleei_long *)hleei_s;
    const void *logdataptr;
    u32 logdatalen;
    struct pcpes *pcpes;

    /* short or long version? */
    switch (hleei_s->ipblength) {
    case sizeof(struct hleei_short):
        /* short */
        logdataptr = hleei_s->logdataptr;
        logdatalen = hleei_s->logdatalen;
    break;

    case sizeof(struct hleei_long):
        /* long */
        logdataptr = hleei_l->logdataptr;
        logdatalen = hleei_l->logdatalen;
    break;

    default:
        /* bad input block */
        rc = TCG_INVALID_INPUT_PARA;
        goto err_exit;
    }

    pcpes = (struct pcpes *)logdataptr;

    struct hlei hlei = {
        .ipblength   = sizeof(hlei),
        .hashdataptr = hleei_s->hashdataptr,
        .hashdatalen = hleei_s->hashdatalen,
        .pcrindex    = hleei_s->pcrindex,
        .logeventtype= pcpes->eventtype,
        .logdataptr  = logdataptr,
        .logdatalen  = logdatalen,
    };

    rc = hash_log_event(&hlei, &hleo);
    if (rc)
        goto err_exit;

    hleeo->opblength = sizeof(struct hleeo);
    hleeo->reserved  = 0;
    hleeo->eventnumber = hleo.eventnumber;

    rc = tpm_extend(pcpes->digest, hleei_s->pcrindex);

err_exit:
    if (rc != 0) {
        hleeo->opblength = 4;
        hleeo->reserved  = 0;
    }

    return rc;

}


static u32
tss(struct ti *ti, struct to *to)
{
    u32 rc = 0;

    if (is_preboot_if_shutdown() == 0) {
        rc = TCG_PC_UNSUPPORTED;
    } else {
        rc = TCG_INTERFACE_SHUTDOWN;
    }

    to->opblength = sizeof(struct to);
    to->reserved  = 0;

    return rc;
}


static u32
compact_hash_log_extend_event(u8 *buffer,
                              u32 info,
                              u32 length,
                              u32 pcrindex,
                              u32 *edx_ptr)
{
    u32 rc = 0;
    struct hleeo hleeo;
    struct pcpes pcpes = {
        .pcrindex      = pcrindex,
        .eventtype     = EV_COMPACT_HASH,
        .eventdatasize = sizeof(info),
        .event         = info,
    };
    struct hleei_short hleei = {
        .ipblength   = sizeof(hleei),
        .hashdataptr = buffer,
        .hashdatalen = length,
        .pcrindex    = pcrindex,
        .logdataptr  = &pcpes,
        .logdatalen  = sizeof(pcpes),
    };

    rc = hash_log_extend_event(&hleei, &hleeo);
    if (rc == 0)
        *edx_ptr = hleeo.eventnumber;

    return rc;
}


void VISIBLE32FLAT
tpm_interrupt_handler32(struct bregs *regs)
{
    if (!CONFIG_TCGBIOS)
        return;

    set_cf(regs, 0);

    if (!has_working_tpm()) {
        regs->eax = TCG_GENERAL_ERROR;
        return;
    }

    switch ((enum irq_ids)regs->al) {
    case TCG_StatusCheck:
        if (is_tpm_present() == 0) {
            /* no TPM available */
            regs->eax = TCG_PC_TPM_NOT_PRESENT;
        } else {
            regs->eax = 0;
            regs->ebx = TCG_MAGIC;
            regs->ch = TCG_VERSION_MAJOR;
            regs->cl = TCG_VERSION_MINOR;
            regs->edx = 0x0;
            regs->esi = (u32)get_lasa_base_ptr(NULL);
            regs->edi =
                  (u32)get_lasa_last_ptr(NULL, NULL);
        }
        break;

    case TCG_HashLogExtendEvent:
        regs->eax =
            hash_log_extend_event(
                  (struct hleei_short *)input_buf32(regs),
                  (struct hleeo *)output_buf32(regs));
        break;

    case TCG_PassThroughToTPM:
        regs->eax =
            pass_through_to_tpm((struct pttti *)input_buf32(regs),
                                (struct pttto *)output_buf32(regs));
        break;

    case TCG_ShutdownPreBootInterface:
        regs->eax = shutdown_preboot_interface();
        break;

    case TCG_HashLogEvent:
        regs->eax = hash_log_event((struct hlei*)input_buf32(regs),
                                   (struct hleo*)output_buf32(regs));
        break;

    case TCG_HashAll:
        regs->eax =
            hash_all((struct hai*)input_buf32(regs),
                     (u8 *)output_buf32(regs));
        break;

    case TCG_TSS:
        regs->eax = tss((struct ti*)input_buf32(regs),
                    (struct to*)output_buf32(regs));
        break;

    case TCG_CompactHashLogExtendEvent:
        regs->eax =
          compact_hash_log_extend_event((u8 *)input_buf32(regs),
                                        regs->esi,
                                        regs->ecx,
                                        regs->edx,
                                        &regs->edx);
        break;

    default:
        set_cf(regs, 1);
    }

    return;
}

/*
 * Add a measurement to the log; the data at data_seg:data/length are
 * appended to the TCG_PCClientPCREventStruct
 *
 * Input parameters:
 *  pcrIndex   : which PCR to extend
 *  event_type : type of event; specs section on 'Event Types'
 *  info       : pointer to info (e.g., string) to be added to log as-is
 *  info_length: length of the info
 *  data       : pointer to the data (i.e., string) to be added to the log
 *  data_length: length of the data
 */
static u32
tpm_add_measurement_to_log(u32 pcrIndex, u32 event_type,
                           const char *info, u32 info_length,
                           const u8 *data, u32 data_length)
{
    u32 rc = 0;
    struct hleeo hleeo;
    u8 _pcpes[offsetof(struct pcpes, event) + 400];
    struct pcpes *pcpes = (struct pcpes *)_pcpes;

    if (info_length < sizeof(_pcpes) - offsetof(struct pcpes, event)) {

        pcpes->pcrindex      = pcrIndex;
        pcpes->eventtype     = event_type;
        memset(&pcpes->digest, 0x0, sizeof(pcpes->digest));
        pcpes->eventdatasize = info_length;
        memcpy(&pcpes->event, info, info_length);

        struct hleei_short hleei = {
            .ipblength   = sizeof(hleei),
            .hashdataptr = data,
            .hashdatalen = data_length,
            .pcrindex    = pcrIndex,
            .logdataptr  = _pcpes,
            .logdatalen  = info_length + offsetof(struct pcpes, event),
        };

        rc = hash_log_extend_event(&hleei, &hleeo);
    } else {
        rc = TCG_GENERAL_ERROR;
    }

    return rc;
}


/*
 * Add a measurement to the list of measurements
 * pcrIndex   : PCR to be extended
 * event_type : type of event; specs section on 'Event Types'
 * data       : additional parameter; used as parameter for
 *              'action index'
 */
static u32
tpm_add_measurement(u32 pcrIndex,
                    u16 event_type,
                    const char *string)
{
    u32 rc;
    u32 len;

    switch (event_type) {
    case EV_SEPARATOR:
        len = sizeof(evt_separator);
        rc = tpm_add_measurement_to_log(pcrIndex, event_type,
                                        (char *)NULL, 0,
                                        (u8 *)evt_separator, len);
        break;

    case EV_ACTION:
        rc = tpm_add_measurement_to_log(pcrIndex, event_type,
                                        string, strlen(string),
                                        (u8 *)string, strlen(string));
        break;

    default:
        rc = TCG_INVALID_INPUT_PARA;
    }

    return rc;
}


static u32
tpm_calling_int19h(void)
{
    if (!CONFIG_TCGBIOS)
        return 0;

    if (!has_working_tpm())
        return TCG_GENERAL_ERROR;

    return tpm_add_measurement(4, EV_ACTION,
                               "Calling INT 19h");
}

/*
 * Add event separators for PCRs 0 to 7; specs on 'Measuring Boot Events'
 */
u32
tpm_add_event_separators(void)
{
    u32 rc;
    u32 pcrIndex = 0;

    if (!CONFIG_TCGBIOS)
        return 0;

    if (!has_working_tpm())
        return TCG_GENERAL_ERROR;

    while (pcrIndex <= 7) {
        rc = tpm_add_measurement(pcrIndex, EV_SEPARATOR, NULL);
        if (rc)
            break;
        pcrIndex ++;
    }

    return rc;
}


/*
 * Add a measurement regarding the boot device (CDRom, Floppy, HDD) to
 * the list of measurements.
 */
static u32
tpm_add_bootdevice(u32 bootcd, u32 bootdrv)
{
    const char *string;

    if (!CONFIG_TCGBIOS)
        return 0;

    if (!has_working_tpm())
        return TCG_GENERAL_ERROR;

    switch (bootcd) {
    case 0:
        switch (bootdrv) {
        case 0:
            string = "Booting BCV device 00h (Floppy)";
            break;

        case 0x80:
            string = "Booting BCV device 80h (HDD)";
            break;

        default:
            string = "Booting unknown device";
            break;
        }

        break;

    default:
        string = "Booting from CD ROM device";
    }

    return tpm_add_measurement_to_log(4, EV_ACTION,
                                      string, strlen(string),
                                      (u8 *)string, strlen(string));
}


/*
 * Add measurement to the log about option rom scan
 */
u32
tpm_start_option_rom_scan(void)
{
    if (!CONFIG_TCGBIOS)
        return 0;

    if (!has_working_tpm())
        return TCG_GENERAL_ERROR;

    return tpm_add_measurement(2, EV_ACTION,
                               "Start Option ROM Scan");
}


/*
 * Add measurement to the log about an option rom
 */
u32
tpm_option_rom(const void *addr, u32 len)
{
    if (!CONFIG_TCGBIOS)
        return 0;

    if (!has_working_tpm())
        return TCG_GENERAL_ERROR;

    u32 rc;
    struct pcctes_romex pcctes = {
        .eventid = 7,
        .eventdatasize = sizeof(u16) + sizeof(u16) + SHA1_BUFSIZE,
    };

    rc = sha1((const u8 *)addr, len, pcctes.digest);
    if (rc)
        return rc;

    return tpm_add_measurement_to_log(2,
                                      EV_EVENT_TAG,
                                      (const char *)&pcctes, sizeof(pcctes),
                                      (u8 *)&pcctes, sizeof(pcctes));
}


u32
tpm_smbios_measure(void)
{
    if (!CONFIG_TCGBIOS)
        return 0;

    if (!has_working_tpm())
        return TCG_GENERAL_ERROR;

    u32 rc;
    struct pcctes pcctes = {
        .eventid = 1,
        .eventdatasize = SHA1_BUFSIZE,
    };
    struct smbios_entry_point *sep = SMBiosAddr;

    dprintf(DEBUG_tcg, "TCGBIOS: SMBIOS at %p\n", sep);

    if (!sep)
        return 0;

    rc = sha1((const u8 *)sep->structure_table_address,
              sep->structure_table_length, pcctes.digest);
    if (rc)
        return rc;

    return tpm_add_measurement_to_log(1,
                                      EV_EVENT_TAG,
                                      (const char *)&pcctes, sizeof(pcctes),
                                      (u8 *)&pcctes, sizeof(pcctes));
}


/*
 * Add a measurement related to Initial Program Loader to the log.
 * Creates two log entries.
 *
 * Input parameter:
 *  bootcd : 0: MBR of hdd, 1: boot image, 2: boot catalog of El Torito
 *  addr   : address where the IP data are located
 *  length : IP data length in bytes
 */
static u32
tpm_ipl(enum ipltype bootcd, const u8 *addr, u32 length)
{
    u32 rc;
    const char *string;

    switch (bootcd) {
    case IPL_EL_TORITO_1:
        /* specs: see section 'El Torito' */
        string = "EL TORITO IPL";
        rc = tpm_add_measurement_to_log(4, EV_IPL,
                                        string, strlen(string),
                                        addr, length);
        break;

    case IPL_EL_TORITO_2:
        /* specs: see section 'El Torito' */
        string = "BOOT CATALOG";
        rc = tpm_add_measurement_to_log(5, EV_IPL_PARTITION_DATA,
                                        string, strlen(string),
                                        addr, length);
        break;

    default:
        /* specs: see section 'Hard Disk Device or Hard Disk-Like Devices' */
        /* equivalent to: dd if=/dev/hda ibs=1 count=440 | sha1sum */
        string = "MBR";
        rc = tpm_add_measurement_to_log(4, EV_IPL,
                                        string, strlen(string),
                                        addr, 0x1b8);

        if (rc)
            break;

        /* equivalent to: dd if=/dev/hda ibs=1 count=72 skip=440 | sha1sum */
        string = "MBR PARTITION_TABLE";
        rc = tpm_add_measurement_to_log(5, EV_IPL_PARTITION_DATA,
                                        string, strlen(string),
                                        addr + 0x1b8, 0x48);
    }

    return rc;
}

u32
tpm_add_bcv(u32 bootdrv, const u8 *addr, u32 length)
{
    if (!CONFIG_TCGBIOS)
        return 0;

    if (!has_working_tpm())
        return TCG_GENERAL_ERROR;

    u32 rc = tpm_add_bootdevice(0, bootdrv);
    if (rc)
        return rc;

    return tpm_ipl(IPL_BCV, addr, length);
}

u32
tpm_add_cdrom(u32 bootdrv, const u8 *addr, u32 length)
{
    if (!CONFIG_TCGBIOS)
        return 0;

    if (!has_working_tpm())
        return TCG_GENERAL_ERROR;

    u32 rc = tpm_add_bootdevice(1, bootdrv);
    if (rc)
        return rc;

    return tpm_ipl(IPL_EL_TORITO_1, addr, length);
}

u32
tpm_add_cdrom_catalog(const u8 *addr, u32 length)
{
    if (!CONFIG_TCGBIOS)
        return 0;

    if (!has_working_tpm())
        return TCG_GENERAL_ERROR;

    u32 rc = tpm_add_bootdevice(1, 0);
    if (rc)
        return rc;

    return tpm_ipl(IPL_EL_TORITO_2, addr, length);
}

void
tpm_s3_resume(void)
{
    u32 rc;
    u32 returnCode;

    if (!CONFIG_TCGBIOS)
        return;

    if (!has_working_tpm())
        return;

    dprintf(DEBUG_tcg, "TCGBIOS: Resuming with TPM_Startup(ST_STATE)\n");

    rc = build_and_send_cmd(0, TPM_ORD_Startup,
                            Startup_ST_STATE, sizeof(Startup_ST_STATE),
                            NULL, 0, &returnCode, TPM_DURATION_TYPE_SHORT);

    dprintf(DEBUG_tcg, "TCGBIOS: ReturnCode from TPM_Startup = 0x%08x\n",
            returnCode);

    if (rc || returnCode)
        goto err_exit;

    return;

err_exit:
    dprintf(DEBUG_tcg, "TCGBIOS: TPM malfunctioning (line %d).\n", __LINE__);

    tpm_state.tpm_working = 0;
}
