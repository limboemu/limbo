#ifndef TPM_DRIVERS_H
#define TPM_DRIVERS_H

#include "types.h" // u32


enum tpmDurationType {
    TPM_DURATION_TYPE_SHORT = 0,
    TPM_DURATION_TYPE_MEDIUM,
    TPM_DURATION_TYPE_LONG,
};

/* low level driver implementation */
struct tpm_driver {
    u32 *timeouts;
    u32 *durations;
    void (*set_timeouts)(u32 timeouts[4], u32 durations[3]);
    u32 (*probe)(void);
    u32 (*init)(void);
    u32 (*activate)(u8 locty);
    u32 (*ready)(void);
    u32 (*senddata)(const u8 *const data, u32 len);
    u32 (*readresp)(u8 *buffer, u32 *len);
    u32 (*waitdatavalid)(void);
    u32 (*waitrespready)(enum tpmDurationType to_t);
    /* the TPM will be used for buffers of sizes below the sha1threshold
       for calculating the hash */
    u32 sha1threshold;
};

extern struct tpm_driver tpm_drivers[];


#define TIS_DRIVER_IDX       0
#define TPM_NUM_DRIVERS      1

#define TPM_INVALID_DRIVER  -1

/* TIS driver */
/* address of locality 0 (TIS) */
#define TPM_TIS_BASE_ADDRESS        0xfed40000

#define TIS_REG(LOCTY, REG) \
    (void *)(TPM_TIS_BASE_ADDRESS + (LOCTY << 12) + REG)

/* hardware registers */
#define TIS_REG_ACCESS                 0x0
#define TIS_REG_INT_ENABLE             0x8
#define TIS_REG_INT_VECTOR             0xc
#define TIS_REG_INT_STATUS             0x10
#define TIS_REG_INTF_CAPABILITY        0x14
#define TIS_REG_STS                    0x18
#define TIS_REG_DATA_FIFO              0x24
#define TIS_REG_DID_VID                0xf00
#define TIS_REG_RID                    0xf04

#define TIS_STS_VALID                  (1 << 7) /* 0x80 */
#define TIS_STS_COMMAND_READY          (1 << 6) /* 0x40 */
#define TIS_STS_TPM_GO                 (1 << 5) /* 0x20 */
#define TIS_STS_DATA_AVAILABLE         (1 << 4) /* 0x10 */
#define TIS_STS_EXPECT                 (1 << 3) /* 0x08 */
#define TIS_STS_RESPONSE_RETRY         (1 << 1) /* 0x02 */

#define TIS_ACCESS_TPM_REG_VALID_STS   (1 << 7) /* 0x80 */
#define TIS_ACCESS_ACTIVE_LOCALITY     (1 << 5) /* 0x20 */
#define TIS_ACCESS_BEEN_SEIZED         (1 << 4) /* 0x10 */
#define TIS_ACCESS_SEIZE               (1 << 3) /* 0x08 */
#define TIS_ACCESS_PENDING_REQUEST     (1 << 2) /* 0x04 */
#define TIS_ACCESS_REQUEST_USE         (1 << 1) /* 0x02 */
#define TIS_ACCESS_TPM_ESTABLISHMENT   (1 << 0) /* 0x01 */

#define SCALER 10

#define TIS_DEFAULT_TIMEOUT_A          (750  * SCALER)
#define TIS_DEFAULT_TIMEOUT_B          (2000 * SCALER)
#define TIS_DEFAULT_TIMEOUT_C          (750  * SCALER)
#define TIS_DEFAULT_TIMEOUT_D          (750  * SCALER)

enum tisTimeoutType {
    TIS_TIMEOUT_TYPE_A = 0,
    TIS_TIMEOUT_TYPE_B,
    TIS_TIMEOUT_TYPE_C,
    TIS_TIMEOUT_TYPE_D,
};

#define TPM_DEFAULT_DURATION_SHORT     (2000  * SCALER)
#define TPM_DEFAULT_DURATION_MEDIUM    (20000 * SCALER)
#define TPM_DEFAULT_DURATION_LONG      (60000 * SCALER)

#endif /* TPM_DRIVERS_H */
