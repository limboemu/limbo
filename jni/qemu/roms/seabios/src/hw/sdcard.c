// PCI SD Host Controller Interface
//
// Copyright (C) 2014  Kevin O'Connor <kevin@koconnor.net>
//
// This file may be distributed under the terms of the GNU LGPLv3 license.

#include "block.h" // struct drive_s
#include "malloc.h" // malloc_fseg
#include "output.h" // znprintf
#include "pci.h" // pci_config_readl
#include "pci_ids.h" // PCI_CLASS_SYSTEM_SDHCI
#include "pci_regs.h" // PCI_BASE_ADDRESS_0
#include "romfile.h" // romfile_findprefix
#include "stacks.h" // wait_preempt
#include "std/disk.h" // DISK_RET_SUCCESS
#include "string.h" // memset
#include "util.h" // boot_add_hd
#include "x86.h" // writel

// SDHCI MMIO registers
struct sdhci_s {
    u32 sdma_addr;
    u16 block_size;
    u16 block_count;
    u32 arg;
    u16 transfer_mode;
    u16 cmd;
    u32 response[4];
    u32 data;
    u32 present_state;
    u8 host_control;
    u8 power_control;
    u8 block_gap_control;
    u8 wakeup_control;
    u16 clock_control;
    u8 timeout_control;
    u8 software_reset;
    u16 irq_status;
    u16 error_irq_status;
    u16 irq_enable;
    u16 error_irq_enable;
    u16 irq_signal;
    u16 error_signal;
    u16 auto_cmd12;
    u16 host_control2;
    u32 cap_lo, cap_hi;
    u64 max_current;
    u16 force_auto_cmd12;
    u16 force_error;
    u8 adma_error;
    u8 pad_55[3];
    u64 adma_addr;
    u8 pad_60[156];
    u16 slot_irq;
    u16 controller_version;
} PACKED;

// SDHCI commands
#define SCB_R0   0x00 // No response
#define SCB_R48  0x1a // Response R1 (no data), R5, R6, R7
#define SCB_R48d 0x3a // Response R1 (with data)
#define SCB_R48b 0x1b // Response R1b, R5b
#define SCB_R48o 0x02 // Response R3, R4
#define SCB_R136 0x09 // Response R2
#define SC_GO_IDLE_STATE        ((0<<8) | SCB_R0)
#define SC_SEND_OP_COND         ((1<<8) | SCB_R48o)
#define SC_ALL_SEND_CID         ((2<<8) | SCB_R136)
#define SC_SEND_RELATIVE_ADDR   ((3<<8) | SCB_R48)
#define SC_SELECT_DESELECT_CARD ((7<<8) | SCB_R48b)
#define SC_SEND_IF_COND         ((8<<8) | SCB_R48)
#define SC_SEND_EXT_CSD         ((8<<8) | SCB_R48d)
#define SC_SEND_CSD             ((9<<8) | SCB_R136)
#define SC_READ_SINGLE          ((17<<8) | SCB_R48d)
#define SC_READ_MULTIPLE        ((18<<8) | SCB_R48d)
#define SC_WRITE_SINGLE         ((24<<8) | SCB_R48d)
#define SC_WRITE_MULTIPLE       ((25<<8) | SCB_R48d)
#define SC_APP_CMD              ((55<<8) | SCB_R48)
#define SC_APP_SEND_OP_COND ((41<<8) | SCB_R48o)

// SDHCI irqs
#define SI_CMD_COMPLETE (1<<0)
#define SI_TRANS_DONE   (1<<1)
#define SI_WRITE_READY  (1<<4)
#define SI_READ_READY   (1<<5)
#define SI_ERROR        (1<<15)

// SDHCI present_state flags
#define SP_CMD_INHIBIT   (1<<0)
#define SP_DAT_INHIBIT   (1<<1)
#define SP_CARD_INSERTED (1<<16)

// SDHCI transfer_mode flags
#define ST_BLOCKCOUNT (1<<1)
#define ST_AUTO_CMD12 (1<<2)
#define ST_READ       (1<<4)
#define ST_MULTIPLE   (1<<5)

// SDHCI capabilities flags
#define SD_CAPLO_V33             (1<<24)
#define SD_CAPLO_V30             (1<<25)
#define SD_CAPLO_V18             (1<<26)
#define SD_CAPLO_BASECLOCK_SHIFT 8
#define SD_CAPLO_BASECLOCK_MASK  0xff

// SDHCI clock control flags
#define SCC_INTERNAL_ENABLE (1<<0)
#define SCC_STABLE          (1<<1)
#define SCC_CLOCK_ENABLE    (1<<2)
#define SCC_SDCLK_MASK      0xff
#define SCC_SDCLK_SHIFT     8
#define SCC_SDCLK_HI_MASK   0x300
#define SCC_SDCLK_HI_RSHIFT 2

// SDHCI power control flags
#define SPC_POWER_ON (1<<0)
#define SPC_V18      0x0a
#define SPC_V30      0x0c
#define SPC_V33      0x0e

// SDHCI software reset flags
#define SRF_ALL  0x01
#define SRF_CMD  0x02
#define SRF_DATA 0x04

// SDHCI result flags
#define SR_OCR_CCS     (1<<30)
#define SR_OCR_NOTBUSY (1<<31)

// SDHCI timeouts
#define SDHCI_POWER_OFF_TIME   1
#define SDHCI_POWER_ON_TIME    1
#define SDHCI_CLOCK_ON_TIME    1 // 74 clock cycles
#define SDHCI_POWERUP_TIMEOUT  1000
#define SDHCI_PIO_TIMEOUT      1000  // XXX - this is just made up

// Internal 'struct drive_s' storage for a detected card
struct sddrive_s {
    struct drive_s drive;
    struct sdhci_s *regs;
    int card_type;
};

// SD card types
#define SF_MMC          (1<<0)
#define SF_HIGHCAPACITY (1<<1)

// Repeatedly read a u16 register until any bit in a given mask is set
static int
sdcard_waitw(u16 *reg, u16 mask)
{
    u32 end = timer_calc(SDHCI_PIO_TIMEOUT);
    for (;;) {
        u16 v = readw(reg);
        if (v & mask)
            return v;
        if (timer_check(end)) {
            warn_timeout();
            return -1;
        }
        yield();
    }
}

// Send an sdhci reset
static int
sdcard_reset(struct sdhci_s *regs, int flags)
{
    writeb(&regs->software_reset, flags);
    u32 end = timer_calc(SDHCI_PIO_TIMEOUT);
    while (readb(&regs->software_reset))
        if (timer_check(end)) {
            warn_timeout();
            return -1;
        }
    return 0;
}

// Send a command to the card.
static int
sdcard_pio(struct sdhci_s *regs, int cmd, u32 *param)
{
    u32 state = readl(&regs->present_state);
    dprintf(9, "sdcard_pio cmd %x %x %x\n", cmd, *param, state);
    if ((state & SP_CMD_INHIBIT)
        || ((cmd & 0x03) == 0x03 && state & SP_DAT_INHIBIT)) {
        dprintf(1, "sdcard_pio not ready %x\n", state);
        return -1;
    }
    // Send command
    writel(&regs->arg, *param);
    writew(&regs->cmd, cmd);
    int ret = sdcard_waitw(&regs->irq_status, SI_ERROR|SI_CMD_COMPLETE);
    if (ret < 0)
        return ret;
    if (ret & SI_ERROR) {
        u16 err = readw(&regs->error_irq_status);
        dprintf(3, "sdcard_pio command stop (code=%x)\n", err);
        sdcard_reset(regs, SRF_CMD|SRF_DATA);
        writew(&regs->error_irq_status, err);
        return -1;
    }
    writew(&regs->irq_status, SI_CMD_COMPLETE);
    // Read response
    memcpy(param, regs->response, sizeof(regs->response));
    dprintf(9, "sdcard cmd %x response %x %x %x %x\n"
            , cmd, param[0], param[1], param[2], param[3]);
    return 0;
}

// Send an "app specific" command to the card.
static int
sdcard_pio_app(struct sdhci_s *regs, int cmd, u32 *param)
{
    u32 aparam[4] = {};
    int ret = sdcard_pio(regs, SC_APP_CMD, aparam);
    if (ret)
        return ret;
    return sdcard_pio(regs, cmd, param);
}

// Send a command to the card which transfers data.
static int
sdcard_pio_transfer(struct sddrive_s *drive, int cmd, u32 addr
                    , void *data, int count)
{
    // Send command
    writew(&drive->regs->block_size, DISK_SECTOR_SIZE);
    writew(&drive->regs->block_count, count);
    int isread = cmd != SC_WRITE_SINGLE && cmd != SC_WRITE_MULTIPLE;
    u16 tmode = ((count > 1 ? ST_MULTIPLE|ST_AUTO_CMD12|ST_BLOCKCOUNT : 0)
                 | (isread ? ST_READ : 0));
    writew(&drive->regs->transfer_mode, tmode);
    if (!(drive->card_type & SF_HIGHCAPACITY))
        addr *= DISK_SECTOR_SIZE;
    u32 param[4] = { addr };
    int ret = sdcard_pio(drive->regs, cmd, param);
    if (ret)
        return ret;
    // Read/write data
    u16 cbit = isread ? SI_READ_READY : SI_WRITE_READY;
    while (count--) {
        ret = sdcard_waitw(&drive->regs->irq_status, cbit);
        if (ret < 0)
            return ret;
        writew(&drive->regs->irq_status, cbit);
        int i;
        for (i=0; i<DISK_SECTOR_SIZE/4; i++) {
            if (isread)
                *(u32*)data = readl(&drive->regs->data);
            else
                writel(&drive->regs->data, *(u32*)data);
            data += 4;
        }
    }
    // Complete command
    ret = sdcard_waitw(&drive->regs->irq_status, SI_TRANS_DONE);
    if (ret < 0)
        return ret;
    writew(&drive->regs->irq_status, SI_TRANS_DONE);
    return 0;
}

// Read/write a block of data to/from the card.
static int
sdcard_readwrite(struct disk_op_s *op, int iswrite)
{
    struct sddrive_s *drive = container_of(
        op->drive_gf, struct sddrive_s, drive);
    int cmd = iswrite ? SC_WRITE_SINGLE : SC_READ_SINGLE;
    if (op->count > 1)
        cmd = iswrite ? SC_WRITE_MULTIPLE : SC_READ_MULTIPLE;
    int ret = sdcard_pio_transfer(drive, cmd, op->lba, op->buf_fl, op->count);
    if (ret)
        return DISK_RET_EBADTRACK;
    return DISK_RET_SUCCESS;
}

int
sdcard_process_op(struct disk_op_s *op)
{
    if (!CONFIG_SDCARD)
        return 0;
    switch (op->command) {
    case CMD_READ:
        return sdcard_readwrite(op, 0);
    case CMD_WRITE:
        return sdcard_readwrite(op, 1);
    default:
        return default_process_op(op);
    }
}


/****************************************************************
 * Setup
 ****************************************************************/

static int
sdcard_set_power(struct sdhci_s *regs)
{
    u32 cap = readl(&regs->cap_lo);
    u32 volt, vbits;
    if (cap & SD_CAPLO_V33) {
        volt = 1<<20;
        vbits = SPC_V33;
    } else if (cap & SD_CAPLO_V30) {
        volt = 1<<18;
        vbits = SPC_V30;
    } else if (cap & SD_CAPLO_V18) {
        volt = 1<<7;
        vbits = SPC_V18;
    } else {
        dprintf(1, "SD controller unsupported volt range (%x)\n", cap);
        return -1;
    }
    writeb(&regs->power_control, 0);
    msleep(SDHCI_POWER_OFF_TIME);
    writeb(&regs->power_control, vbits | SPC_POWER_ON);
    msleep(SDHCI_POWER_ON_TIME);
    return volt;
}

static int
sdcard_set_frequency(struct sdhci_s *regs, u32 khz)
{
    u16 ver = readw(&regs->controller_version);
    u32 cap = readl(&regs->cap_lo);
    u32 base_freq = (cap >> SD_CAPLO_BASECLOCK_SHIFT) & SD_CAPLO_BASECLOCK_MASK;
    if (!base_freq) {
        dprintf(1, "Unknown base frequency for SD controller\n");
        return -1;
    }
    // Set new frequency
    u32 divisor = DIV_ROUND_UP(base_freq * 1000, khz);
    u16 creg;
    if ((ver & 0xff) <= 0x01) {
        divisor = divisor > 1 ? 1 << __fls(divisor-1) : 0;
        creg = (divisor & SCC_SDCLK_MASK) << SCC_SDCLK_SHIFT;
    } else {
        divisor = DIV_ROUND_UP(divisor, 2);
        creg = (divisor & SCC_SDCLK_MASK) << SCC_SDCLK_SHIFT;
        creg |= (divisor & SCC_SDCLK_HI_MASK) >> SCC_SDCLK_HI_RSHIFT;
    }
    dprintf(3, "sdcard_set_frequency %d %d %x\n", base_freq, khz, creg);
    writew(&regs->clock_control, 0);
    writew(&regs->clock_control, creg | SCC_INTERNAL_ENABLE);
    // Wait for frequency to become active
    int ret = sdcard_waitw(&regs->clock_control, SCC_STABLE);
    if (ret < 0)
        return ret;
    // Enable SD clock
    writew(&regs->clock_control, creg | SCC_INTERNAL_ENABLE | SCC_CLOCK_ENABLE);
    return 0;
}

// Obtain the disk size of an SD card
static int
sdcard_get_capacity(struct sddrive_s *drive, u8 *csd)
{
    // Original MMC/SD card capacity formula
    u16 C_SIZE = (csd[6] >> 6) | (csd[7] << 2) | ((csd[8] & 0x03) << 10);
    u8 C_SIZE_MULT = (csd[4] >> 7) | ((csd[5] & 0x03) << 1);
    u8 READ_BL_LEN = csd[9] & 0x0f;
    u32 count = (C_SIZE+1) << (C_SIZE_MULT + 2 + READ_BL_LEN - 9);
    // Check for newer encoding formats.
    u8 CSD_STRUCTURE = csd[14] >> 6;
    if ((drive->card_type & SF_MMC) && CSD_STRUCTURE >= 2) {
        // Get capacity from EXT_CSD register
        u8 ext_csd[512];
        int ret = sdcard_pio_transfer(drive, SC_SEND_EXT_CSD, 0, ext_csd, 1);
        if (ret)
            return ret;
        count = *(u32*)&ext_csd[212];
    } else if (!(drive->card_type & SF_MMC) && CSD_STRUCTURE >= 1) {
        // High capacity SD card
        u32 C_SIZE2 = csd[5] | (csd[6] << 8) | ((csd[7] & 0x3f) << 16);
        count = (C_SIZE2+1) << (19-9);
    }
    // Fill drive struct and return
    drive->drive.blksize = DISK_SECTOR_SIZE;
    drive->drive.sectors = count;
    return 0;
}

// Initialize an SD card
static int
sdcard_card_setup(struct sddrive_s *drive, int volt, int prio)
{
    struct sdhci_s *regs = drive->regs;
    // Set controller to initialization clock rate
    int ret = sdcard_set_frequency(regs, 400);
    if (ret)
        return ret;
    msleep(SDHCI_CLOCK_ON_TIME);
    // Reset card
    u32 param[4] = { };
    ret = sdcard_pio(regs, SC_GO_IDLE_STATE, param);
    if (ret)
        return ret;
    // Let card know SDHC/SDXC is supported and confirm voltage
    u32 hcs = 0, vrange = (volt >= (1<<15) ? 0x100 : 0x200) | 0xaa;
    param[0] = vrange;
    ret = sdcard_pio(regs, SC_SEND_IF_COND, param);
    if (!ret && param[0] == vrange)
        hcs = (1<<30);
    // Verify SD card (instead of MMC or SDIO)
    param[0] = 0x00;
    ret = sdcard_pio_app(regs, SC_APP_SEND_OP_COND, param);
    if (ret) {
        // Check for MMC card
        param[0] = 0x00;
        ret = sdcard_pio(regs, SC_SEND_OP_COND, param);
        if (ret)
            return ret;
        drive->card_type |= SF_MMC;
        hcs = (1<<30);
    }
    // Init card
    u32 end = timer_calc(SDHCI_POWERUP_TIMEOUT);
    for (;;) {
        param[0] = hcs | volt; // high-capacity support and voltage level
        if (drive->card_type & SF_MMC)
            ret = sdcard_pio(regs, SC_SEND_OP_COND, param);
        else
            ret = sdcard_pio_app(regs, SC_APP_SEND_OP_COND, param);
        if (ret)
            return ret;
        if (param[0] & SR_OCR_NOTBUSY)
            break;
        if (timer_check(end)) {
            warn_timeout();
            return -1;
        }
        msleep(5); // Avoid flooding log when debugging
    }
    drive->card_type |= (param[0] & SR_OCR_CCS) ? SF_HIGHCAPACITY : 0;
    // Select card (get cid, set rca, get csd, select card)
    param[0] = 0x00;
    ret = sdcard_pio(regs, SC_ALL_SEND_CID, param);
    if (ret)
        return ret;
    u8 cid[16];
    memcpy(cid, param, sizeof(cid));
    param[0] = drive->card_type & SF_MMC ? 0x0001 << 16 : 0x00;
    ret = sdcard_pio(regs, SC_SEND_RELATIVE_ADDR, param);
    if (ret)
        return ret;
    u16 rca = drive->card_type & SF_MMC ? 0x0001 : param[0] >> 16;
    param[0] = rca << 16;
    ret = sdcard_pio(regs, SC_SEND_CSD, param);
    if (ret)
        return ret;
    u8 csd[16];
    memcpy(csd, param, sizeof(csd));
    param[0] = rca << 16;
    ret = sdcard_pio(regs, SC_SELECT_DESELECT_CARD, param);
    if (ret)
        return ret;
    // Set controller to data transfer clock rate
    ret = sdcard_set_frequency(regs, 25000);
    if (ret)
        return ret;
    // Register drive
    ret = sdcard_get_capacity(drive, csd);
    if (ret)
        return ret;
    char pnm[7] = {};
    int i;
    for (i=0; i < (drive->card_type & SF_MMC ? 6 : 5); i++)
        pnm[i] = cid[11-i];
    char *desc = znprintf(MAXDESCSIZE, "%s %s %dMiB"
                          , drive->card_type & SF_MMC ? "MMC drive" : "SD card"
                          , pnm, (u32)(drive->drive.sectors >> 11));
    dprintf(1, "Found sdcard at %p: %s\n", regs, desc);
    boot_add_hd(&drive->drive, desc, prio);
    return 0;
}

// Setup and configure an SD card controller
static void
sdcard_controller_setup(struct sdhci_s *regs, int prio)
{
    // Initialize controller
    u32 present_state = readl(&regs->present_state);
    if (!(present_state & SP_CARD_INSERTED))
        // No card present
        return;
    dprintf(3, "sdhci@%p ver=%x cap=%x %x\n", regs
            , readw(&regs->controller_version)
            , readl(&regs->cap_lo), readl(&regs->cap_hi));
    sdcard_reset(regs, SRF_ALL);
    writew(&regs->irq_signal, 0);
    writew(&regs->irq_enable, 0x01ff);
    writew(&regs->irq_status, readw(&regs->irq_status));
    writew(&regs->error_signal, 0);
    writew(&regs->error_irq_enable, 0x01ff);
    writew(&regs->error_irq_status, readw(&regs->error_irq_status));
    writeb(&regs->timeout_control, 0x0e); // Set to max timeout
    int volt = sdcard_set_power(regs);
    if (volt < 0)
        return;

    // Initialize card
    struct sddrive_s *drive = malloc_fseg(sizeof(*drive));
    if (!drive) {
        warn_noalloc();
        goto fail;
    }
    memset(drive, 0, sizeof(*drive));
    drive->drive.type = DTYPE_SDCARD;
    drive->regs = regs;
    int ret = sdcard_card_setup(drive, volt, prio);
    if (ret) {
        free(drive);
        goto fail;
    }
    return;
fail:
    writeb(&regs->power_control, 0);
    writew(&regs->clock_control, 0);
}

static void
sdcard_pci_setup(void *data)
{
    struct pci_device *pci = data;
    wait_preempt();  // Avoid pci_config_readl when preempting
    // XXX - bars dependent on slot index register in pci config space
    u32 regs = pci_config_readl(pci->bdf, PCI_BASE_ADDRESS_0);
    regs &= PCI_BASE_ADDRESS_MEM_MASK;
    pci_config_maskw(pci->bdf, PCI_COMMAND, 0,
                     PCI_COMMAND_IO | PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER);
    int prio = bootprio_find_pci_device(pci);
    sdcard_controller_setup((void*)regs, prio);
}

static void
sdcard_romfile_setup(void *data)
{
    struct romfile_s *file = data;
    int prio = bootprio_find_named_rom(file->name, 0);
    u32 addr = romfile_loadint(file->name, 0);
    dprintf(1, "Starting sdcard controller check at addr %x\n", addr);
    sdcard_controller_setup((void*)addr, prio);
}

void
sdcard_setup(void)
{
    if (!CONFIG_SDCARD)
        return;

    struct romfile_s *file = NULL;
    for (;;) {
        file = romfile_findprefix("etc/sdcard", file);
        if (!file)
            break;
        run_thread(sdcard_romfile_setup, file);
    }

    struct pci_device *pci;
    foreachpci(pci) {
        if (pci->class != PCI_CLASS_SYSTEM_SDHCI || pci->prog_if >= 2)
            // Not an SDHCI controller following SDHCI spec
            continue;
        run_thread(sdcard_pci_setup, pci);
    }
}
