/* Disabled from config/defaults/pcbios.h */

#undef IMAGE_ELF
#undef SANBOOT_PROTO_ISCSI
#undef SANBOOT_PROTO_AOE
#undef SANBOOT_PROTO_IB_SRP
#undef SANBOOT_PROTO_FCP
#undef REBOOT_CMD
#undef CPUID_CMD

/* Disabled from config/general.h */

#undef DOWNLOAD_PROTO_HTTP
#undef CRYPTO_80211_WEP
#undef CRYPTO_80211_WPA
#undef CRYPTO_80211_WPA2
#undef IWMGMT_CMD
#undef FCMGMT_CMD
#undef SANBOOT_CMD
#undef MENU_CMD
#undef LOGIN_CMD
#undef SYNC_CMD

/* Ensure ROM banner is not displayed */

#undef ROM_BANNER_TIMEOUT
#define ROM_BANNER_TIMEOUT 0
