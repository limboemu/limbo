/* GIO - GLib Input, Output and Streaming Library
 * Copyright © 2010 Collabora Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the licence or (at
 * your option) any later version.
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
 *
 * Authors: Nicolas Dufresne <nicolas.dufresne@collabora.co.uk>
 *
 */

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#ifndef __G_PROXY_PROXY_CONNECTION_H__
#define __G_PROXY_PROXY_CONNECTION_H__

#include <glib-object.h>
#include <gio/gtcpconnection.h>

G_BEGIN_DECLS

#define G_TYPE_PROXY_CONNECTION                  (_g_proxy_connection_get_type ())
#define G_PROXY_PROXY_CONNECTION(inst)           (G_TYPE_CHECK_INSTANCE_CAST ((inst),                     \
                                                         G_TYPE_PROXY_CONNECTION, GProxyConnection))
#define G_PROXY_PROXY_CONNECTION_CLASS(class)    (G_TYPE_CHECK_CLASS_CAST ((class),                       \
                                                         G_TYPE_PROXY_CONNECTION, GProxyConnectionClass))
#define G_IS_PROXY_CONNECTION(inst)              (G_TYPE_CHECK_INSTANCE_TYPE ((inst),                     \
                                                         G_TYPE_PROXY_CONNECTION))
#define G_IS_PROXY_CONNECTION_CLASS(class)       (G_TYPE_CHECK_CLASS_TYPE ((class),                       \
                                                         G_TYPE_PROXY_CONNECTION))
#define G_PROXY_PROXY_CONNECTION_GET_CLASS(inst) (G_TYPE_INSTANCE_GET_CLASS ((inst),                      \
                                                         G_TYPE_PROXY_CONNECTION, GProxyConnectionClass))

typedef struct _GProxyConnection                   GProxyConnection;
typedef struct _GProxyConnectionPrivate            GProxyConnectionPrivate;
typedef struct _GProxyConnectionClass              GProxyConnectionClass;

struct _GProxyConnectionClass
{
  GTcpConnectionClass parent_class;
};

struct _GProxyConnection
{
  GTcpConnection parent_instance;
  GProxyConnectionPrivate *priv;
};

GType _g_proxy_connection_get_type (void) G_GNUC_CONST;

GSocketConnection *_g_proxy_connection_new (GTcpConnection *connection,
                                            GIOStream      *io_stream);

G_END_DECLS

#endif /* __G_PROXY_PROXY_CONNECTION_H__ */
