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

#include "gdbusutils.h"
#include "gdbusconnection.h"
#include "gdbusmessage.h"
#include "gdbusmethodinvocation.h"
#include "gdbusintrospection.h"
#include "gdbuserror.h"
#include "gdbusprivate.h"

#include "glibintl.h"

/**
 * SECTION:gdbusmethodinvocation
 * @short_description: Object for handling remote calls
 * @include: gio/gio.h
 *
 * Instances of the #GDBusMethodInvocation class are used when
 * handling D-Bus method calls. It provides a way to asynchronously
 * return results and errors.
 *
 * The normal way to obtain a #GDBusMethodInvocation object is to receive
 * it as an argument to the handle_method_call() function in a
 * #GDBusInterfaceVTable that was passed to g_dbus_connection_register_object().
 */

typedef struct _GDBusMethodInvocationClass GDBusMethodInvocationClass;

/**
 * GDBusMethodInvocationClass:
 *
 * Class structure for #GDBusMethodInvocation.
 *
 * Since: 2.26
 */
struct _GDBusMethodInvocationClass
{
  /*< private >*/
  GObjectClass parent_class;
};

/**
 * GDBusMethodInvocation:
 *
 * The #GDBusMethodInvocation structure contains only private data and
 * should only be accessed using the provided API.
 *
 * Since: 2.26
 */
struct _GDBusMethodInvocation
{
  /*< private >*/
  GObject parent_instance;

  /* construct-only properties */
  gchar           *sender;
  gchar           *object_path;
  gchar           *interface_name;
  gchar           *method_name;
  const GDBusMethodInfo *method_info;
  GDBusConnection *connection;
  GDBusMessage    *message;
  GVariant        *parameters;
  gpointer         user_data;
};

G_DEFINE_TYPE (GDBusMethodInvocation, g_dbus_method_invocation, G_TYPE_OBJECT);

static void
g_dbus_method_invocation_finalize (GObject *object)
{
  GDBusMethodInvocation *invocation = G_DBUS_METHOD_INVOCATION (object);

  g_free (invocation->sender);
  g_free (invocation->object_path);
  g_free (invocation->interface_name);
  g_free (invocation->method_name);
  g_object_unref (invocation->connection);
  g_object_unref (invocation->message);
  g_variant_unref (invocation->parameters);

  G_OBJECT_CLASS (g_dbus_method_invocation_parent_class)->finalize (object);
}

static void
g_dbus_method_invocation_class_init (GDBusMethodInvocationClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize = g_dbus_method_invocation_finalize;
}

static void
g_dbus_method_invocation_init (GDBusMethodInvocation *invocation)
{
}

/**
 * g_dbus_method_invocation_get_sender:
 * @invocation: A #GDBusMethodInvocation.
 *
 * Gets the bus name that invoked the method.
 *
 * Returns: A string. Do not free, it is owned by @invocation.
 *
 * Since: 2.26
 */
const gchar *
g_dbus_method_invocation_get_sender (GDBusMethodInvocation *invocation)
{
  g_return_val_if_fail (G_IS_DBUS_METHOD_INVOCATION (invocation), NULL);
  return invocation->sender;
}

/**
 * g_dbus_method_invocation_get_object_path:
 * @invocation: A #GDBusMethodInvocation.
 *
 * Gets the object path the method was invoked on.
 *
 * Returns: A string. Do not free, it is owned by @invocation.
 *
 * Since: 2.26
 */
const gchar *
g_dbus_method_invocation_get_object_path (GDBusMethodInvocation *invocation)
{
  g_return_val_if_fail (G_IS_DBUS_METHOD_INVOCATION (invocation), NULL);
  return invocation->object_path;
}

/**
 * g_dbus_method_invocation_get_interface_name:
 * @invocation: A #GDBusMethodInvocation.
 *
 * Gets the name of the D-Bus interface the method was invoked on.
 *
 * Returns: A string. Do not free, it is owned by @invocation.
 *
 * Since: 2.26
 */
const gchar *
g_dbus_method_invocation_get_interface_name (GDBusMethodInvocation *invocation)
{
  g_return_val_if_fail (G_IS_DBUS_METHOD_INVOCATION (invocation), NULL);
  return invocation->interface_name;
}

/**
 * g_dbus_method_invocation_get_method_info:
 * @invocation: A #GDBusMethodInvocation.
 *
 * Gets information about the method call, if any.
 *
 * Returns: A #GDBusMethodInfo or %NULL. Do not free, it is owned by @invocation.
 *
 * Since: 2.26
 */
const GDBusMethodInfo *
g_dbus_method_invocation_get_method_info (GDBusMethodInvocation *invocation)
{
  g_return_val_if_fail (G_IS_DBUS_METHOD_INVOCATION (invocation), NULL);
  return invocation->method_info;
}

/**
 * g_dbus_method_invocation_get_method_name:
 * @invocation: A #GDBusMethodInvocation.
 *
 * Gets the name of the method that was invoked.
 *
 * Returns: A string. Do not free, it is owned by @invocation.
 *
 * Since: 2.26
 */
const gchar *
g_dbus_method_invocation_get_method_name (GDBusMethodInvocation *invocation)
{
  g_return_val_if_fail (G_IS_DBUS_METHOD_INVOCATION (invocation), NULL);
  return invocation->method_name;
}

/**
 * g_dbus_method_invocation_get_connection:
 * @invocation: A #GDBusMethodInvocation.
 *
 * Gets the #GDBusConnection the method was invoked on.
 *
 * Returns: A #GDBusConnection. Do not free, it is owned by @invocation.
 *
 * Since: 2.26
 */
GDBusConnection *
g_dbus_method_invocation_get_connection (GDBusMethodInvocation *invocation)
{
  g_return_val_if_fail (G_IS_DBUS_METHOD_INVOCATION (invocation), NULL);
  return invocation->connection;
}

/**
 * g_dbus_method_invocation_get_message:
 * @invocation: A #GDBusMethodInvocation.
 *
 * Gets the #GDBusMessage for the method invocation. This is useful if
 * you need to use low-level protocol features, such as UNIX file
 * descriptor passing, that cannot be properly expressed in the
 * #GVariant API.
 *
 * See <xref linkend="gdbus-server"/> and <xref
 * linkend="gdbus-unix-fd-client"/> for an example of how to use this
 * low-level API to send and receive UNIX file descriptors.
 *
 * Returns: A #GDBusMessage. Do not free, it is owned by @invocation.
 *
 * Since: 2.26
 */
GDBusMessage *
g_dbus_method_invocation_get_message (GDBusMethodInvocation *invocation)
{
  g_return_val_if_fail (G_IS_DBUS_METHOD_INVOCATION (invocation), NULL);
  return invocation->message;
}

/**
 * g_dbus_method_invocation_get_parameters:
 * @invocation: A #GDBusMethodInvocation.
 *
 * Gets the parameters of the method invocation.
 *
 * Returns: A #GVariant. Do not free, it is owned by @invocation.
 *
 * Since: 2.26
 */
GVariant *
g_dbus_method_invocation_get_parameters (GDBusMethodInvocation *invocation)
{
  g_return_val_if_fail (G_IS_DBUS_METHOD_INVOCATION (invocation), NULL);
  return invocation->parameters;
}

/**
 * g_dbus_method_invocation_get_user_data:
 * @invocation: A #GDBusMethodInvocation.
 *
 * Gets the @user_data #gpointer passed to g_dbus_connection_register_object().
 *
 * Returns: A #gpointer.
 *
 * Since: 2.26
 */
gpointer
g_dbus_method_invocation_get_user_data (GDBusMethodInvocation *invocation)
{
  g_return_val_if_fail (G_IS_DBUS_METHOD_INVOCATION (invocation), NULL);
  return invocation->user_data;
}

/* < internal >
 * _g_dbus_method_invocation_new:
 * @sender: The bus name that invoked the method or %NULL if @connection is not a bus connection.
 * @object_path: The object path the method was invoked on.
 * @interface_name: The name of the D-Bus interface the method was invoked on.
 * @method_name: The name of the method that was invoked.
 * @method_info: Information about the method call or %NULL.
 * @connection: The #GDBusConnection the method was invoked on.
 * @message: The D-Bus message as a #GDBusMessage.
 * @parameters: The parameters as a #GVariant tuple.
 * @user_data: The @user_data #gpointer passed to g_dbus_connection_register_object().
 *
 * Creates a new #GDBusMethodInvocation object.
 *
 * Returns: A #GDBusMethodInvocation. Free with g_object_unref().
 *
 * Since: 2.26
 */
GDBusMethodInvocation *
_g_dbus_method_invocation_new (const gchar           *sender,
                               const gchar           *object_path,
                               const gchar           *interface_name,
                               const gchar           *method_name,
                               const GDBusMethodInfo *method_info,
                               GDBusConnection       *connection,
                               GDBusMessage          *message,
                               GVariant              *parameters,
                               gpointer               user_data)
{
  GDBusMethodInvocation *invocation;

  g_return_val_if_fail (sender == NULL || g_dbus_is_name (sender), NULL);
  g_return_val_if_fail (g_variant_is_object_path (object_path), NULL);
  g_return_val_if_fail (interface_name == NULL || g_dbus_is_interface_name (interface_name), NULL);
  g_return_val_if_fail (g_dbus_is_member_name (method_name), NULL);
  g_return_val_if_fail (G_IS_DBUS_CONNECTION (connection), NULL);
  g_return_val_if_fail (G_IS_DBUS_MESSAGE (message), NULL);
  g_return_val_if_fail (g_variant_is_of_type (parameters, G_VARIANT_TYPE_TUPLE), NULL);

  invocation = G_DBUS_METHOD_INVOCATION (g_object_new (G_TYPE_DBUS_METHOD_INVOCATION, NULL));
  invocation->sender = g_strdup (sender);
  invocation->object_path = g_strdup (object_path);
  invocation->interface_name = g_strdup (interface_name);
  invocation->method_name = g_strdup (method_name);
  invocation->method_info = g_dbus_method_info_ref ((GDBusMethodInfo *)method_info);
  invocation->connection = g_object_ref (connection);
  invocation->message = g_object_ref (message);
  invocation->parameters = g_variant_ref (parameters);
  invocation->user_data = user_data;

  return invocation;
}

/* ---------------------------------------------------------------------------------------------------- */

/**
 * g_dbus_method_invocation_return_value:
 * @invocation: A #GDBusMethodInvocation.
 * @parameters: A #GVariant tuple with out parameters for the method or %NULL if not passing any parameters.
 *
 * Finishes handling a D-Bus method call by returning @parameters.
 * If the @parameters GVariant is floating, it is consumed.
 *
 * It is an error if @parameters is not of the right format.
 *
 * This method will free @invocation, you cannot use it afterwards.
 *
 * Since: 2.26
 */
void
g_dbus_method_invocation_return_value (GDBusMethodInvocation *invocation,
                                       GVariant              *parameters)
{
  GDBusMessage *reply;
  GError *error;

  g_return_if_fail (G_IS_DBUS_METHOD_INVOCATION (invocation));
  g_return_if_fail ((parameters == NULL) || g_variant_is_of_type (parameters, G_VARIANT_TYPE_TUPLE));

  if (parameters == NULL)
    parameters = g_variant_new_tuple (NULL, 0);

  /* if we have introspection data, check that the signature of @parameters is correct */
  if (invocation->method_info != NULL)
    {
      GVariantType *type;

      type = _g_dbus_compute_complete_signature (invocation->method_info->out_args);

      if (!g_variant_is_of_type (parameters, type))
        {
          gchar *type_string = g_variant_type_dup_string (type);

          g_warning (_("Type of return value is incorrect, got `%s', expected `%s'"),
                     g_variant_get_type_string (parameters), type_string);
          g_variant_type_free (type);
          g_free (type_string);
          goto out;
        }
      g_variant_type_free (type);
    }

  if (G_UNLIKELY (_g_dbus_debug_return ()))
    {
      _g_dbus_debug_print_lock ();
      g_print ("========================================================================\n"
               "GDBus-debug:Return:\n"
               " >>>> METHOD RETURN\n"
               "      in response to %s.%s()\n"
               "      on object %s\n"
               "      to name %s\n"
               "      reply-serial %d\n",
               invocation->interface_name, invocation->method_name,
               invocation->object_path,
               invocation->sender,
               g_dbus_message_get_serial (invocation->message));
      _g_dbus_debug_print_unlock ();
    }

  reply = g_dbus_message_new_method_reply (invocation->message);
  g_dbus_message_set_body (reply, parameters);
  error = NULL;
  if (!g_dbus_connection_send_message (g_dbus_method_invocation_get_connection (invocation), reply, G_DBUS_SEND_MESSAGE_FLAGS_NONE, NULL, &error))
    {
      g_warning (_("Error sending message: %s"), error->message);
      g_error_free (error);
    }
  g_object_unref (reply);

 out:
  g_object_unref (invocation);
}

/* ---------------------------------------------------------------------------------------------------- */

/**
 * g_dbus_method_invocation_return_error:
 * @invocation: A #GDBusMethodInvocation.
 * @domain: A #GQuark for the #GError error domain.
 * @code: The error code.
 * @format: printf()-style format.
 * @...: Parameters for @format.
 *
 * Finishes handling a D-Bus method call by returning an error.
 *
 * See g_dbus_error_encode_gerror() for details about what error name
 * will be returned on the wire. In a nutshell, if the given error is
 * registered using g_dbus_error_register_error() the name given
 * during registration is used. Otherwise, a name of the form
 * <literal>org.gtk.GDBus.UnmappedGError.Quark...</literal> is
 * used. This provides transparent mapping of #GError between
 * applications using GDBus.
 *
 * If you are writing an application intended to be portable,
 * <emphasis>always</emphasis> register errors with g_dbus_error_register_error()
 * or use g_dbus_method_invocation_return_dbus_error().
 *
 * This method will free @invocation, you cannot use it afterwards.
 *
 * Since: 2.26
 */
void
g_dbus_method_invocation_return_error (GDBusMethodInvocation *invocation,
                                       GQuark                 domain,
                                       gint                   code,
                                       const gchar           *format,
                                       ...)
{
  va_list var_args;

  g_return_if_fail (G_IS_DBUS_METHOD_INVOCATION (invocation));
  g_return_if_fail (format != NULL);

  va_start (var_args, format);
  g_dbus_method_invocation_return_error_valist (invocation,
                                                domain,
                                                code,
                                                format,
                                                var_args);
  va_end (var_args);
}

/**
 * g_dbus_method_invocation_return_error_valist:
 * @invocation: A #GDBusMethodInvocation.
 * @domain: A #GQuark for the #GError error domain.
 * @code: The error code.
 * @format: printf()-style format.
 * @var_args: #va_list of parameters for @format.
 *
 * Like g_dbus_method_invocation_return_error() but intended for
 * language bindings.
 *
 * This method will free @invocation, you cannot use it afterwards.
 *
 * Since: 2.26
 */
void
g_dbus_method_invocation_return_error_valist (GDBusMethodInvocation *invocation,
                                              GQuark                 domain,
                                              gint                   code,
                                              const gchar           *format,
                                              va_list                var_args)
{
  gchar *literal_message;

  g_return_if_fail (G_IS_DBUS_METHOD_INVOCATION (invocation));
  g_return_if_fail (format != NULL);

  literal_message = g_strdup_vprintf (format, var_args);
  g_dbus_method_invocation_return_error_literal (invocation,
                                                 domain,
                                                 code,
                                                 literal_message);
  g_free (literal_message);
}

/**
 * g_dbus_method_invocation_return_error_literal:
 * @invocation: A #GDBusMethodInvocation.
 * @domain: A #GQuark for the #GError error domain.
 * @code: The error code.
 * @message: The error message.
 *
 * Like g_dbus_method_invocation_return_error() but without printf()-style formatting.
 *
 * This method will free @invocation, you cannot use it afterwards.
 *
 * Since: 2.26
 */
void
g_dbus_method_invocation_return_error_literal (GDBusMethodInvocation *invocation,
                                               GQuark                 domain,
                                               gint                   code,
                                               const gchar           *message)
{
  GError *error;

  g_return_if_fail (G_IS_DBUS_METHOD_INVOCATION (invocation));
  g_return_if_fail (message != NULL);

  error = g_error_new_literal (domain, code, message);
  g_dbus_method_invocation_return_gerror (invocation, error);
  g_error_free (error);
}

/**
 * g_dbus_method_invocation_return_gerror:
 * @invocation: A #GDBusMethodInvocation.
 * @error: A #GError.
 *
 * Like g_dbus_method_invocation_return_error() but takes a #GError
 * instead of the error domain, error code and message.
 *
 * This method will free @invocation, you cannot use it afterwards.
 *
 * Since: 2.26
 */
void
g_dbus_method_invocation_return_gerror (GDBusMethodInvocation *invocation,
                                        const GError          *error)
{
  gchar *dbus_error_name;

  g_return_if_fail (G_IS_DBUS_METHOD_INVOCATION (invocation));
  g_return_if_fail (error != NULL);

  dbus_error_name = g_dbus_error_encode_gerror (error);

  g_dbus_method_invocation_return_dbus_error (invocation,
                                              dbus_error_name,
                                              error->message);
  g_free (dbus_error_name);
}

/**
 * g_dbus_method_invocation_return_dbus_error:
 * @invocation: A #GDBusMethodInvocation.
 * @error_name: A valid D-Bus error name.
 * @error_message: A valid D-Bus error message.
 *
 * Finishes handling a D-Bus method call by returning an error.
 *
 * This method will free @invocation, you cannot use it afterwards.
 *
 * Since: 2.26
 */
void
g_dbus_method_invocation_return_dbus_error (GDBusMethodInvocation *invocation,
                                            const gchar           *error_name,
                                            const gchar           *error_message)
{
  GDBusMessage *reply;

  g_return_if_fail (G_IS_DBUS_METHOD_INVOCATION (invocation));
  g_return_if_fail (error_name != NULL && g_dbus_is_name (error_name));
  g_return_if_fail (error_message != NULL);

  if (G_UNLIKELY (_g_dbus_debug_return ()))
    {
      _g_dbus_debug_print_lock ();
      g_print ("========================================================================\n"
               "GDBus-debug:Return:\n"
               " >>>> METHOD ERROR %s\n"
               "      message `%s'\n"
               "      in response to %s.%s()\n"
               "      on object %s\n"
               "      to name %s\n"
               "      reply-serial %d\n",
               error_name,
               error_message,
               invocation->interface_name, invocation->method_name,
               invocation->object_path,
               invocation->sender,
               g_dbus_message_get_serial (invocation->message));
      _g_dbus_debug_print_unlock ();
    }

  reply = g_dbus_message_new_method_error_literal (invocation->message,
                                                   error_name,
                                                   error_message);
  g_dbus_connection_send_message (g_dbus_method_invocation_get_connection (invocation), reply, G_DBUS_SEND_MESSAGE_FLAGS_NONE, NULL, NULL);
  g_object_unref (reply);

  g_object_unref (invocation);
}
