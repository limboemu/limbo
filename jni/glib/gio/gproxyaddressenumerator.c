/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2010 Collabora, Ltd.
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
 * Author: Nicolas Dufresne <nicolas.dufresne@collabora.co.uk>
 */

#include "config.h"
#include "gproxyaddressenumerator.h"

#include <string.h>

#include "gasyncresult.h"
#include "ginetaddress.h"
#include "glibintl.h"
#include "gnetworkaddress.h"
#include "gnetworkingprivate.h"
#include "gproxy.h"
#include "gproxyaddress.h"
#include "gproxyresolver.h"
#include "gsimpleasyncresult.h"
#include "gresolver.h"
#include "gsocketaddress.h"
#include "gsocketaddressenumerator.h"
#include "gsocketconnectable.h"

G_DEFINE_TYPE (GProxyAddressEnumerator, g_proxy_address_enumerator, G_TYPE_SOCKET_ADDRESS_ENUMERATOR);

#define GET_PRIVATE(o) (G_PROXY_ADDRESS_ENUMERATOR (o)->priv)

enum
{
  PROP_0,
  PROP_URI,
  PROP_CONNECTABLE
};

struct _GProxyAddressEnumeratorPrivate
{
  /* Destination address */
  GSocketConnectable *connectable;
  gchar		     *dest_uri;
  gchar		     *dest_hostname;
  guint16	      dest_port;
  GList              *dest_ips;

  /* Proxy enumeration */
  gchar			  **proxies;
  gchar			  **next_proxy;
  GSocketAddressEnumerator *addr_enum;
  GSocketAddress           *proxy_address;
  gchar			   *proxy_type;
  gchar			   *proxy_username;
  gchar			   *proxy_password;
  gboolean                  supports_hostname;
  GList			   *next_dest_ip;

  /* Async attributes */
  GSimpleAsyncResult *simple;
  GCancellable       *cancellable;
};

static void
save_userinfo (GProxyAddressEnumeratorPrivate *priv,
	       const gchar *proxy)
{
  gchar *userinfo;

  if (priv->proxy_username)
    {
      g_free (priv->proxy_username);
      priv->proxy_username = NULL;
    }

  if (priv->proxy_password)
    {
      g_free (priv->proxy_password);
      priv->proxy_password = NULL;
    }
  
  if (_g_uri_parse_authority (proxy, NULL, NULL, &userinfo))
    {
      if (userinfo)
	{
	  gchar **split = g_strsplit (userinfo, ":", 2);

	  if (split[0] != NULL)
	    {
	      priv->proxy_username = g_uri_unescape_string (split[0], NULL);
	      if (split[1] != NULL)
		priv->proxy_password = g_uri_unescape_string (split[1], NULL);
	    }

	  g_strfreev (split);
	  g_free (userinfo);
	}
    }
}

static void
next_enumerator (GProxyAddressEnumeratorPrivate *priv)
{
  if (priv->proxy_address)
    return;

  while (priv->addr_enum == NULL && *priv->next_proxy)
    {
      GSocketConnectable *connectable = NULL;
      const gchar *proxy_uri;
      GProxy *proxy;

      proxy_uri = *priv->next_proxy++;
      g_free (priv->proxy_type);
      priv->proxy_type = g_uri_parse_scheme (proxy_uri);

      if (priv->proxy_type == NULL)
	continue;

      /* Assumes hostnames are supported for unkown protocols */
      priv->supports_hostname = TRUE;
      proxy = g_proxy_get_default_for_protocol (priv->proxy_type);
      if (proxy)
        {
	  priv->supports_hostname = g_proxy_supports_hostname (proxy);
	  g_object_unref (proxy);
        }

      if (strcmp ("direct", priv->proxy_type) == 0)
	{
	  if (priv->connectable)
	    connectable = g_object_ref (priv->connectable);
	  else
	    connectable = g_network_address_new (priv->dest_hostname,
						 priv->dest_port);
	}
      else
	{
	  GError *error = NULL;

	  connectable = g_network_address_parse_uri (proxy_uri, 0, &error);

	  if (error)
	    {
	      g_warning ("Invalid proxy URI '%s': %s",
			 proxy_uri, error->message);
	      g_error_free (error);
	    }

	  save_userinfo (priv, proxy_uri);
	}

      if (connectable)
	{
	  priv->addr_enum = g_socket_connectable_enumerate (connectable);
	  g_object_unref (connectable);
	}
    }
}

static GSocketAddress *
g_proxy_address_enumerator_next (GSocketAddressEnumerator  *enumerator,
				 GCancellable              *cancellable,
				 GError                   **error)
{
  GProxyAddressEnumeratorPrivate *priv = GET_PRIVATE (enumerator);
  GSocketAddress *result = NULL;
  GError *first_error = NULL;

  if (priv->proxies == NULL)
    {
      GProxyResolver *resolver = g_proxy_resolver_get_default ();
      priv->proxies = g_proxy_resolver_lookup (resolver,
					       priv->dest_uri,
					       cancellable,
					       error);
      priv->next_proxy = priv->proxies;

      if (priv->proxies == NULL)
	return NULL;
    }

  while (result == NULL && (*priv->next_proxy || priv->addr_enum))
    {
      gchar *dest_hostname;
      GInetSocketAddress *inetsaddr;
      GInetAddress *inetaddr;
      guint16 port;

      next_enumerator (priv);

      if (!priv->addr_enum)
	continue;

      if (priv->proxy_address == NULL)
	{
	  priv->proxy_address = g_socket_address_enumerator_next (
				    priv->addr_enum,
				    cancellable,
				    first_error ? NULL : &first_error);
	}

      if (priv->proxy_address == NULL)
	{
	  g_object_unref (priv->addr_enum);
	  priv->addr_enum = NULL;

	  if (priv->dest_ips)
	    {
	      g_resolver_free_addresses (priv->dest_ips);
	      priv->dest_ips = NULL;
	    }

	  continue;
	}

      if (strcmp ("direct", priv->proxy_type) == 0)
	{
	  result = priv->proxy_address;
	  priv->proxy_address = NULL;
	  continue;
	}

      if (!priv->supports_hostname)
	{
	  GInetAddress *dest_ip;

	  if (!priv->dest_ips)
	    {
	      GResolver *resolver;

	      resolver = g_resolver_get_default();
	      priv->dest_ips = g_resolver_lookup_by_name (resolver,
							  priv->dest_hostname,
							  cancellable,
							  first_error ? NULL : &first_error);
	      g_object_unref (resolver);

	      if (!priv->dest_ips)
		{
		  g_object_unref (priv->proxy_address);
		  priv->proxy_address = NULL;
		  continue;
		}
	    }

	  if (!priv->next_dest_ip)
	    priv->next_dest_ip = priv->dest_ips;
	
	  dest_ip = G_INET_ADDRESS (priv->next_dest_ip->data);
	  dest_hostname = g_inet_address_to_string (dest_ip);

	  priv->next_dest_ip = g_list_next (priv->next_dest_ip);
	}
      else
	{
	  dest_hostname = g_strdup (priv->dest_hostname);
	}
	
		 		  
      g_return_val_if_fail (G_IS_INET_SOCKET_ADDRESS (priv->proxy_address),
			    NULL);

      inetsaddr = G_INET_SOCKET_ADDRESS (priv->proxy_address);
      inetaddr = g_inet_socket_address_get_address (inetsaddr);
      port = g_inet_socket_address_get_port (inetsaddr);

      result = g_proxy_address_new (inetaddr, port,
				    priv->proxy_type,
				    dest_hostname, priv->dest_port,
				    priv->proxy_username,
				    priv->proxy_password);

      g_free (dest_hostname);

      if (priv->supports_hostname || priv->next_dest_ip == NULL)
	{
	  g_object_unref (priv->proxy_address);
	  priv->proxy_address = NULL;
	}
    }

  if (result == NULL && first_error)
    g_propagate_error (error, first_error);
  else if (first_error)
    g_error_free (first_error);

  return result;
}



static void
complete_async (GProxyAddressEnumeratorPrivate *priv)
{
  GSimpleAsyncResult *simple = priv->simple;

  if (priv->cancellable)
    {
      g_object_unref (priv->cancellable);
      priv->cancellable = NULL;
    }

  priv->simple = NULL;
  g_simple_async_result_complete (simple);
  g_object_unref (simple);
}

static void
save_result (GProxyAddressEnumeratorPrivate *priv)
{
  GSocketAddress *result;

  if (strcmp ("direct", priv->proxy_type) == 0)
    {
      result = priv->proxy_address;
      priv->proxy_address = NULL;
    }
  else
    {
      gchar *dest_hostname;
      GInetSocketAddress *inetsaddr;
      GInetAddress *inetaddr;
      guint16 port;

      if (!priv->supports_hostname)
	{
	  GInetAddress *dest_ip;

	  if (!priv->next_dest_ip)
	    priv->next_dest_ip = priv->dest_ips;

	  dest_ip = G_INET_ADDRESS (priv->next_dest_ip->data);
	  dest_hostname = g_inet_address_to_string (dest_ip);

	  priv->next_dest_ip = g_list_next (priv->next_dest_ip);
	}
      else
	{
	  dest_hostname = g_strdup (priv->dest_hostname);
	}

      g_return_if_fail (G_IS_INET_SOCKET_ADDRESS (priv->proxy_address));

      inetsaddr = G_INET_SOCKET_ADDRESS (priv->proxy_address);
      inetaddr = g_inet_socket_address_get_address (inetsaddr);
      port = g_inet_socket_address_get_port (inetsaddr);

      result = g_proxy_address_new (inetaddr, port,
				    priv->proxy_type,
				    dest_hostname, priv->dest_port,
				    priv->proxy_username,
				    priv->proxy_password);

      g_free (dest_hostname);

      if (priv->supports_hostname || priv->next_dest_ip == NULL)
	{
	  g_object_unref (priv->proxy_address);
	  priv->proxy_address = NULL;
	}
    }

  g_simple_async_result_set_op_res_gpointer (priv->simple,
					     result,
					     g_object_unref);
}

static void
dest_hostname_lookup_cb (GObject           *object,
			 GAsyncResult      *result,
			 gpointer           user_data)
{
  GError *error = NULL;
  GProxyAddressEnumeratorPrivate *priv = user_data;
  GSimpleAsyncResult *simple = priv->simple;

  priv->dest_ips = g_resolver_lookup_by_name_finish (G_RESOLVER (object),
						     result,
						     &error);
  if (priv->dest_ips)
    {
      save_result (priv);
    }
  else
    {
      g_simple_async_result_set_from_error (simple, error);
      g_error_free (error);
    }

  complete_async (priv); 
}

static void
address_enumerate_cb (GObject	   *object,
		      GAsyncResult *result,
		      gpointer	    user_data)
{
  GError *error = NULL;
  GProxyAddressEnumeratorPrivate *priv = user_data;
  GSimpleAsyncResult *simple = priv->simple;

  priv->proxy_address =
    g_socket_address_enumerator_next_finish (priv->addr_enum,
					     result,
					     &error);
  if (priv->proxy_address)
    {
      if (!priv->supports_hostname && !priv->dest_ips)
	{
	  GResolver *resolver;
	  resolver = g_resolver_get_default();
	  g_resolver_lookup_by_name_async (resolver,
					   priv->dest_hostname,
					   priv->cancellable,
					   dest_hostname_lookup_cb,
					   priv);
	  g_object_unref (resolver);
	  return;
	}

      save_result (priv);
    }
  else if (*priv->next_proxy)
    {
      g_object_unref (priv->addr_enum);
      priv->addr_enum = NULL;

      if (priv->dest_ips)
	{
	  g_resolver_free_addresses (priv->dest_ips);
	  priv->dest_ips = NULL;
	}

      next_enumerator (priv);

      if (priv->addr_enum)
	{
	  g_socket_address_enumerator_next_async (priv->addr_enum,
						  priv->cancellable,
						  address_enumerate_cb,
						  priv);
	  return;
	}
    }

  if (error)
    {
      g_simple_async_result_set_from_error (simple, error);
      g_error_free (error);
    }

  complete_async (priv); 
}

static void
proxy_lookup_cb (GObject      *object,
		 GAsyncResult *result,
		 gpointer      user_data)
{
  GError *error = NULL;
  GProxyAddressEnumeratorPrivate *priv = user_data;
  GSimpleAsyncResult *simple = priv->simple;

  priv->proxies = g_proxy_resolver_lookup_finish (G_PROXY_RESOLVER (object),
						  result,
						  &error);
  priv->next_proxy = priv->proxies;

  if (error)
    {
      g_simple_async_result_set_from_error (simple, error);
      g_error_free (error);
    }
  else
    {
      next_enumerator (priv);
      if (priv->addr_enum)
	{
	  g_socket_address_enumerator_next_async (priv->addr_enum,
						  priv->cancellable,
						  address_enumerate_cb,
						  priv);
	  return;
	}
    }

  complete_async (priv); 
}

static void
g_proxy_address_enumerator_next_async (GSocketAddressEnumerator *enumerator,
				       GCancellable             *cancellable,
				       GAsyncReadyCallback       callback,
				       gpointer                  user_data)
{
  GProxyAddressEnumeratorPrivate *priv = GET_PRIVATE (enumerator);

  g_return_if_fail (priv->simple == NULL);
  g_return_if_fail (priv->cancellable == NULL);

  priv->simple = g_simple_async_result_new (G_OBJECT (enumerator),
					    callback, user_data,
					    g_proxy_address_enumerator_next_async);

  priv->cancellable = cancellable ? g_object_ref (cancellable) : NULL;

  if (priv->proxies == NULL)
    {
      GProxyResolver *resolver = g_proxy_resolver_get_default ();
      g_proxy_resolver_lookup_async (resolver,
				     priv->dest_uri,
				     cancellable,
				     proxy_lookup_cb,
				     priv);
      return;
    }

  if (priv->addr_enum)
    {
      if (priv->proxy_address)
	{
	  save_result (priv);
	}
      else
	{
	  g_socket_address_enumerator_next_async (priv->addr_enum,
						  cancellable,
						  address_enumerate_cb,
						  priv);
	  return;
	}
    }

  g_simple_async_result_complete_in_idle (priv->simple);

  g_object_unref (priv->simple);
  priv->simple = NULL;

  if (priv->cancellable)
    {
      g_object_unref (priv->cancellable);
      priv->cancellable = NULL;
    }
}

static GSocketAddress *
g_proxy_address_enumerator_next_finish (GSocketAddressEnumerator  *enumerator,
					GAsyncResult              *result,
					GError                   **error)
{
  GSimpleAsyncResult *simple = G_SIMPLE_ASYNC_RESULT (result);
  GSocketAddress *address;

  if (g_simple_async_result_propagate_error (simple, error))
    return NULL;

  address = g_simple_async_result_get_op_res_gpointer (simple);
  if (address)
    g_object_ref (address);

  return address;
}

static void
g_proxy_address_enumerator_get_property (GObject        *object,
					 guint           property_id,
					 GValue         *value,
					 GParamSpec     *pspec)
{
  GProxyAddressEnumeratorPrivate *priv = GET_PRIVATE (object);
  switch (property_id)
    {
      case PROP_URI:
	g_value_set_string (value, priv->dest_uri);
	break;

      case PROP_CONNECTABLE:
	g_value_set_object (value, priv->connectable);
	break;

      default:
	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
g_proxy_address_enumerator_set_property (GObject        *object,
					 guint           property_id,
					 const GValue   *value,
					 GParamSpec     *pspec)
{
  GProxyAddressEnumeratorPrivate *priv = GET_PRIVATE (object);
  switch (property_id)
    {
      case PROP_URI:
	  {
	    const gchar *uri;

	    g_free (priv->dest_hostname);
	    priv->dest_hostname = NULL;
	    priv->dest_port = 0;

	    g_free (priv->dest_uri);
	    priv->dest_uri = NULL;

	    uri = g_value_get_string (value);

	    if (uri)
	      {
		GSocketConnectable *conn;

		conn = g_network_address_parse_uri (uri, 0, NULL);
		if (conn)
		  {
		    guint port;

		    priv->dest_uri = g_strdup (uri);
		    
		    g_object_get (conn,
				  "hostname", &priv->dest_hostname,
				  "port", &port,
				  NULL);

		    priv->dest_port = port;
		    g_object_unref (conn);
		  }
		else
		  g_warning ("Invalid URI '%s'", uri);
	      }

	    break;
	  }

      case PROP_CONNECTABLE:
	  priv->connectable = g_value_dup_object (value);
	  break;

      default:
	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
g_proxy_address_enumerator_finalize (GObject *object)
{
  GProxyAddressEnumeratorPrivate *priv = GET_PRIVATE (object);

  if (priv->connectable)
    g_object_unref (priv->connectable);

  g_free (priv->dest_uri);
  g_free (priv->dest_hostname);

  if (priv->dest_ips)
    g_resolver_free_addresses (priv->dest_ips);

  g_strfreev (priv->proxies);

  if (priv->addr_enum)
    g_object_unref (priv->addr_enum);

  g_free (priv->proxy_type);
  g_free (priv->proxy_username);
  g_free (priv->proxy_password);

  if (priv->cancellable)
    g_object_unref (priv->cancellable);

  G_OBJECT_CLASS (g_proxy_address_enumerator_parent_class)->finalize (object);
}

static void
g_proxy_address_enumerator_init (GProxyAddressEnumerator *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
					    G_TYPE_PROXY_ADDRESS_ENUMERATOR,
					    GProxyAddressEnumeratorPrivate);
}

static void
g_proxy_address_enumerator_class_init (GProxyAddressEnumeratorClass *proxy_enumerator_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (proxy_enumerator_class);
  GSocketAddressEnumeratorClass *enumerator_class = G_SOCKET_ADDRESS_ENUMERATOR_CLASS (proxy_enumerator_class);

  g_type_class_add_private (enumerator_class,
			    sizeof (GProxyAddressEnumeratorPrivate));

  object_class->set_property = g_proxy_address_enumerator_set_property;
  object_class->get_property = g_proxy_address_enumerator_get_property;
  object_class->finalize = g_proxy_address_enumerator_finalize;

  enumerator_class->next = g_proxy_address_enumerator_next;
  enumerator_class->next_async = g_proxy_address_enumerator_next_async;
  enumerator_class->next_finish = g_proxy_address_enumerator_next_finish;

  g_object_class_install_property (object_class,
				   PROP_URI,
				   g_param_spec_string ("uri",
							P_("URI"),
							P_("The destination URI, use none:// for generic socket"),
							NULL,
							G_PARAM_READWRITE |
							G_PARAM_CONSTRUCT_ONLY |
							G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class,
				   PROP_CONNECTABLE,
				   g_param_spec_object ("connectable",
							P_("Connectable"),
							P_("The connectable being enumerated."),
							G_TYPE_SOCKET_CONNECTABLE,
							G_PARAM_READWRITE |
							G_PARAM_CONSTRUCT_ONLY |
							G_PARAM_STATIC_STRINGS));
}
