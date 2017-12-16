/* GDBus - GLib D-Bus Library
 *
 * Copyright (C) 2008-2010 Red Hat, Inc.
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
 *
 * Author: David Zeuthen <davidz@redhat.com>
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include "gdbusutils.h"
#include "gdbusproxy.h"
#include "gioenumtypes.h"
#include "gdbusconnection.h"
#include "gdbuserror.h"
#include "gdbusprivate.h"
#include "gio-marshal.h"
#include "ginitable.h"
#include "gasyncinitable.h"
#include "gioerror.h"
#include "gasyncresult.h"
#include "gsimpleasyncresult.h"
#include "gcancellable.h"

#include "glibintl.h"

/**
 * SECTION:gdbusproxy
 * @short_description: Client-side proxies
 * @include: gio/gio.h
 *
 * #GDBusProxy is a base class used for proxies to access a D-Bus
 * interface on a remote object. A #GDBusProxy can be constructed for
 * both well-known and unique names.
 *
 * By default, #GDBusProxy will cache all properties (and listen to
 * changes) of the remote object, and proxy all signals that gets
 * emitted. This behaviour can be changed by passing suitable
 * #GDBusProxyFlags when the proxy is created. If the proxy is for a
 * well-known name, the property cache is flushed when the name owner
 * vanishes and reloaded when a name owner appears.
 *
 * If a #GDBusProxy is used for a well-known name, the owner of the
 * name is tracked and can be read from
 * #GDBusProxy:g-name-owner. Connect to the #GObject::notify signal to
 * get notified of changes. Additionally, only signals and property
 * changes emitted from the current name owner are considered and
 * calls are always sent to the current name owner. This avoids a
 * number of race conditions when the name is lost by one owner and
 * claimed by another. However, if no name owner currently exists,
 * then calls will be sent to the well-known name which may result in
 * the message bus launching an owner (unless
 * %G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START is set).
 *
 * The generic #GDBusProxy::g-properties-changed and #GDBusProxy::g-signal
 * signals are not very convenient to work with. Therefore, the recommended
 * way of working with proxies is to subclass #GDBusProxy, and have
 * more natural properties and signals in your derived class.
 *
 * See <xref linkend="gdbus-example-proxy-subclass"/> for an example.
 *
 * <example id="gdbus-wellknown-proxy"><title>GDBusProxy for a well-known-name</title><programlisting><xi:include xmlns:xi="http://www.w3.org/2001/XInclude" parse="text" href="../../../../gio/tests/gdbus-example-watch-proxy.c"><xi:fallback>FIXME: MISSING XINCLUDE CONTENT</xi:fallback></xi:include></programlisting></example>
 */

struct _GDBusProxyPrivate
{
  GBusType bus_type;
  GDBusConnection *connection;

  GDBusProxyFlags flags;
  gchar *name;
  gchar *name_owner;
  gchar *object_path;
  gchar *interface_name;
  gint timeout_msec;

  guint name_owner_changed_subscription_id;

  GCancellable *get_all_cancellable;

  /* gchar* -> GVariant* */
  GHashTable *properties;

  GDBusInterfaceInfo *expected_interface;

  guint properties_changed_subscriber_id;
  guint signals_subscriber_id;

  gboolean initialized;
};

enum
{
  PROP_0,
  PROP_G_CONNECTION,
  PROP_G_BUS_TYPE,
  PROP_G_NAME,
  PROP_G_NAME_OWNER,
  PROP_G_FLAGS,
  PROP_G_OBJECT_PATH,
  PROP_G_INTERFACE_NAME,
  PROP_G_DEFAULT_TIMEOUT,
  PROP_G_INTERFACE_INFO
};

enum
{
  PROPERTIES_CHANGED_SIGNAL,
  SIGNAL_SIGNAL,
  LAST_SIGNAL,
};

guint signals[LAST_SIGNAL] = {0};

static void initable_iface_init       (GInitableIface *initable_iface);
static void async_initable_iface_init (GAsyncInitableIface *async_initable_iface);

G_DEFINE_TYPE_WITH_CODE (GDBusProxy, g_dbus_proxy, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE, initable_iface_init)
                         G_IMPLEMENT_INTERFACE (G_TYPE_ASYNC_INITABLE, async_initable_iface_init)
                         );

static void
g_dbus_proxy_finalize (GObject *object)
{
  GDBusProxy *proxy = G_DBUS_PROXY (object);

  g_warn_if_fail (proxy->priv->get_all_cancellable == NULL);

  if (proxy->priv->name_owner_changed_subscription_id > 0)
    g_dbus_connection_signal_unsubscribe (proxy->priv->connection,
                                          proxy->priv->name_owner_changed_subscription_id);

  if (proxy->priv->properties_changed_subscriber_id > 0)
    g_dbus_connection_signal_unsubscribe (proxy->priv->connection,
                                          proxy->priv->properties_changed_subscriber_id);

  if (proxy->priv->signals_subscriber_id > 0)
    g_dbus_connection_signal_unsubscribe (proxy->priv->connection,
                                          proxy->priv->signals_subscriber_id);

  g_object_unref (proxy->priv->connection);
  g_free (proxy->priv->name);
  g_free (proxy->priv->name_owner);
  g_free (proxy->priv->object_path);
  g_free (proxy->priv->interface_name);
  if (proxy->priv->properties != NULL)
    g_hash_table_unref (proxy->priv->properties);

  if (proxy->priv->expected_interface != NULL)
    g_dbus_interface_info_unref (proxy->priv->expected_interface);

  G_OBJECT_CLASS (g_dbus_proxy_parent_class)->finalize (object);
}

static void
g_dbus_proxy_get_property (GObject    *object,
                           guint       prop_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  GDBusProxy *proxy = G_DBUS_PROXY (object);

  switch (prop_id)
    {
    case PROP_G_CONNECTION:
      g_value_set_object (value, proxy->priv->connection);
      break;

    case PROP_G_FLAGS:
      g_value_set_flags (value, proxy->priv->flags);
      break;

    case PROP_G_NAME:
      g_value_set_string (value, proxy->priv->name);
      break;

    case PROP_G_NAME_OWNER:
      g_value_set_string (value, proxy->priv->name_owner);
      break;

    case PROP_G_OBJECT_PATH:
      g_value_set_string (value, proxy->priv->object_path);
      break;

    case PROP_G_INTERFACE_NAME:
      g_value_set_string (value, proxy->priv->interface_name);
      break;

    case PROP_G_DEFAULT_TIMEOUT:
      g_value_set_int (value, proxy->priv->timeout_msec);
      break;

    case PROP_G_INTERFACE_INFO:
      g_value_set_boxed (value, g_dbus_proxy_get_interface_info (proxy));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
g_dbus_proxy_set_property (GObject      *object,
                           guint         prop_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  GDBusProxy *proxy = G_DBUS_PROXY (object);

  switch (prop_id)
    {
    case PROP_G_CONNECTION:
      proxy->priv->connection = g_value_dup_object (value);
      break;

    case PROP_G_FLAGS:
      proxy->priv->flags = g_value_get_flags (value);
      break;

    case PROP_G_NAME:
      proxy->priv->name = g_value_dup_string (value);
      break;

    case PROP_G_OBJECT_PATH:
      proxy->priv->object_path = g_value_dup_string (value);
      break;

    case PROP_G_INTERFACE_NAME:
      proxy->priv->interface_name = g_value_dup_string (value);
      break;

    case PROP_G_DEFAULT_TIMEOUT:
      g_dbus_proxy_set_default_timeout (proxy, g_value_get_int (value));
      break;

    case PROP_G_INTERFACE_INFO:
      g_dbus_proxy_set_interface_info (proxy, g_value_get_boxed (value));
      break;

    case PROP_G_BUS_TYPE:
      proxy->priv->bus_type = g_value_get_enum (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
g_dbus_proxy_class_init (GDBusProxyClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize     = g_dbus_proxy_finalize;
  gobject_class->set_property = g_dbus_proxy_set_property;
  gobject_class->get_property = g_dbus_proxy_get_property;

  /* Note that all property names are prefixed to avoid collisions with D-Bus property names
   * in derived classes */

  /**
   * GDBusProxy:g-interface-info:
   *
   * Ensure that interactions with this proxy conform to the given
   * interface.  For example, when completing a method call, if the
   * type signature of the message isn't what's expected, the given
   * #GError is set.  Signals that have a type signature mismatch are
   * simply dropped.
   *
   * Since: 2.26
   */
  g_object_class_install_property (gobject_class,
                                   PROP_G_INTERFACE_INFO,
                                   g_param_spec_boxed ("g-interface-info",
                                                       P_("Interface Information"),
                                                       P_("Interface Information"),
                                                       G_TYPE_DBUS_INTERFACE_INFO,
                                                       G_PARAM_READABLE |
                                                       G_PARAM_WRITABLE |
                                                       G_PARAM_STATIC_NAME |
                                                       G_PARAM_STATIC_BLURB |
                                                       G_PARAM_STATIC_NICK));

  /**
   * GDBusProxy:g-connection:
   *
   * The #GDBusConnection the proxy is for.
   *
   * Since: 2.26
   */
  g_object_class_install_property (gobject_class,
                                   PROP_G_CONNECTION,
                                   g_param_spec_object ("g-connection",
                                                        P_("g-connection"),
                                                        P_("The connection the proxy is for"),
                                                        G_TYPE_DBUS_CONNECTION,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_BLURB |
                                                        G_PARAM_STATIC_NICK));

  /**
   * GDBusProxy:g-bus-type:
   *
   * If this property is not %G_BUS_TYPE_NONE, then
   * #GDBusProxy:g-connection must be %NULL and will be set to the
   * #GDBusConnection obtained by calling g_bus_get() with the value
   * of this property.
   *
   * Since: 2.26
   */
  g_object_class_install_property (gobject_class,
                                   PROP_G_BUS_TYPE,
                                   g_param_spec_enum ("g-bus-type",
                                                      P_("Bus Type"),
                                                      P_("The bus to connect to, if any"),
                                                      G_TYPE_BUS_TYPE,
                                                      G_BUS_TYPE_NONE,
                                                      G_PARAM_WRITABLE |
                                                      G_PARAM_CONSTRUCT_ONLY |
                                                      G_PARAM_STATIC_NAME |
                                                      G_PARAM_STATIC_BLURB |
                                                      G_PARAM_STATIC_NICK));

  /**
   * GDBusProxy:g-flags:
   *
   * Flags from the #GDBusProxyFlags enumeration.
   *
   * Since: 2.26
   */
  g_object_class_install_property (gobject_class,
                                   PROP_G_FLAGS,
                                   g_param_spec_flags ("g-flags",
                                                       P_("g-flags"),
                                                       P_("Flags for the proxy"),
                                                       G_TYPE_DBUS_PROXY_FLAGS,
                                                       G_DBUS_PROXY_FLAGS_NONE,
                                                       G_PARAM_READABLE |
                                                       G_PARAM_WRITABLE |
                                                       G_PARAM_CONSTRUCT_ONLY |
                                                       G_PARAM_STATIC_NAME |
                                                       G_PARAM_STATIC_BLURB |
                                                       G_PARAM_STATIC_NICK));

  /**
   * GDBusProxy:g-name:
   *
   * The well-known or unique name that the proxy is for.
   *
   * Since: 2.26
   */
  g_object_class_install_property (gobject_class,
                                   PROP_G_NAME,
                                   g_param_spec_string ("g-name",
                                                        P_("g-name"),
                                                        P_("The well-known or unique name that the proxy is for"),
                                                        NULL,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_BLURB |
                                                        G_PARAM_STATIC_NICK));

  /**
   * GDBusProxy:g-name-owner:
   *
   * The unique name that owns #GDBusProxy:name or %NULL if no-one
   * currently owns that name. You may connect to #GObject::notify signal to
   * track changes to this property.
   *
   * Since: 2.26
   */
  g_object_class_install_property (gobject_class,
                                   PROP_G_NAME_OWNER,
                                   g_param_spec_string ("g-name-owner",
                                                        P_("g-name-owner"),
                                                        P_("The unique name for the owner"),
                                                        NULL,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_BLURB |
                                                        G_PARAM_STATIC_NICK));

  /**
   * GDBusProxy:g-object-path:
   *
   * The object path the proxy is for.
   *
   * Since: 2.26
   */
  g_object_class_install_property (gobject_class,
                                   PROP_G_OBJECT_PATH,
                                   g_param_spec_string ("g-object-path",
                                                        P_("g-object-path"),
                                                        P_("The object path the proxy is for"),
                                                        NULL,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_BLURB |
                                                        G_PARAM_STATIC_NICK));

  /**
   * GDBusProxy:g-interface-name:
   *
   * The D-Bus interface name the proxy is for.
   *
   * Since: 2.26
   */
  g_object_class_install_property (gobject_class,
                                   PROP_G_INTERFACE_NAME,
                                   g_param_spec_string ("g-interface-name",
                                                        P_("g-interface-name"),
                                                        P_("The D-Bus interface name the proxy is for"),
                                                        NULL,
                                                        G_PARAM_READABLE |
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_NAME |
                                                        G_PARAM_STATIC_BLURB |
                                                        G_PARAM_STATIC_NICK));

  /**
   * GDBusProxy:g-default-timeout:
   *
   * The timeout to use if -1 (specifying default timeout) is passed
   * as @timeout_msec in the g_dbus_proxy_call() and
   * g_dbus_proxy_call_sync() functions.
   *
   * This allows applications to set a proxy-wide timeout for all
   * remote method invocations on the proxy. If this property is -1,
   * the default timeout (typically 25 seconds) is used. If set to
   * %G_MAXINT, then no timeout is used.
   *
   * Since: 2.26
   */
  g_object_class_install_property (gobject_class,
                                   PROP_G_DEFAULT_TIMEOUT,
                                   g_param_spec_int ("g-default-timeout",
                                                     P_("Default Timeout"),
                                                     P_("Timeout for remote method invocation"),
                                                     -1,
                                                     G_MAXINT,
                                                     -1,
                                                     G_PARAM_READABLE |
                                                     G_PARAM_WRITABLE |
                                                     G_PARAM_CONSTRUCT |
                                                     G_PARAM_STATIC_NAME |
                                                     G_PARAM_STATIC_BLURB |
                                                     G_PARAM_STATIC_NICK));

  /**
   * GDBusProxy::g-properties-changed:
   * @proxy: The #GDBusProxy emitting the signal.
   * @changed_properties: A #GVariant containing the properties that changed
   * @invalidated_properties: A %NULL terminated array of properties that was invalidated
   *
   * Emitted when one or more D-Bus properties on @proxy changes. The
   * local cache has already been updated when this signal fires. Note
   * that both @changed_properties and @invalidated_properties are
   * guaranteed to never be %NULL (either may be empty though).
   *
   * This signal corresponds to the
   * <literal>PropertiesChanged</literal> D-Bus signal on the
   * <literal>org.freedesktop.DBus.Properties</literal> interface.
   *
   * Since: 2.26
   */
  signals[PROPERTIES_CHANGED_SIGNAL] = g_signal_new ("g-properties-changed",
                                                     G_TYPE_DBUS_PROXY,
                                                     G_SIGNAL_RUN_LAST,
                                                     G_STRUCT_OFFSET (GDBusProxyClass, g_properties_changed),
                                                     NULL,
                                                     NULL,
                                                     _gio_marshal_VOID__VARIANT_BOXED,
                                                     G_TYPE_NONE,
                                                     2,
                                                     G_TYPE_VARIANT,
                                                     G_TYPE_STRV | G_SIGNAL_TYPE_STATIC_SCOPE);

  /**
   * GDBusProxy::g-signal:
   * @proxy: The #GDBusProxy emitting the signal.
   * @sender_name: The sender of the signal or %NULL if the connection is not a bus connection.
   * @signal_name: The name of the signal.
   * @parameters: A #GVariant tuple with parameters for the signal.
   *
   * Emitted when a signal from the remote object and interface that @proxy is for, has been received.
   *
   * Since: 2.26
   */
  signals[SIGNAL_SIGNAL] = g_signal_new ("g-signal",
                                         G_TYPE_DBUS_PROXY,
                                         G_SIGNAL_RUN_LAST,
                                         G_STRUCT_OFFSET (GDBusProxyClass, g_signal),
                                         NULL,
                                         NULL,
                                         _gio_marshal_VOID__STRING_STRING_VARIANT,
                                         G_TYPE_NONE,
                                         3,
                                         G_TYPE_STRING,
                                         G_TYPE_STRING,
                                         G_TYPE_VARIANT);


  g_type_class_add_private (klass, sizeof (GDBusProxyPrivate));
}

static void
g_dbus_proxy_init (GDBusProxy *proxy)
{
  proxy->priv = G_TYPE_INSTANCE_GET_PRIVATE (proxy, G_TYPE_DBUS_PROXY, GDBusProxyPrivate);
  proxy->priv->properties = g_hash_table_new_full (g_str_hash,
                                                   g_str_equal,
                                                   g_free,
                                                   (GDestroyNotify) g_variant_unref);
}

/* ---------------------------------------------------------------------------------------------------- */

static gint
property_name_sort_func (const gchar **a,
                         const gchar **b)
{
  return g_strcmp0 (*a, *b);
}

/**
 * g_dbus_proxy_get_cached_property_names:
 * @proxy: A #GDBusProxy.
 *
 * Gets the names of all cached properties on @proxy.
 *
 * Returns: A %NULL-terminated array of strings or %NULL if @proxy has
 * no cached properties. Free the returned array with g_strfreev().
 *
 * Since: 2.26
 */
gchar **
g_dbus_proxy_get_cached_property_names (GDBusProxy  *proxy)
{
  gchar **names;
  GPtrArray *p;
  GHashTableIter iter;
  const gchar *key;

  g_return_val_if_fail (G_IS_DBUS_PROXY (proxy), NULL);

  names = NULL;
  if (g_hash_table_size (proxy->priv->properties) == 0)
    goto out;

  p = g_ptr_array_new ();

  g_hash_table_iter_init (&iter, proxy->priv->properties);
  while (g_hash_table_iter_next (&iter, (gpointer) &key, NULL))
    g_ptr_array_add (p, g_strdup (key));
  g_ptr_array_sort (p, (GCompareFunc) property_name_sort_func);
  g_ptr_array_add (p, NULL);

  names = (gchar **) g_ptr_array_free (p, FALSE);

 out:
  return names;
}

static const GDBusPropertyInfo *
lookup_property_info_or_warn (GDBusProxy  *proxy,
                              const gchar *property_name)
{
  const GDBusPropertyInfo *info;

  if (proxy->priv->expected_interface == NULL)
    return NULL;

  info = g_dbus_interface_info_lookup_property (proxy->priv->expected_interface, property_name);
  if (info == NULL)
    {
      g_warning ("Trying to lookup property %s which isn't in expected interface %s",
                 property_name,
                 proxy->priv->expected_interface->name);
    }

  return info;
}

/**
 * g_dbus_proxy_get_cached_property:
 * @proxy: A #GDBusProxy.
 * @property_name: Property name.
 *
 * Looks up the value for a property from the cache. This call does no
 * blocking IO.
 *
 * If @proxy has an expected interface (see
 * #GDBusProxy:g-interface-info), then @property_name (for existence)
 * is checked against it.
 *
 * Returns: A reference to the #GVariant instance that holds the value
 * for @property_name or %NULL if the value is not in the cache. The
 * returned reference must be freed with g_variant_unref().
 *
 * Since: 2.26
 */
GVariant *
g_dbus_proxy_get_cached_property (GDBusProxy   *proxy,
                                  const gchar  *property_name)
{
  GVariant *value;

  g_return_val_if_fail (G_IS_DBUS_PROXY (proxy), NULL);
  g_return_val_if_fail (property_name != NULL, NULL);

  value = g_hash_table_lookup (proxy->priv->properties, property_name);
  if (value == NULL)
    {
      const GDBusPropertyInfo *info;
      info = lookup_property_info_or_warn (proxy, property_name);
      /* no difference */
      goto out;
    }

  g_variant_ref (value);

 out:
  return value;
}

/**
 * g_dbus_proxy_set_cached_property:
 * @proxy: A #GDBusProxy
 * @property_name: Property name.
 * @value: Value for the property or %NULL to remove it from the cache.
 *
 * If @value is not %NULL, sets the cached value for the property with
 * name @property_name to the value in @value.
 *
 * If @value is %NULL, then the cached value is removed from the
 * property cache.
 *
 * If @proxy has an expected interface (see
 * #GDBusProxy:g-interface-info), then @property_name (for existence)
 * and @value (for the type) is checked against it.
 *
 * If the @value #GVariant is floating, it is consumed. This allows
 * convenient 'inline' use of g_variant_new(), e.g.
 * |[
 *  g_dbus_proxy_set_cached_property (proxy,
 *                                    "SomeProperty",
 *                                    g_variant_new ("(si)",
 *                                                  "A String",
 *                                                  42));
 * ]|
 *
 * Normally you will not need to use this method since @proxy is
 * tracking changes using the
 * <literal>org.freedesktop.DBus.Properties.PropertiesChanged</literal>
 * D-Bus signal. However, for performance reasons an object may decide
 * to not use this signal for some properties and instead use a
 * proprietary out-of-band mechanism to transmit changes.
 *
 * As a concrete example, consider an object with a property
 * <literal>ChatroomParticipants</literal> which is an array of
 * strings. Instead of transmitting the same (long) array every time
 * the property changes, it is more efficient to only transmit the
 * delta using e.g. signals <literal>ChatroomParticipantJoined(String
 * name)</literal> and <literal>ChatroomParticipantParted(String
 * name)</literal>.
 *
 * Since: 2.26
 */
void
g_dbus_proxy_set_cached_property (GDBusProxy   *proxy,
                                  const gchar  *property_name,
                                  GVariant     *value)
{
  const GDBusPropertyInfo *info;

  g_return_if_fail (G_IS_DBUS_PROXY (proxy));
  g_return_if_fail (property_name != NULL);

  if (value != NULL)
    {
      info = lookup_property_info_or_warn (proxy, property_name);
      if (info != NULL)
        {
          if (g_strcmp0 (info->signature, g_variant_get_type_string (value)) != 0)
            {
              g_warning (_("Trying to set property %s of type %s but according to the expected "
                           "interface the type is %s"),
                         property_name,
                         g_variant_get_type_string (value),
                         info->signature);
              goto out;
            }
        }
      g_hash_table_insert (proxy->priv->properties,
                           g_strdup (property_name),
                           g_variant_ref_sink (value));
    }
  else
    {
      g_hash_table_remove (proxy->priv->properties, property_name);
    }

 out:
  ;
}

/* ---------------------------------------------------------------------------------------------------- */

static void
on_signal_received (GDBusConnection *connection,
                    const gchar     *sender_name,
                    const gchar     *object_path,
                    const gchar     *interface_name,
                    const gchar     *signal_name,
                    GVariant        *parameters,
                    gpointer         user_data)
{
  GDBusProxy *proxy = G_DBUS_PROXY (user_data);

  if (!proxy->priv->initialized)
    goto out;

  if (proxy->priv->name_owner != NULL && g_strcmp0 (sender_name, proxy->priv->name_owner) != 0)
    goto out;

  g_signal_emit (proxy,
                 signals[SIGNAL_SIGNAL],
                 0,
                 sender_name,
                 signal_name,
                 parameters);
 out:
  ;
}

/* ---------------------------------------------------------------------------------------------------- */

static void
on_properties_changed (GDBusConnection *connection,
                       const gchar     *sender_name,
                       const gchar     *object_path,
                       const gchar     *interface_name,
                       const gchar     *signal_name,
                       GVariant        *parameters,
                       gpointer         user_data)
{
  GDBusProxy *proxy = G_DBUS_PROXY (user_data);
  GError *error;
  const gchar *interface_name_for_signal;
  GVariant *changed_properties;
  gchar **invalidated_properties;
  GVariantIter iter;
  gchar *key;
  GVariant *value;
  guint n;

  error = NULL;
  changed_properties = NULL;
  invalidated_properties = NULL;

  if (!proxy->priv->initialized)
    goto out;

  if (proxy->priv->name_owner != NULL && g_strcmp0 (sender_name, proxy->priv->name_owner) != 0)
    goto out;

  if (!g_variant_is_of_type (parameters, G_VARIANT_TYPE ("(sa{sv}as)")))
    {
      g_warning ("Value for PropertiesChanged signal with type `%s' does not match `(sa{sv}as)'",
                 g_variant_get_type_string (parameters));
      goto out;
    }

  g_variant_get (parameters,
                 "(&s@a{sv}^a&s)",
                 &interface_name_for_signal,
                 &changed_properties,
                 &invalidated_properties);

  if (g_strcmp0 (interface_name_for_signal, proxy->priv->interface_name) != 0)
    goto out;

  g_variant_iter_init (&iter, changed_properties);
  while (g_variant_iter_next (&iter, "{sv}", &key, &value))
    {
      g_hash_table_insert (proxy->priv->properties,
                           key, /* adopts string */
                           value); /* adopts value */
    }

  for (n = 0; invalidated_properties[n] != NULL; n++)
    {
      g_hash_table_remove (proxy->priv->properties, invalidated_properties[n]);
    }

  /* emit signal */
  g_signal_emit (proxy, signals[PROPERTIES_CHANGED_SIGNAL],
                 0,
                 changed_properties,
                 invalidated_properties);

 out:
  if (changed_properties != NULL)
    g_variant_unref (changed_properties);
  g_free (invalidated_properties);
}

/* ---------------------------------------------------------------------------------------------------- */

static void
process_get_all_reply (GDBusProxy *proxy,
                       GVariant   *result)
{
  GVariantIter *iter;
  gchar *key;
  GVariant *value;

  if (!g_variant_is_of_type (result, G_VARIANT_TYPE ("(a{sv})")))
    {
      g_warning ("Value for GetAll reply with type `%s' does not match `(a{sv})'",
                 g_variant_get_type_string (result));
      goto out;
    }

  g_variant_get (result, "(a{sv})", &iter);
  while (g_variant_iter_next (iter, "{sv}", &key, &value))
    {
      g_hash_table_insert (proxy->priv->properties,
                           key, /* adopts string */
                           value); /* adopts value */
    }
  g_variant_iter_free (iter);

  /* Synthesize ::g-properties-changed changed */
  if (g_hash_table_size (proxy->priv->properties) > 0)
    {
      GVariant *changed_properties;
      const gchar *invalidated_properties[1] = {NULL};

      g_variant_get (result,
                     "(@a{sv})",
                     &changed_properties);
      g_signal_emit (proxy, signals[PROPERTIES_CHANGED_SIGNAL],
                     0,
                     changed_properties,
                     invalidated_properties);
      g_variant_unref (changed_properties);
    }

 out:
  ;
}

typedef struct
{
  GDBusProxy *proxy;
  GCancellable *cancellable;
  gchar *name_owner;
} LoadPropertiesOnNameOwnerChangedData;

static void
on_name_owner_changed_get_all_cb (GDBusConnection *connection,
                                  GAsyncResult    *res,
                                  gpointer         user_data)
{
  LoadPropertiesOnNameOwnerChangedData *data = user_data;
  GVariant *result;
  GError *error;
  gboolean cancelled;

  cancelled = FALSE;

  error = NULL;
  result = g_dbus_connection_call_finish (connection,
                                          res,
                                          &error);
  if (result == NULL)
    {
      if (error->domain == G_IO_ERROR && error->code == G_IO_ERROR_CANCELLED)
        cancelled = TRUE;
      /* We just ignore if GetAll() is failing. Because this might happen
       * if the object has no properties at all. Or if the caller is
       * not authorized to see the properties.
       *
       * Either way, apps can know about this by using
       * get_cached_property_names() or get_cached_property().
       *
       * TODO: handle G_DBUS_DEBUG flag 'proxy' and, if enabled, log the
       * fact that GetAll() failed
       */
      //g_debug ("error: %d %d %s", error->domain, error->code, error->message);
      g_error_free (error);
    }

  /* and finally we can notify */
  if (!cancelled)
    {
      g_free (data->proxy->priv->name_owner);
      data->proxy->priv->name_owner = data->name_owner;
      data->name_owner = NULL; /* to avoid an extra copy, we steal the string */

      g_hash_table_remove_all (data->proxy->priv->properties);
      if (result != NULL)
        {
          process_get_all_reply (data->proxy, result);
          g_variant_unref (result);
        }

      g_object_notify (G_OBJECT (data->proxy), "g-name-owner");
    }

  if (data->cancellable == data->proxy->priv->get_all_cancellable)
    data->proxy->priv->get_all_cancellable = NULL;

  g_object_unref (data->proxy);
  g_object_unref (data->cancellable);
  g_free (data->name_owner);
  g_free (data);
}

static void
on_name_owner_changed (GDBusConnection *connection,
                       const gchar      *sender_name,
                       const gchar      *object_path,
                       const gchar      *interface_name,
                       const gchar      *signal_name,
                       GVariant         *parameters,
                       gpointer          user_data)
{
  GDBusProxy *proxy = G_DBUS_PROXY (user_data);
  const gchar *old_owner;
  const gchar *new_owner;

  /* if we are already trying to load properties, cancel that */
  if (proxy->priv->get_all_cancellable != NULL)
    {
      g_cancellable_cancel (proxy->priv->get_all_cancellable);
      proxy->priv->get_all_cancellable = NULL;
    }

  g_variant_get (parameters,
                 "(&s&s&s)",
                 NULL,
                 &old_owner,
                 &new_owner);

  if (strlen (new_owner) == 0)
    {
      g_free (proxy->priv->name_owner);
      proxy->priv->name_owner = NULL;

      /* Synthesize ::g-properties-changed changed */
      if (!(proxy->priv->flags & G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES) &&
          g_hash_table_size (proxy->priv->properties) > 0)
        {
          GVariantBuilder builder;
          GVariant *changed_properties;
          GPtrArray *invalidated_properties;
          GHashTableIter iter;
          const gchar *key;

          /* Build changed_properties (always empty) and invalidated_properties ... */
          g_variant_builder_init (&builder, G_VARIANT_TYPE ("a{sv}"));
          changed_properties = g_variant_builder_end (&builder);
          invalidated_properties = g_ptr_array_new_with_free_func (g_free);
          g_hash_table_iter_init (&iter, proxy->priv->properties);
          while (g_hash_table_iter_next (&iter, (gpointer) &key, NULL))
            g_ptr_array_add (invalidated_properties, g_strdup (key));
          g_ptr_array_add (invalidated_properties, NULL);

          /* ... throw out the properties ... */
          g_hash_table_remove_all (proxy->priv->properties);

          /* ... and finally emit the ::g-properties-changed signal */
          g_signal_emit (proxy, signals[PROPERTIES_CHANGED_SIGNAL],
                         0,
                         changed_properties,
                         (const gchar* const *) invalidated_properties->pdata);
          g_variant_unref (changed_properties);
          g_ptr_array_unref (invalidated_properties);
        }
      g_object_notify (G_OBJECT (proxy), "g-name-owner");
    }
  else
    {
      /* ignore duplicates - this can happen when activating the service */
      if (g_strcmp0 (new_owner, proxy->priv->name_owner) == 0)
        goto out;

      if (proxy->priv->flags & G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES)
        {
          g_free (proxy->priv->name_owner);
          proxy->priv->name_owner = g_strdup (new_owner);
          g_hash_table_remove_all (proxy->priv->properties);
          g_object_notify (G_OBJECT (proxy), "g-name-owner");
        }
      else
        {
          LoadPropertiesOnNameOwnerChangedData *data;

          /* start loading properties.. only then emit notify::g-name-owner .. we
           * need to be able to cancel this in the event another NameOwnerChanged
           * signal suddenly happens
           */

          g_assert (proxy->priv->get_all_cancellable == NULL);
          proxy->priv->get_all_cancellable = g_cancellable_new ();
          data = g_new0 (LoadPropertiesOnNameOwnerChangedData, 1);
          data->proxy = g_object_ref (proxy);
          data->cancellable = proxy->priv->get_all_cancellable;
          data->name_owner = g_strdup (new_owner);
          g_dbus_connection_call (proxy->priv->connection,
                                  data->name_owner,
                                  proxy->priv->object_path,
                                  "org.freedesktop.DBus.Properties",
                                  "GetAll",
                                  g_variant_new ("(s)", proxy->priv->interface_name),
                                  G_VARIANT_TYPE ("(a{sv})"),
                                  G_DBUS_CALL_FLAGS_NONE,
                                  -1,           /* timeout */
                                  proxy->priv->get_all_cancellable,
                                  (GAsyncReadyCallback) on_name_owner_changed_get_all_cb,
                                  data);
        }
    }

 out:
  ;
}

/* ---------------------------------------------------------------------------------------------------- */

typedef struct
{
  GDBusProxy *proxy;
  GCancellable *cancellable;
  GSimpleAsyncResult *simple;
} AsyncInitData;

static void
async_init_data_free (AsyncInitData *data)
{
  g_object_unref (data->proxy);
  if (data->cancellable != NULL)
    g_object_unref (data->cancellable);
  g_object_unref (data->simple);
  g_free (data);
}

static void
async_init_get_all_cb (GDBusConnection *connection,
                       GAsyncResult    *res,
                       gpointer         user_data)
{
  AsyncInitData *data = user_data;
  GVariant *result;
  GError *error;

  error = NULL;
  result = g_dbus_connection_call_finish (connection,
                                          res,
                                          &error);
  if (result == NULL)
    {
      /* We just ignore if GetAll() is failing. Because this might happen
       * if the object has no properties at all. Or if the caller is
       * not authorized to see the properties.
       *
       * Either way, apps can know about this by using
       * get_cached_property_names() or get_cached_property().
       *
       * TODO: handle G_DBUS_DEBUG flag 'proxy' and, if enabled, log the
       * fact that GetAll() failed
       */
      //g_debug ("error: %d %d %s", error->domain, error->code, error->message);
      g_error_free (error);
    }
  else
    {
      g_simple_async_result_set_op_res_gpointer (data->simple,
                                                 result,
                                                 (GDestroyNotify) g_variant_unref);
    }

  g_simple_async_result_complete_in_idle (data->simple);
  async_init_data_free (data);
}


static void
async_init_get_name_owner_cb (GDBusConnection *connection,
                              GAsyncResult    *res,
                              gpointer         user_data)
{
  AsyncInitData *data = user_data;

  if (res != NULL)
    {
      GError *error;
      GVariant *result;

      error = NULL;
      result = g_dbus_connection_call_finish (connection,
                                              res,
                                              &error);
      if (result == NULL)
        {
          if (error->domain == G_DBUS_ERROR &&
              error->code == G_DBUS_ERROR_NAME_HAS_NO_OWNER)
            {
              g_error_free (error);
            }
          else
            {
              g_simple_async_result_set_from_error (data->simple, error);
              g_error_free (error);
              g_simple_async_result_complete_in_idle (data->simple);
              async_init_data_free (data);
              goto out;
            }
        }
      else
        {
          g_variant_get (result,
                         "(s)",
                         &data->proxy->priv->name_owner);
          g_variant_unref (result);
        }
    }

  if (!(data->proxy->priv->flags & G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES))
    {
      /* load all properties asynchronously */
      g_dbus_connection_call (data->proxy->priv->connection,
                              data->proxy->priv->name_owner,
                              data->proxy->priv->object_path,
                              "org.freedesktop.DBus.Properties",
                              "GetAll",
                              g_variant_new ("(s)", data->proxy->priv->interface_name),
                              G_VARIANT_TYPE ("(a{sv})"),
                              G_DBUS_CALL_FLAGS_NONE,
                              -1,           /* timeout */
                              data->cancellable,
                              (GAsyncReadyCallback) async_init_get_all_cb,
                              data);
    }
  else
    {
      g_simple_async_result_complete_in_idle (data->simple);
      async_init_data_free (data);
    }

 out:
  ;
}

static void
async_init_call_get_name_owner (AsyncInitData *data)
{
  g_dbus_connection_call (data->proxy->priv->connection,
                          "org.freedesktop.DBus",  /* name */
                          "/org/freedesktop/DBus", /* object path */
                          "org.freedesktop.DBus",  /* interface */
                          "GetNameOwner",
                          g_variant_new ("(s)",
                                         data->proxy->priv->name),
                          G_VARIANT_TYPE ("(s)"),
                          G_DBUS_CALL_FLAGS_NONE,
                          -1,           /* timeout */
                          data->cancellable,
                          (GAsyncReadyCallback) async_init_get_name_owner_cb,
                          data);
}

static void
async_init_start_service_by_name_cb (GDBusConnection *connection,
                                     GAsyncResult    *res,
                                     gpointer         user_data)
{
  AsyncInitData *data = user_data;
  GError *error;
  GVariant *result;

  error = NULL;
  result = g_dbus_connection_call_finish (connection,
                                          res,
                                          &error);
  if (result == NULL)
    {
      /* Errors are not unexpected; the bus will reply e.g.
       *
       *   org.freedesktop.DBus.Error.ServiceUnknown: The name org.gnome.Epiphany2
       *   was not provided by any .service files
       *
       * This doesn't mean that the name doesn't have an owner, just
       * that it's not provided by a .service file. So just proceed to
       * invoke GetNameOwner() if dealing with that error.
       */
      if (error->domain == G_DBUS_ERROR &&
          error->code == G_DBUS_ERROR_SERVICE_UNKNOWN)
        {
          g_error_free (error);
        }
      else
        {
          g_prefix_error (&error,
                          _("Error calling StartServiceByName for %s: "),
                          data->proxy->priv->name);
          goto failed;
        }
    }
  else
    {
      guint32 start_service_result;
      g_variant_get (result,
                     "(u)",
                     &start_service_result);
      g_variant_unref (result);
      if (start_service_result == 1 ||  /* DBUS_START_REPLY_SUCCESS */
          start_service_result == 2)    /* DBUS_START_REPLY_ALREADY_RUNNING */
        {
          /* continue to invoke GetNameOwner() */
        }
      else
        {
          error = g_error_new (G_IO_ERROR,
                               G_IO_ERROR_FAILED,
                               _("Unexpected reply %d from StartServiceByName(\"%s\") method"),
                               start_service_result,
                               data->proxy->priv->name);
          goto failed;
        }
    }

  async_init_call_get_name_owner (data);
  return;

 failed:
  g_warn_if_fail (error != NULL);
  g_simple_async_result_set_from_error (data->simple, error);
  g_error_free (error);
  g_simple_async_result_complete_in_idle (data->simple);
  async_init_data_free (data);
}

static void
async_init_call_start_service_by_name (AsyncInitData *data)
{
  g_dbus_connection_call (data->proxy->priv->connection,
                          "org.freedesktop.DBus",  /* name */
                          "/org/freedesktop/DBus", /* object path */
                          "org.freedesktop.DBus",  /* interface */
                          "StartServiceByName",
                          g_variant_new ("(su)",
                                         data->proxy->priv->name,
                                         0),
                          G_VARIANT_TYPE ("(u)"),
                          G_DBUS_CALL_FLAGS_NONE,
                          -1,           /* timeout */
                          data->cancellable,
                          (GAsyncReadyCallback) async_init_start_service_by_name_cb,
                          data);
}

static void
async_initable_init_second_async (GAsyncInitable      *initable,
                                  gint                 io_priority,
                                  GCancellable        *cancellable,
                                  GAsyncReadyCallback  callback,
                                  gpointer             user_data)
{
  GDBusProxy *proxy = G_DBUS_PROXY (initable);
  AsyncInitData *data;

  data = g_new0 (AsyncInitData, 1);
  data->proxy = g_object_ref (proxy);
  data->cancellable = cancellable != NULL ? g_object_ref (cancellable) : NULL;
  data->simple = g_simple_async_result_new (G_OBJECT (proxy),
                                            callback,
                                            user_data,
                                            NULL);

  /* Check name ownership asynchronously - possibly also start the service */
  if (proxy->priv->name == NULL)
    {
      /* Do nothing */
      async_init_get_name_owner_cb (proxy->priv->connection, NULL, data);
    }
  else if (g_dbus_is_unique_name (proxy->priv->name))
    {
      proxy->priv->name_owner = g_strdup (proxy->priv->name);
      async_init_get_name_owner_cb (proxy->priv->connection, NULL, data);
    }
  else
    {
      if (proxy->priv->flags & G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START)
        {
          async_init_call_get_name_owner (data);
        }
      else
        {
          async_init_call_start_service_by_name (data);
        }
    }
}

static gboolean
async_initable_init_second_finish (GAsyncInitable  *initable,
                                   GAsyncResult    *res,
                                   GError         **error)
{
  GDBusProxy *proxy = G_DBUS_PROXY (initable);
  GSimpleAsyncResult *simple = G_SIMPLE_ASYNC_RESULT (res);
  GVariant *result;
  gboolean ret;

  ret = FALSE;

  if (g_simple_async_result_propagate_error (simple, error))
    goto out;

  result = g_simple_async_result_get_op_res_gpointer (simple);
  if (result != NULL)
    {
      process_get_all_reply (proxy, result);
    }

  ret = TRUE;

 out:
  proxy->priv->initialized = TRUE;
  return ret;
}

/* ---------------------------------------------------------------------------------------------------- */

static void
async_initable_init_first (GAsyncInitable *initable)
{
  GDBusProxy *proxy = G_DBUS_PROXY (initable);

  if (!(proxy->priv->flags & G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES))
    {
      /* subscribe to PropertiesChanged() */
      proxy->priv->properties_changed_subscriber_id =
        g_dbus_connection_signal_subscribe (proxy->priv->connection,
                                            proxy->priv->name,
                                            "org.freedesktop.DBus.Properties",
                                            "PropertiesChanged",
                                            proxy->priv->object_path,
                                            proxy->priv->interface_name,
                                            G_DBUS_SIGNAL_FLAGS_NONE,
                                            on_properties_changed,
                                            proxy,
                                            NULL);
    }

  if (!(proxy->priv->flags & G_DBUS_PROXY_FLAGS_DO_NOT_CONNECT_SIGNALS))
    {
      /* subscribe to all signals for the object */
      proxy->priv->signals_subscriber_id =
        g_dbus_connection_signal_subscribe (proxy->priv->connection,
                                            proxy->priv->name,
                                            proxy->priv->interface_name,
                                            NULL,                        /* member */
                                            proxy->priv->object_path,
                                            NULL,                        /* arg0 */
                                            G_DBUS_SIGNAL_FLAGS_NONE,
                                            on_signal_received,
                                            proxy,
                                            NULL);
    }

  if (proxy->priv->name != NULL && !g_dbus_is_unique_name (proxy->priv->name))
    {
      proxy->priv->name_owner_changed_subscription_id =
        g_dbus_connection_signal_subscribe (proxy->priv->connection,
                                            "org.freedesktop.DBus",  /* name */
                                            "org.freedesktop.DBus",  /* interface */
                                            "NameOwnerChanged",      /* signal name */
                                            "/org/freedesktop/DBus", /* path */
                                            proxy->priv->name,       /* arg0 */
                                            G_DBUS_SIGNAL_FLAGS_NONE,
                                            on_name_owner_changed,
                                            proxy,
                                            NULL);
    }
}

/* ---------------------------------------------------------------------------------------------------- */

/* initialization is split into two parts - the first is the
 * non-blocing part that requires the callers GMainContext - the
 * second is a blocking part async part that doesn't require the
 * callers GMainContext.. we do this split so the code can be reused
 * in the GInitable implementation below.
 *
 * Note that obtaining a GDBusConnection is not shared between the two
 * paths.
 */

typedef struct
{
  GDBusProxy          *proxy;
  gint                 io_priority;
  GCancellable        *cancellable;
  GAsyncReadyCallback  callback;
  gpointer             user_data;
} GetConnectionData;

static void
get_connection_cb (GObject       *source_object,
                   GAsyncResult  *res,
                   gpointer       user_data)
{
  GetConnectionData *data = user_data;
  GError *error;

  error = NULL;
  data->proxy->priv->connection = g_bus_get_finish (res, &error);
  if (data->proxy->priv->connection == NULL)
    {
      GSimpleAsyncResult *simple;
      simple = g_simple_async_result_new (G_OBJECT (data->proxy),
                                          data->callback,
                                          data->user_data,
                                          NULL);
      g_simple_async_result_set_from_error (simple, error);
      g_error_free (error);
      g_simple_async_result_complete_in_idle (simple);
      g_object_unref (simple);
    }
  else
    {
      async_initable_init_first (G_ASYNC_INITABLE (data->proxy));
      async_initable_init_second_async (G_ASYNC_INITABLE (data->proxy),
                                        data->io_priority,
                                        data->cancellable,
                                        data->callback,
                                        data->user_data);
    }

  if (data->cancellable != NULL)
    g_object_unref (data->cancellable);
  if (data->proxy != NULL)
    g_object_unref (data->proxy);
  g_free (data);
}

static void
async_initable_init_async (GAsyncInitable      *initable,
                           gint                 io_priority,
                           GCancellable        *cancellable,
                           GAsyncReadyCallback  callback,
                           gpointer             user_data)
{
  GDBusProxy *proxy = G_DBUS_PROXY (initable);

  if (proxy->priv->bus_type != G_BUS_TYPE_NONE)
    {
      GetConnectionData *data;

      g_assert (proxy->priv->connection == NULL);

      data = g_new0 (GetConnectionData, 1);
      data->proxy = g_object_ref (proxy);
      data->io_priority = io_priority;
      data->cancellable = cancellable != NULL ? g_object_ref (cancellable) : NULL;
      data->callback = callback;
      data->user_data = user_data;
      g_bus_get (proxy->priv->bus_type,
                 cancellable,
                 get_connection_cb,
                 data);
    }
  else
    {
      async_initable_init_first (initable);
      async_initable_init_second_async (initable, io_priority, cancellable, callback, user_data);
    }
}

static gboolean
async_initable_init_finish (GAsyncInitable  *initable,
                            GAsyncResult    *res,
                            GError         **error)
{
  return async_initable_init_second_finish (initable, res, error);
}

static void
async_initable_iface_init (GAsyncInitableIface *async_initable_iface)
{
  async_initable_iface->init_async = async_initable_init_async;
  async_initable_iface->init_finish = async_initable_init_finish;
}

/* ---------------------------------------------------------------------------------------------------- */

typedef struct
{
  GMainContext *context;
  GMainLoop *loop;
  GAsyncResult *res;
} InitableAsyncInitableData;

static void
async_initable_init_async_cb (GObject      *source_object,
                              GAsyncResult *res,
                              gpointer      user_data)
{
  InitableAsyncInitableData *data = user_data;
  data->res = g_object_ref (res);
  g_main_loop_quit (data->loop);
}

/* Simply reuse the GAsyncInitable implementation but run the first
 * part (that is non-blocking and requires the callers GMainContext)
 * with the callers GMainContext.. and the second with a private
 * GMainContext (bug 621310 is slightly related).
 *
 * Note that obtaining a GDBusConnection is not shared between the two
 * paths.
 */
static gboolean
initable_init (GInitable     *initable,
               GCancellable  *cancellable,
               GError       **error)
{
  GDBusProxy *proxy = G_DBUS_PROXY (initable);
  InitableAsyncInitableData *data;
  gboolean ret;

  ret = FALSE;

  if (proxy->priv->bus_type != G_BUS_TYPE_NONE)
    {
      g_assert (proxy->priv->connection == NULL);
      proxy->priv->connection = g_bus_get_sync (proxy->priv->bus_type,
                                                cancellable,
                                                error);
      if (proxy->priv->connection == NULL)
        goto out;
    }

  async_initable_init_first (G_ASYNC_INITABLE (initable));

  data = g_new0 (InitableAsyncInitableData, 1);
  data->context = g_main_context_new ();
  data->loop = g_main_loop_new (data->context, FALSE);

  g_main_context_push_thread_default (data->context);

  async_initable_init_second_async (G_ASYNC_INITABLE (initable),
                                    G_PRIORITY_DEFAULT,
                                    cancellable,
                                    async_initable_init_async_cb,
                                    data);

  g_main_loop_run (data->loop);

  ret = async_initable_init_second_finish (G_ASYNC_INITABLE (initable),
                                           data->res,
                                           error);

  g_main_context_pop_thread_default (data->context);

  g_main_context_unref (data->context);
  g_main_loop_unref (data->loop);
  g_object_unref (data->res);
  g_free (data);

 out:

  return ret;
}

static void
initable_iface_init (GInitableIface *initable_iface)
{
  initable_iface->init = initable_init;
}

/* ---------------------------------------------------------------------------------------------------- */

/**
 * g_dbus_proxy_new:
 * @connection: A #GDBusConnection.
 * @flags: Flags used when constructing the proxy.
 * @info: A #GDBusInterfaceInfo specifying the minimal interface that @proxy conforms to or %NULL.
 * @name: A bus name (well-known or unique) or %NULL if @connection is not a message bus connection.
 * @object_path: An object path.
 * @interface_name: A D-Bus interface name.
 * @cancellable: A #GCancellable or %NULL.
 * @callback: Callback function to invoke when the proxy is ready.
 * @user_data: User data to pass to @callback.
 *
 * Creates a proxy for accessing @interface_name on the remote object
 * at @object_path owned by @name at @connection and asynchronously
 * loads D-Bus properties unless the
 * %G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES flag is used. Connect to
 * the #GDBusProxy::g-properties-changed signal to get notified about
 * property changes.
 *
 * If the %G_DBUS_PROXY_FLAGS_DO_NOT_CONNECT_SIGNALS flag is not set, also sets up
 * match rules for signals. Connect to the #GDBusProxy::g-signal signal
 * to handle signals from the remote object.
 *
 * If @name is a well-known name and the
 * %G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START flag isn't set and no name
 * owner currently exists, the message bus will be requested to launch
 * a name owner for the name.
 *
 * This is a failable asynchronous constructor - when the proxy is
 * ready, @callback will be invoked and you can use
 * g_dbus_proxy_new_finish() to get the result.
 *
 * See g_dbus_proxy_new_sync() and for a synchronous version of this constructor.
 *
 * See <xref linkend="gdbus-wellknown-proxy"/> for an example of how #GDBusProxy can be used.
 *
 * Since: 2.26
 */
void
g_dbus_proxy_new (GDBusConnection     *connection,
                  GDBusProxyFlags      flags,
                  GDBusInterfaceInfo  *info,
                  const gchar         *name,
                  const gchar         *object_path,
                  const gchar         *interface_name,
                  GCancellable        *cancellable,
                  GAsyncReadyCallback  callback,
                  gpointer             user_data)
{
  g_return_if_fail (G_IS_DBUS_CONNECTION (connection));
  g_return_if_fail ((name == NULL && g_dbus_connection_get_unique_name (connection) == NULL) || g_dbus_is_name (name));
  g_return_if_fail (g_variant_is_object_path (object_path));
  g_return_if_fail (g_dbus_is_interface_name (interface_name));

  g_async_initable_new_async (G_TYPE_DBUS_PROXY,
                              G_PRIORITY_DEFAULT,
                              cancellable,
                              callback,
                              user_data,
                              "g-flags", flags,
                              "g-interface-info", info,
                              "g-name", name,
                              "g-connection", connection,
                              "g-object-path", object_path,
                              "g-interface-name", interface_name,
                              NULL);
}

/**
 * g_dbus_proxy_new_finish:
 * @res: A #GAsyncResult obtained from the #GAsyncReadyCallback function passed to g_dbus_proxy_new().
 * @error: Return location for error or %NULL.
 *
 * Finishes creating a #GDBusProxy.
 *
 * Returns: A #GDBusProxy or %NULL if @error is set. Free with g_object_unref().
 *
 * Since: 2.26
 */
GDBusProxy *
g_dbus_proxy_new_finish (GAsyncResult  *res,
                         GError       **error)
{
  GObject *object;
  GObject *source_object;

  source_object = g_async_result_get_source_object (res);
  g_assert (source_object != NULL);

  object = g_async_initable_new_finish (G_ASYNC_INITABLE (source_object),
                                        res,
                                        error);
  g_object_unref (source_object);

  if (object != NULL)
    return G_DBUS_PROXY (object);
  else
    return NULL;
}

/**
 * g_dbus_proxy_new_sync:
 * @connection: A #GDBusConnection.
 * @flags: Flags used when constructing the proxy.
 * @info: (allow-none): A #GDBusInterfaceInfo specifying the minimal interface that @proxy conforms to or %NULL.
 * @name: (allow-none): A bus name (well-known or unique) or %NULL if @connection is not a message bus connection.
 * @object_path: An object path.
 * @interface_name: A D-Bus interface name.
 * @cancellable: (allow-none): A #GCancellable or %NULL.
 * @error: (allow-none): Return location for error or %NULL.
 *
 * Creates a proxy for accessing @interface_name on the remote object
 * at @object_path owned by @name at @connection and synchronously
 * loads D-Bus properties unless the
 * %G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES flag is used.
 *
 * If the %G_DBUS_PROXY_FLAGS_DO_NOT_CONNECT_SIGNALS flag is not set, also sets up
 * match rules for signals. Connect to the #GDBusProxy::g-signal signal
 * to handle signals from the remote object.
 *
 * If @name is a well-known name and the
 * %G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START flag isn't set and no name
 * owner currently exists, the message bus will be requested to launch
 * a name owner for the name.
 *
 * This is a synchronous failable constructor. See g_dbus_proxy_new()
 * and g_dbus_proxy_new_finish() for the asynchronous version.
 *
 * See <xref linkend="gdbus-wellknown-proxy"/> for an example of how #GDBusProxy can be used.
 *
 * Returns: A #GDBusProxy or %NULL if error is set. Free with g_object_unref().
 *
 * Since: 2.26
 */
GDBusProxy *
g_dbus_proxy_new_sync (GDBusConnection     *connection,
                       GDBusProxyFlags      flags,
                       GDBusInterfaceInfo  *info,
                       const gchar         *name,
                       const gchar         *object_path,
                       const gchar         *interface_name,
                       GCancellable        *cancellable,
                       GError             **error)
{
  GInitable *initable;

  g_return_val_if_fail (G_IS_DBUS_CONNECTION (connection), NULL);
  g_return_val_if_fail ((name == NULL && g_dbus_connection_get_unique_name (connection) == NULL) ||
                        g_dbus_is_name (name), NULL);
  g_return_val_if_fail (g_variant_is_object_path (object_path), NULL);
  g_return_val_if_fail (g_dbus_is_interface_name (interface_name), NULL);

  initable = g_initable_new (G_TYPE_DBUS_PROXY,
                             cancellable,
                             error,
                             "g-flags", flags,
                             "g-interface-info", info,
                             "g-name", name,
                             "g-connection", connection,
                             "g-object-path", object_path,
                             "g-interface-name", interface_name,
                             NULL);
  if (initable != NULL)
    return G_DBUS_PROXY (initable);
  else
    return NULL;
}

/* ---------------------------------------------------------------------------------------------------- */

/**
 * g_dbus_proxy_new_for_bus:
 * @bus_type: A #GBusType.
 * @flags: Flags used when constructing the proxy.
 * @info: A #GDBusInterfaceInfo specifying the minimal interface that @proxy conforms to or %NULL.
 * @name: A bus name (well-known or unique).
 * @object_path: An object path.
 * @interface_name: A D-Bus interface name.
 * @cancellable: A #GCancellable or %NULL.
 * @callback: Callback function to invoke when the proxy is ready.
 * @user_data: User data to pass to @callback.
 *
 * Like g_dbus_proxy_new() but takes a #GBusType instead of a #GDBusConnection.
 *
 * See <xref linkend="gdbus-wellknown-proxy"/> for an example of how #GDBusProxy can be used.
 *
 * Since: 2.26
 */
void
g_dbus_proxy_new_for_bus (GBusType             bus_type,
                          GDBusProxyFlags      flags,
                          GDBusInterfaceInfo  *info,
                          const gchar         *name,
                          const gchar         *object_path,
                          const gchar         *interface_name,
                          GCancellable        *cancellable,
                          GAsyncReadyCallback  callback,
                          gpointer             user_data)
{
  g_return_if_fail (g_dbus_is_name (name));
  g_return_if_fail (g_variant_is_object_path (object_path));
  g_return_if_fail (g_dbus_is_interface_name (interface_name));

  g_async_initable_new_async (G_TYPE_DBUS_PROXY,
                              G_PRIORITY_DEFAULT,
                              cancellable,
                              callback,
                              user_data,
                              "g-flags", flags,
                              "g-interface-info", info,
                              "g-name", name,
                              "g-bus-type", bus_type,
                              "g-object-path", object_path,
                              "g-interface-name", interface_name,
                              NULL);
}

/**
 * g_dbus_proxy_new_for_bus_finish:
 * @res: A #GAsyncResult obtained from the #GAsyncReadyCallback function passed to g_dbus_proxy_new_for_bus().
 * @error: Return location for error or %NULL.
 *
 * Finishes creating a #GDBusProxy.
 *
 * Returns: A #GDBusProxy or %NULL if @error is set. Free with g_object_unref().
 *
 * Since: 2.26
 */
GDBusProxy *
g_dbus_proxy_new_for_bus_finish (GAsyncResult  *res,
                                 GError       **error)
{
  return g_dbus_proxy_new_finish (res, error);
}

/**
 * g_dbus_proxy_new_for_bus_sync:
 * @bus_type: A #GBusType.
 * @flags: Flags used when constructing the proxy.
 * @info: A #GDBusInterfaceInfo specifying the minimal interface that @proxy conforms to or %NULL.
 * @name: A bus name (well-known or unique).
 * @object_path: An object path.
 * @interface_name: A D-Bus interface name.
 * @cancellable: A #GCancellable or %NULL.
 * @error: Return location for error or %NULL.
 *
 * Like g_dbus_proxy_new_sync() but takes a #GBusType instead of a #GDBusConnection.
 *
 * See <xref linkend="gdbus-wellknown-proxy"/> for an example of how #GDBusProxy can be used.
 *
 * Returns: A #GDBusProxy or %NULL if error is set. Free with g_object_unref().
 *
 * Since: 2.26
 */
GDBusProxy *
g_dbus_proxy_new_for_bus_sync (GBusType             bus_type,
                               GDBusProxyFlags      flags,
                               GDBusInterfaceInfo  *info,
                               const gchar         *name,
                               const gchar         *object_path,
                               const gchar         *interface_name,
                               GCancellable        *cancellable,
                               GError             **error)
{
  GInitable *initable;

  g_return_val_if_fail (g_dbus_is_name (name), NULL);
  g_return_val_if_fail (g_variant_is_object_path (object_path), NULL);
  g_return_val_if_fail (g_dbus_is_interface_name (interface_name), NULL);

  initable = g_initable_new (G_TYPE_DBUS_PROXY,
                             cancellable,
                             error,
                             "g-flags", flags,
                             "g-interface-info", info,
                             "g-name", name,
                             "g-bus-type", bus_type,
                             "g-object-path", object_path,
                             "g-interface-name", interface_name,
                             NULL);
  if (initable != NULL)
    return G_DBUS_PROXY (initable);
  else
    return NULL;
}

/* ---------------------------------------------------------------------------------------------------- */

/**
 * g_dbus_proxy_get_connection:
 * @proxy: A #GDBusProxy.
 *
 * Gets the connection @proxy is for.
 *
 * Returns: A #GDBusConnection owned by @proxy. Do not free.
 *
 * Since: 2.26
 */
GDBusConnection *
g_dbus_proxy_get_connection (GDBusProxy *proxy)
{
  g_return_val_if_fail (G_IS_DBUS_PROXY (proxy), NULL);
  return proxy->priv->connection;
}

/**
 * g_dbus_proxy_get_flags:
 * @proxy: A #GDBusProxy.
 *
 * Gets the flags that @proxy was constructed with.
 *
 * Returns: Flags from the #GDBusProxyFlags enumeration.
 *
 * Since: 2.26
 */
GDBusProxyFlags
g_dbus_proxy_get_flags (GDBusProxy *proxy)
{
  g_return_val_if_fail (G_IS_DBUS_PROXY (proxy), 0);
  return proxy->priv->flags;
}

/**
 * g_dbus_proxy_get_name:
 * @proxy: A #GDBusProxy.
 *
 * Gets the name that @proxy was constructed for.
 *
 * Returns: A string owned by @proxy. Do not free.
 *
 * Since: 2.26
 */
const gchar *
g_dbus_proxy_get_name (GDBusProxy *proxy)
{
  g_return_val_if_fail (G_IS_DBUS_PROXY (proxy), NULL);
  return proxy->priv->name;
}

/**
 * g_dbus_proxy_get_name_owner:
 * @proxy: A #GDBusProxy.
 *
 * The unique name that owns the name that @proxy is for or %NULL if
 * no-one currently owns that name. You may connect to the
 * #GObject::notify signal to track changes to the
 * #GDBusProxy:g-name-owner property.
 *
 * Returns: The name owner or %NULL if no name owner exists. Free with g_free().
 *
 * Since: 2.26
 */
gchar *
g_dbus_proxy_get_name_owner (GDBusProxy *proxy)
{
  g_return_val_if_fail (G_IS_DBUS_PROXY (proxy), NULL);
  return g_strdup (proxy->priv->name_owner);
}

/**
 * g_dbus_proxy_get_object_path:
 * @proxy: A #GDBusProxy.
 *
 * Gets the object path @proxy is for.
 *
 * Returns: A string owned by @proxy. Do not free.
 *
 * Since: 2.26
 */
const gchar *
g_dbus_proxy_get_object_path (GDBusProxy *proxy)
{
  g_return_val_if_fail (G_IS_DBUS_PROXY (proxy), NULL);
  return proxy->priv->object_path;
}

/**
 * g_dbus_proxy_get_interface_name:
 * @proxy: A #GDBusProxy.
 *
 * Gets the D-Bus interface name @proxy is for.
 *
 * Returns: A string owned by @proxy. Do not free.
 *
 * Since: 2.26
 */
const gchar *
g_dbus_proxy_get_interface_name (GDBusProxy *proxy)
{
  g_return_val_if_fail (G_IS_DBUS_PROXY (proxy), NULL);
  return proxy->priv->interface_name;
}

/**
 * g_dbus_proxy_get_default_timeout:
 * @proxy: A #GDBusProxy.
 *
 * Gets the timeout to use if -1 (specifying default timeout) is
 * passed as @timeout_msec in the g_dbus_proxy_call() and
 * g_dbus_proxy_call_sync() functions.
 *
 * See the #GDBusProxy:g-default-timeout property for more details.
 *
 * Returns: Timeout to use for @proxy.
 *
 * Since: 2.26
 */
gint
g_dbus_proxy_get_default_timeout (GDBusProxy *proxy)
{
  g_return_val_if_fail (G_IS_DBUS_PROXY (proxy), -1);
  return proxy->priv->timeout_msec;
}

/**
 * g_dbus_proxy_set_default_timeout:
 * @proxy: A #GDBusProxy.
 * @timeout_msec: Timeout in milliseconds.
 *
 * Sets the timeout to use if -1 (specifying default timeout) is
 * passed as @timeout_msec in the g_dbus_proxy_call() and
 * g_dbus_proxy_call_sync() functions.
 *
 * See the #GDBusProxy:g-default-timeout property for more details.
 *
 * Since: 2.26
 */
void
g_dbus_proxy_set_default_timeout (GDBusProxy *proxy,
                                  gint        timeout_msec)
{
  g_return_if_fail (G_IS_DBUS_PROXY (proxy));
  g_return_if_fail (timeout_msec == -1 || timeout_msec >= 0);

  /* TODO: locking? */
  if (proxy->priv->timeout_msec != timeout_msec)
    {
      proxy->priv->timeout_msec = timeout_msec;
      g_object_notify (G_OBJECT (proxy), "g-default-timeout");
    }
}

/**
 * g_dbus_proxy_get_interface_info:
 * @proxy: A #GDBusProxy
 *
 * Returns the #GDBusInterfaceInfo, if any, specifying the minimal
 * interface that @proxy conforms to.
 *
 * See the #GDBusProxy:g-interface-info property for more details.
 *
 * Returns: A #GDBusInterfaceInfo or %NULL. Do not unref the returned
 * object, it is owned by @proxy.
 *
 * Since: 2.26
 */
GDBusInterfaceInfo *
g_dbus_proxy_get_interface_info (GDBusProxy *proxy)
{
  g_return_val_if_fail (G_IS_DBUS_PROXY (proxy), NULL);
  return proxy->priv->expected_interface;
}

/**
 * g_dbus_proxy_set_interface_info:
 * @proxy: A #GDBusProxy
 * @info: Minimum interface this proxy conforms to or %NULL to unset.
 *
 * Ensure that interactions with @proxy conform to the given
 * interface.  For example, when completing a method call, if the type
 * signature of the message isn't what's expected, the given #GError
 * is set.  Signals that have a type signature mismatch are simply
 * dropped.
 *
 * See the #GDBusProxy:g-interface-info property for more details.
 *
 * Since: 2.26
 */
void
g_dbus_proxy_set_interface_info (GDBusProxy         *proxy,
                                 GDBusInterfaceInfo *info)
{
  g_return_if_fail (G_IS_DBUS_PROXY (proxy));
  if (proxy->priv->expected_interface != NULL)
    g_dbus_interface_info_unref (proxy->priv->expected_interface);
  proxy->priv->expected_interface = info != NULL ? g_dbus_interface_info_ref (info) : NULL;
}

/* ---------------------------------------------------------------------------------------------------- */

static gboolean
maybe_split_method_name (const gchar  *method_name,
                         gchar       **out_interface_name,
                         const gchar **out_method_name)
{
  gboolean was_split;

  was_split = FALSE;
  g_assert (out_interface_name != NULL);
  g_assert (out_method_name != NULL);
  *out_interface_name = NULL;
  *out_method_name = NULL;

  if (strchr (method_name, '.') != NULL)
    {
      gchar *p;
      gchar *last_dot;

      p = g_strdup (method_name);
      last_dot = strrchr (p, '.');
      *last_dot = '\0';

      *out_interface_name = p;
      *out_method_name = last_dot + 1;

      was_split = TRUE;
    }

  return was_split;
}


static void
reply_cb (GDBusConnection *connection,
          GAsyncResult    *res,
          gpointer         user_data)
{
  GSimpleAsyncResult *simple = G_SIMPLE_ASYNC_RESULT (user_data);
  GVariant *value;
  GError *error;

  error = NULL;
  value = g_dbus_connection_call_finish (connection,
                                         res,
                                         &error);
  if (error != NULL)
    {
      g_simple_async_result_set_from_error (simple,
                                            error);
      g_error_free (error);
    }
  else
    {
      g_simple_async_result_set_op_res_gpointer (simple,
                                                 value,
                                                 (GDestroyNotify) g_variant_unref);
    }

  /* no need to complete in idle since the method GDBusConnection already does */
  g_simple_async_result_complete (simple);
  g_object_unref (simple);
}

static const GDBusMethodInfo *
lookup_method_info_or_warn (GDBusProxy  *proxy,
                            const gchar *method_name)
{
  const GDBusMethodInfo *info;

  if (proxy->priv->expected_interface == NULL)
    return NULL;

  info = g_dbus_interface_info_lookup_method (proxy->priv->expected_interface, method_name);
  if (info == NULL)
    {
      g_warning ("Trying to invoke method %s which isn't in expected interface %s",
                 method_name, proxy->priv->expected_interface->name);
    }

  return info;
}

static const gchar *
get_destination_for_call (GDBusProxy *proxy)
{
  const gchar *ret;

  ret = NULL;

  /* If proxy->priv->name is a unique name, then proxy->priv->name_owner
   * is never NULL and always the same as proxy->priv->name. We use this
   * knowledge to avoid checking if proxy->priv->name is a unique or
   * well-known name.
   */
  ret = proxy->priv->name_owner;
  if (ret != NULL)
    goto out;

  if (proxy->priv->flags & G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START)
    goto out;

  ret = proxy->priv->name;

 out:
  return ret;
}

/**
 * g_dbus_proxy_call:
 * @proxy: A #GDBusProxy.
 * @method_name: Name of method to invoke.
 * @parameters: A #GVariant tuple with parameters for the signal or %NULL if not passing parameters.
 * @flags: Flags from the #GDBusCallFlags enumeration.
 * @timeout_msec: The timeout in milliseconds or -1 to use the proxy default timeout.
 * @cancellable: A #GCancellable or %NULL.
 * @callback: A #GAsyncReadyCallback to call when the request is satisfied or %NULL if you don't
 * care about the result of the method invocation.
 * @user_data: The data to pass to @callback.
 *
 * Asynchronously invokes the @method_name method on @proxy.
 *
 * If @method_name contains any dots, then @name is split into interface and
 * method name parts. This allows using @proxy for invoking methods on
 * other interfaces.
 *
 * If the #GDBusConnection associated with @proxy is closed then
 * the operation will fail with %G_IO_ERROR_CLOSED. If
 * @cancellable is canceled, the operation will fail with
 * %G_IO_ERROR_CANCELLED. If @parameters contains a value not
 * compatible with the D-Bus protocol, the operation fails with
 * %G_IO_ERROR_INVALID_ARGUMENT.
 *
 * If the @parameters #GVariant is floating, it is consumed. This allows
 * convenient 'inline' use of g_variant_new(), e.g.:
 * |[
 *  g_dbus_proxy_call (proxy,
 *                     "TwoStrings",
 *                     g_variant_new ("(ss)",
 *                                    "Thing One",
 *                                    "Thing Two"),
 *                     G_DBUS_CALL_FLAGS_NONE,
 *                     -1,
 *                     NULL,
 *                     (GAsyncReadyCallback) two_strings_done,
 *                     &amp;data);
 * ]|
 *
 * This is an asynchronous method. When the operation is finished,
 * @callback will be invoked in the
 * <link linkend="g-main-context-push-thread-default">thread-default
 * main loop</link> of the thread you are calling this method from.
 * You can then call g_dbus_proxy_call_finish() to get the result of
 * the operation. See g_dbus_proxy_call_sync() for the synchronous
 * version of this method.
 *
 * Since: 2.26
 */
void
g_dbus_proxy_call (GDBusProxy          *proxy,
                   const gchar         *method_name,
                   GVariant            *parameters,
                   GDBusCallFlags       flags,
                   gint                 timeout_msec,
                   GCancellable        *cancellable,
                   GAsyncReadyCallback  callback,
                   gpointer             user_data)
{
  GSimpleAsyncResult *simple;
  gboolean was_split;
  gchar *split_interface_name;
  const gchar *split_method_name;
  const gchar *target_method_name;
  const gchar *target_interface_name;
  const gchar *destination;
  GVariantType *reply_type;

  g_return_if_fail (G_IS_DBUS_PROXY (proxy));
  g_return_if_fail (g_dbus_is_member_name (method_name) || g_dbus_is_interface_name (method_name));
  g_return_if_fail (parameters == NULL || g_variant_is_of_type (parameters, G_VARIANT_TYPE_TUPLE));
  g_return_if_fail (timeout_msec == -1 || timeout_msec >= 0);

  reply_type = NULL;
  split_interface_name = NULL;

  simple = g_simple_async_result_new (G_OBJECT (proxy),
                                      callback,
                                      user_data,
                                      g_dbus_proxy_call);

  was_split = maybe_split_method_name (method_name, &split_interface_name, &split_method_name);
  target_method_name = was_split ? split_method_name : method_name;
  target_interface_name = was_split ? split_interface_name : proxy->priv->interface_name;

  /* Warn if method is unexpected (cf. :g-interface-info) */
  if (!was_split)
    {
      const GDBusMethodInfo *expected_method_info;
      expected_method_info = lookup_method_info_or_warn (proxy, target_method_name);
      if (expected_method_info != NULL)
        reply_type = _g_dbus_compute_complete_signature (expected_method_info->out_args);
    }

  destination = NULL;
  if (proxy->priv->name != NULL)
    {
      destination = get_destination_for_call (proxy);
      if (destination == NULL)
        {
          g_simple_async_result_set_error (simple,
                                           G_IO_ERROR,
                                           G_IO_ERROR_FAILED,
                                           _("Cannot invoke method; proxy is for a well-known name without an owner and proxy was constructed with the G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START flag"));
          goto out;
        }
    }

  g_dbus_connection_call (proxy->priv->connection,
                          destination,
                          proxy->priv->object_path,
                          target_interface_name,
                          target_method_name,
                          parameters,
                          reply_type,
                          flags,
                          timeout_msec == -1 ? proxy->priv->timeout_msec : timeout_msec,
                          cancellable,
                          (GAsyncReadyCallback) reply_cb,
                          simple);

 out:
  if (reply_type != NULL)
    g_variant_type_free (reply_type);

  g_free (split_interface_name);
}

/**
 * g_dbus_proxy_call_finish:
 * @proxy: A #GDBusProxy.
 * @res: A #GAsyncResult obtained from the #GAsyncReadyCallback passed to g_dbus_proxy_call().
 * @error: Return location for error or %NULL.
 *
 * Finishes an operation started with g_dbus_proxy_call().
 *
 * Returns: %NULL if @error is set. Otherwise a #GVariant tuple with
 * return values. Free with g_variant_unref().
 *
 * Since: 2.26
 */
GVariant *
g_dbus_proxy_call_finish (GDBusProxy    *proxy,
                          GAsyncResult  *res,
                          GError       **error)
{
  GSimpleAsyncResult *simple = G_SIMPLE_ASYNC_RESULT (res);
  GVariant *value;

  g_return_val_if_fail (G_IS_DBUS_PROXY (proxy), NULL);
  g_return_val_if_fail (G_IS_ASYNC_RESULT (res), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  g_warn_if_fail (g_simple_async_result_get_source_tag (simple) == g_dbus_proxy_call);

  value = NULL;

  if (g_simple_async_result_propagate_error (simple, error))
    goto out;

  value = g_variant_ref (g_simple_async_result_get_op_res_gpointer (simple));

 out:
  return value;
}

/**
 * g_dbus_proxy_call_sync:
 * @proxy: A #GDBusProxy.
 * @method_name: Name of method to invoke.
 * @parameters: A #GVariant tuple with parameters for the signal or %NULL if not passing parameters.
 * @flags: Flags from the #GDBusCallFlags enumeration.
 * @timeout_msec: The timeout in milliseconds or -1 to use the proxy default timeout.
 * @cancellable: A #GCancellable or %NULL.
 * @error: Return location for error or %NULL.
 *
 * Synchronously invokes the @method_name method on @proxy.
 *
 * If @method_name contains any dots, then @name is split into interface and
 * method name parts. This allows using @proxy for invoking methods on
 * other interfaces.
 *
 * If the #GDBusConnection associated with @proxy is disconnected then
 * the operation will fail with %G_IO_ERROR_CLOSED. If
 * @cancellable is canceled, the operation will fail with
 * %G_IO_ERROR_CANCELLED. If @parameters contains a value not
 * compatible with the D-Bus protocol, the operation fails with
 * %G_IO_ERROR_INVALID_ARGUMENT.
 *
 * If the @parameters #GVariant is floating, it is consumed. This allows
 * convenient 'inline' use of g_variant_new(), e.g.:
 * |[
 *  g_dbus_proxy_call_sync (proxy,
 *                          "TwoStrings",
 *                          g_variant_new ("(ss)",
 *                                         "Thing One",
 *                                         "Thing Two"),
 *                          G_DBUS_CALL_FLAGS_NONE,
 *                          -1,
 *                          NULL,
 *                          &amp;error);
 * ]|
 *
 * The calling thread is blocked until a reply is received. See
 * g_dbus_proxy_call() for the asynchronous version of this
 * method.
 *
 * Returns: %NULL if @error is set. Otherwise a #GVariant tuple with
 * return values. Free with g_variant_unref().
 *
 * Since: 2.26
 */
GVariant *
g_dbus_proxy_call_sync (GDBusProxy      *proxy,
                        const gchar     *method_name,
                        GVariant        *parameters,
                        GDBusCallFlags   flags,
                        gint             timeout_msec,
                        GCancellable    *cancellable,
                        GError         **error)
{
  GVariant *ret;
  gboolean was_split;
  gchar *split_interface_name;
  const gchar *split_method_name;
  const gchar *target_method_name;
  const gchar *target_interface_name;
  const gchar *destination;
  GVariantType *reply_type;

  g_return_val_if_fail (G_IS_DBUS_PROXY (proxy), NULL);
  g_return_val_if_fail (g_dbus_is_member_name (method_name) || g_dbus_is_interface_name (method_name), NULL);
  g_return_val_if_fail (parameters == NULL || g_variant_is_of_type (parameters, G_VARIANT_TYPE_TUPLE), NULL);
  g_return_val_if_fail (timeout_msec == -1 || timeout_msec >= 0, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  reply_type = NULL;

  was_split = maybe_split_method_name (method_name, &split_interface_name, &split_method_name);
  target_method_name = was_split ? split_method_name : method_name;
  target_interface_name = was_split ? split_interface_name : proxy->priv->interface_name;

  /* Warn if method is unexpected (cf. :g-interface-info) */
  if (!was_split)
    {
      const GDBusMethodInfo *expected_method_info;
      expected_method_info = lookup_method_info_or_warn (proxy, target_method_name);
      if (expected_method_info != NULL)
        reply_type = _g_dbus_compute_complete_signature (expected_method_info->out_args);
    }

  destination = NULL;
  if (proxy->priv->name != NULL)
    {
      destination = get_destination_for_call (proxy);
      if (destination == NULL)
        {
          g_set_error_literal (error,
                               G_IO_ERROR,
                               G_IO_ERROR_FAILED,
                               _("Cannot invoke method; proxy is for a well-known name without an owner and proxy was constructed with the G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START flag"));
          ret = NULL;
          goto out;
        }
    }

  ret = g_dbus_connection_call_sync (proxy->priv->connection,
                                     destination,
                                     proxy->priv->object_path,
                                     target_interface_name,
                                     target_method_name,
                                     parameters,
                                     reply_type,
                                     flags,
                                     timeout_msec == -1 ? proxy->priv->timeout_msec : timeout_msec,
                                     cancellable,
                                     error);

 out:
  if (reply_type != NULL)
    g_variant_type_free (reply_type);

  g_free (split_interface_name);

  return ret;
}

/* ---------------------------------------------------------------------------------------------------- */
