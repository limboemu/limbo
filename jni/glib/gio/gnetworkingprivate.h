/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2008 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __G_NETWORKINGPRIVATE_H__
#define __G_NETWORKINGPRIVATE_H__

#ifdef G_OS_WIN32

#define _WIN32_WINNT 0x0501
#include <winsock2.h>
#undef interface
#include <ws2tcpip.h>
#include <windns.h>
#include <mswsock.h>

#ifdef HAVE_WSPIAPI_H
/* <wspiapi.h> in the Windows SDK and in mingw-w64 has wrappers for
 * inline workarounds for getaddrinfo, getnameinfo and freeaddrinfo if
 * they aren't present at run-time (on Windows 2000).
 */
#include <wspiapi.h>
#endif

#else /* !G_OS_WIN32 */

/* need this for struct ucred on Linux */
#ifdef __linux__
#define __USE_GNU
#include <sys/types.h>
#include <sys/socket.h>
#undef __USE_GNU
#endif

#include <sys/types.h>
#include <arpa/inet.h>
#include <arpa/nameser.h>
#if defined(HAVE_ARPA_NAMESER_COMPAT_H) && !defined(GETSHORT)
#include <arpa/nameser_compat.h>
#endif

#ifndef T_SRV
#define T_SRV 33
#endif

/* We're supposed to define _GNU_SOURCE to get EAI_NODATA, but that
 * won't actually work since <features.h> has already been included at
 * this point. So we define __USE_GNU instead.
 */
#define __USE_GNU
#include <netdb.h>
#undef __USE_GNU
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <resolv.h>
#include <sys/socket.h>
#include <sys/un.h>

#ifndef _PATH_RESCONF
#define _PATH_RESCONF "/etc/resolv.conf"
#endif

#ifndef CMSG_LEN
/* CMSG_LEN and CMSG_SPACE are defined by RFC 2292, but missing on
 * some older platforms.
 */
#define CMSG_LEN(len) ((size_t)CMSG_DATA((struct cmsghdr *)NULL) + (len))

/* CMSG_SPACE must add at least as much padding as CMSG_NXTHDR()
 * adds. We overestimate here.
 */
#define ALIGN_TO_SIZEOF(len, obj) (((len) + sizeof (obj) - 1) & ~(sizeof (obj) - 1))
#define CMSG_SPACE(len) ALIGN_TO_SIZEOF (CMSG_LEN (len), struct cmsghdr)
#endif
#endif

G_BEGIN_DECLS

extern struct addrinfo _g_resolver_addrinfo_hints;

GList *_g_resolver_addresses_from_addrinfo (const char       *hostname,
					    struct addrinfo  *res,
					    gint              gai_retval,
					    GError          **error);

void   _g_resolver_address_to_sockaddr     (GInetAddress            *address,
					    struct sockaddr_storage *sa,
					    gsize                   *len);
char  *_g_resolver_name_from_nameinfo      (GInetAddress     *address,
					    const gchar      *name,
					    gint              gni_retval,
					    GError          **error);

#if defined(G_OS_UNIX)
GList *_g_resolver_targets_from_res_query  (const gchar      *rrname,
					    guchar           *answer,
					    gint              len,
					    gint              herr,
					    GError          **error);
#elif defined(G_OS_WIN32)
GList *_g_resolver_targets_from_DnsQuery   (const gchar      *rrname,
					    DNS_STATUS        status,
					    DNS_RECORD       *results,
					    GError          **error);
#endif

gboolean _g_uri_parse_authority            (const char       *uri,
					    char            **host,
					    guint16          *port,
					    char            **userinfo);
gchar *  _g_uri_from_authority             (const gchar      *protocol,
					    const gchar      *host,
					    guint             port,
					    const gchar      *userinfo);

G_END_DECLS

#endif /* __G_NETWORKINGPRIVATE_H__ */
