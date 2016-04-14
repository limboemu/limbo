/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2, or (at
 * your option) any later version.
 */

FILE_LICENCE ( GPL2_OR_LATER );

#include <config/general.h>

/** @file
 *
 * 802.11 configuration options
 *
 */

/*
 * Drag in 802.11-specific commands
 *
 */
#ifdef IWMGMT_CMD
REQUIRE_OBJECT ( iwmgmt_cmd );
#endif

/*
 * Drag in 802.11 error message tables
 *
 */
#ifdef ERRMSG_80211
REQUIRE_OBJECT ( wireless_errors );
#endif

/*
 * Drag in 802.11 cryptosystems and handshaking protocols
 *
 */
#ifdef CRYPTO_80211_WEP
REQUIRE_OBJECT ( wep );
#endif

#ifdef CRYPTO_80211_WPA2
#define CRYPTO_80211_WPA
REQUIRE_OBJECT ( wpa_ccmp );
#endif

#ifdef CRYPTO_80211_WPA
REQUIRE_OBJECT ( wpa_psk );
REQUIRE_OBJECT ( wpa_tkip );
#endif
