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

#ifndef __G_UNIX_RESOLVER_H__
#define __G_UNIX_RESOLVER_H__

#include <gio/gthreadedresolver.h>
#include "libasyncns/asyncns.h"

G_BEGIN_DECLS

#define G_TYPE_UNIX_RESOLVER         (g_unix_resolver_get_type ())
#define G_UNIX_RESOLVER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), G_TYPE_UNIX_RESOLVER, GUnixResolver))
#define G_UNIX_RESOLVER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), G_TYPE_UNIX_RESOLVER, GUnixResolverClass))
#define G_IS_UNIX_RESOLVER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), G_TYPE_UNIX_RESOLVER))
#define G_IS_UNIX_RESOLVER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), G_TYPE_UNIX_RESOLVER))
#define G_UNIX_RESOLVER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), G_TYPE_UNIX_RESOLVER, GUnixResolverClass))

typedef struct {
  GThreadedResolver parent_instance;

  _g_asyncns_t *asyncns;
  guint watch;

} GUnixResolver;

typedef struct {
  GThreadedResolverClass parent_class;

} GUnixResolverClass;

GType g_unix_resolver_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __G_RESOLVER_H__ */
