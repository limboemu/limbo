/*
 * Copyright © 2009, 2010 Codethink Limited
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the licence, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author: Ryan Lortie <desrt@desrt.ca>
 */

/* Prelude {{{1 */
#define _GNU_SOURCE
#include "config.h"

#include <glib.h>
#include <glibintl.h>
#include <locale.h>

#include "gsettings.h"

#include "gdelayedsettingsbackend.h"
#include "gsettingsbackendinternal.h"
#include "gsettings-mapping.h"
#include "gio-marshal.h"
#include "gsettingsschema.h"

#include <string.h>


#include "strinfo.c"

/**
 * SECTION:gsettings
 * @short_description: a high-level API for application settings
 *
 * The #GSettings class provides a convenient API for storing and retrieving
 * application settings.
 *
 * When creating a GSettings instance, you have to specify a schema
 * that describes the keys in your settings and their types and default
 * values, as well as some other information.
 *
 * Normally, a schema has as fixed path that determines where the settings
 * are stored in the conceptual global tree of settings. However, schemas
 * can also be 'relocatable', i.e. not equipped with a fixed path. This is
 * useful e.g. when the schema describes an 'account', and you want to be
 * able to store a arbitrary number of accounts.
 *
 * Unlike other configuration systems (like GConf), GSettings does not
 * restrict keys to basic types like strings and numbers. GSettings stores
 * values as #GVariant, and allows any #GVariantType for keys. Key names
 * are restricted to lowercase characters, numbers and '-'. Furthermore,
 * the names must begin with a lowercase character, must not end
 * with a '-', and must not contain consecutive dashes. Key names can
 * be up to 32 characters long.
 *
 * Similar to GConf, the default values in GSettings schemas can be
 * localized, but the localized values are stored in gettext catalogs
 * and looked up with the domain that is specified in the
 * <tag class="attribute">gettext-domain</tag> attribute of the
 * <tag class="starttag">schemalist</tag> or <tag class="starttag">schema</tag>
 * elements and the category that is specified in the l10n attribute of the
 * <tag class="starttag">key</tag> element.
 *
 * GSettings uses schemas in a compact binary form that is created
 * by the <link linkend="glib-compile-schemas">glib-compile-schemas</link>
 * utility. The input is a schema description in an XML format that can be
 * described by the following DTD:
 * |[<xi:include xmlns:xi="http://www.w3.org/2001/XInclude" parse="text" href="../../../../gio/gschema.dtd"><xi:fallback>FIXME: MISSING XINCLUDE CONTENT</xi:fallback></xi:include>]|
 *
 * At runtime, schemas are identified by their id (as specified
 * in the <tag class="attribute">id</tag> attribute of the
 * <tag class="starttag">schema</tag> element). The
 * convention for schema ids is to use a dotted name, similar in
 * style to a DBus bus name, e.g. "org.gnome.font-rendering".
 *
 * <example><title>Default values</title>
 * <programlisting><![CDATA[
 * <schemalist>
 *   <schema id="org.gtk.test" path="/tests/" gettext-domain="test">
 *
 *     <key name="greeting" type="s">
 *       <default l10n="messages">"Hello, earthlings"</default>
 *       <summary>A greeting</summary>
 *       <description>
 *         Greeting of the invading martians
 *       </description>
 *     </key>
 *
 *     <key name="box" type="(ii)">
 *       <default>(20,30)</default>
 *     </key>
 *
 *   </schema>
 * </schemalist>
 * ]]></programlisting></example>
 *
 * <example><title>Ranges, choices and enumerated types</title>
 * <programlisting><![CDATA[
 * <schemalist>
 *
 *   <enum id="myenum">
 *     <value nick="first" value="1"/>
 *     <value nick="second" value="2"/>
 *   </enum>
 *
 *   <schema id="org.gtk.test">
 *
 *     <key name="key-with-range" type="i">
 *       <range min="1" max="100"/>
 *       <default>10</default>
 *     </key>
 *
 *     <key name="key-with-choices" type="s">
 *       <choices>
 *         <choice value='Elisabeth'/>
 *         <choice value='Annabeth'/>
 *         <choice value='Joe'/>
 *       </choices>
 *       <aliases>
 *         <alias value='Anna' target='Annabeth'/>
 *         <alias value='Beth' target='Elisabeth'/>
 *       </aliases>
 *       <default>'Joe'</default>
 *     </key>
 *
 *     <key name='enumerated-key' enum='myenum'>
 *       <default>'first'</default>
 *     </key>
 *
 *   </schema>
 * </schemalist>
 * ]]></programlisting></example>
 *
 * <refsect2>
 *   <title>Vendor overrides</title>
 *   <para>
 *     Default values are defined in the schemas that get installed by
 *     an application. Sometimes, it is necessary for a vendor or distributor
 *     to adjust these defaults. Since patching the XML source for the schema
 *     is inconvenient and error-prone,
 *     <link linkend="glib-compile-schemas">glib-compile-schemas</link> reads
 *     so-called 'vendor override' files. These are keyfiles in the same
 *     directory as the XML schema sources which can override default values.
 *     The schema id serves as the group name in the key file, and the values
 *     are expected in serialized GVariant form, as in the following example:
 *     <informalexample><programlisting>
 *     [org.gtk.Example]
 *     key1='string'
 *     key2=1.5
 *     </programlisting></informalexample>
 *   </para>
 * </refsect2>
 *
 * <refsect2>
 *   <title>Binding</title>
 *   <para>
 *     A very convenient feature of GSettings lets you bind #GObject properties
 *     directly to settings, using g_settings_bind(). Once a GObject property
 *     has been bound to a setting, changes on either side are automatically
 *     propagated to the other side. GSettings handles details like
 *     mapping between GObject and GVariant types, and preventing infinite
 *     cycles.
 *   </para>
 *   <para>
 *     This makes it very easy to hook up a preferences dialog to the
 *     underlying settings. To make this even more convenient, GSettings
 *     looks for a boolean property with the name "sensitivity" and
 *     automatically binds it to the writability of the bound setting.
 *     If this 'magic' gets in the way, it can be suppressed with the
 *     #G_SETTINGS_BIND_NO_SENSITIVITY flag.
 *   </para>
 * </refsect2>
 **/

struct _GSettingsPrivate
{
  /* where the signals go... */
  GMainContext *main_context;

  GSettingsBackend *backend;
  GSettingsSchema *schema;
  gchar *schema_name;
  gchar *path;

  GDelayedSettingsBackend *delayed;
};

enum
{
  PROP_0,
  PROP_SCHEMA,
  PROP_BACKEND,
  PROP_PATH,
  PROP_HAS_UNAPPLIED,
};

enum
{
  SIGNAL_WRITABLE_CHANGE_EVENT,
  SIGNAL_WRITABLE_CHANGED,
  SIGNAL_CHANGE_EVENT,
  SIGNAL_CHANGED,
  N_SIGNALS
};

static guint g_settings_signals[N_SIGNALS];

G_DEFINE_TYPE (GSettings, g_settings, G_TYPE_OBJECT)

/* Signals {{{1 */
static gboolean
g_settings_real_change_event (GSettings    *settings,
                              const GQuark *keys,
                              gint          n_keys)
{
  gint i;

  if (keys == NULL)
    keys = g_settings_schema_list (settings->priv->schema, &n_keys);

  for (i = 0; i < n_keys; i++)
    g_signal_emit (settings, g_settings_signals[SIGNAL_CHANGED],
                   keys[i], g_quark_to_string (keys[i]));

  return FALSE;
}

static gboolean
g_settings_real_writable_change_event (GSettings *settings,
                                       GQuark     key)
{
  const GQuark *keys = &key;
  gint n_keys = 1;
  gint i;

  if (key == 0)
    keys = g_settings_schema_list (settings->priv->schema, &n_keys);

  for (i = 0; i < n_keys; i++)
    g_signal_emit (settings, g_settings_signals[SIGNAL_WRITABLE_CHANGED],
                   keys[i], g_quark_to_string (keys[i]));

  return FALSE;
}

static void
settings_backend_changed (GObject             *target,
                          GSettingsBackend    *backend,
                          const gchar         *key,
                          gpointer             origin_tag)
{
  GSettings *settings = G_SETTINGS (target);
  gboolean ignore_this;
  gint i;

  g_assert (settings->priv->backend == backend);

  for (i = 0; key[i] == settings->priv->path[i]; i++);

  if (settings->priv->path[i] == '\0' &&
      g_settings_schema_has_key (settings->priv->schema, key + i))
    {
      GQuark quark;

      quark = g_quark_from_string (key + i);
      g_signal_emit (settings, g_settings_signals[SIGNAL_CHANGE_EVENT],
                     0, &quark, 1, &ignore_this);
    }
}

static void
settings_backend_path_changed (GObject          *target,
                               GSettingsBackend *backend,
                               const gchar      *path,
                               gpointer          origin_tag)
{
  GSettings *settings = G_SETTINGS (target);
  gboolean ignore_this;

  g_assert (settings->priv->backend == backend);

  if (g_str_has_prefix (settings->priv->path, path))
    g_signal_emit (settings, g_settings_signals[SIGNAL_CHANGE_EVENT],
                   0, NULL, 0, &ignore_this);
}

static void
settings_backend_keys_changed (GObject             *target,
                               GSettingsBackend    *backend,
                               const gchar         *path,
                               const gchar * const *items,
                               gpointer             origin_tag)
{
  GSettings *settings = G_SETTINGS (target);
  gboolean ignore_this;
  gint i;

  g_assert (settings->priv->backend == backend);

  for (i = 0; settings->priv->path[i] &&
              settings->priv->path[i] == path[i]; i++);

  if (path[i] == '\0')
    {
      GQuark quarks[256];
      gint j, l = 0;

      for (j = 0; items[j]; j++)
         {
           const gchar *item = items[j];
           gint k;

           for (k = 0; item[k] == settings->priv->path[i + k]; k++);

           if (settings->priv->path[i + k] == '\0' &&
               g_settings_schema_has_key (settings->priv->schema, item + k))
             quarks[l++] = g_quark_from_string (item + k);

           /* "256 quarks ought to be enough for anybody!"
            * If this bites you, I'm sorry.  Please file a bug.
            */
           g_assert (l < 256);
         }

      if (l > 0)
        g_signal_emit (settings, g_settings_signals[SIGNAL_CHANGE_EVENT],
                       0, quarks, l, &ignore_this);
    }
}

static void
settings_backend_writable_changed (GObject          *target,
                                   GSettingsBackend *backend,
                                   const gchar      *key)
{
  GSettings *settings = G_SETTINGS (target);
  gboolean ignore_this;
  gint i;

  g_assert (settings->priv->backend == backend);

  for (i = 0; key[i] == settings->priv->path[i]; i++);

  if (settings->priv->path[i] == '\0' &&
      g_settings_schema_has_key (settings->priv->schema, key + i))
    g_signal_emit (settings, g_settings_signals[SIGNAL_WRITABLE_CHANGE_EVENT],
                   0, g_quark_from_string (key + i), &ignore_this);
}

static void
settings_backend_path_writable_changed (GObject          *target,
                                        GSettingsBackend *backend,
                                        const gchar      *path)
{
  GSettings *settings = G_SETTINGS (target);
  gboolean ignore_this;

  g_assert (settings->priv->backend == backend);

  if (g_str_has_prefix (settings->priv->path, path))
    g_signal_emit (settings, g_settings_signals[SIGNAL_WRITABLE_CHANGE_EVENT],
                   0, (GQuark) 0, &ignore_this);
}

/* Properties, Construction, Destruction {{{1 */
static void
g_settings_set_property (GObject      *object,
                         guint         prop_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
  GSettings *settings = G_SETTINGS (object);

  switch (prop_id)
    {
    case PROP_SCHEMA:
      g_assert (settings->priv->schema_name == NULL);
      settings->priv->schema_name = g_value_dup_string (value);
      break;

    case PROP_PATH:
      settings->priv->path = g_value_dup_string (value);
      break;

    case PROP_BACKEND:
      settings->priv->backend = g_value_dup_object (value);
      break;

    default:
      g_assert_not_reached ();
    }
}

static void
g_settings_get_property (GObject    *object,
                         guint       prop_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
  GSettings *settings = G_SETTINGS (object);

  switch (prop_id)
    {
     case PROP_SCHEMA:
      g_value_set_string (value, settings->priv->schema_name);
      break;

     case PROP_HAS_UNAPPLIED:
      g_value_set_boolean (value, g_settings_get_has_unapplied (settings));
      break;

     default:
      g_assert_not_reached ();
    }
}

static const GSettingsListenerVTable listener_vtable = {
  settings_backend_changed,
  settings_backend_path_changed,
  settings_backend_keys_changed,
  settings_backend_writable_changed,
  settings_backend_path_writable_changed
};

static void
g_settings_constructed (GObject *object)
{
  GSettings *settings = G_SETTINGS (object);
  const gchar *schema_path;

  settings->priv->schema = g_settings_schema_new (settings->priv->schema_name);
  schema_path = g_settings_schema_get_path (settings->priv->schema);

  if (settings->priv->path && schema_path && strcmp (settings->priv->path, schema_path) != 0)
    g_error ("settings object created with schema '%s' and path '%s', but "
             "path '%s' is specified by schema",
             settings->priv->schema_name, settings->priv->path, schema_path);

  if (settings->priv->path == NULL)
    {
      if (schema_path == NULL)
        g_error ("attempting to create schema '%s' without a path",
                 settings->priv->schema_name);

      settings->priv->path = g_strdup (schema_path);
    }

  if (settings->priv->backend == NULL)
    settings->priv->backend = g_settings_backend_get_default ();

  g_settings_backend_watch (settings->priv->backend,
                            &listener_vtable, G_OBJECT (settings),
                            settings->priv->main_context);
  g_settings_backend_subscribe (settings->priv->backend,
                                settings->priv->path);
}

static void
g_settings_finalize (GObject *object)
{
  GSettings *settings = G_SETTINGS (object);

  g_settings_backend_unsubscribe (settings->priv->backend,
                                  settings->priv->path);
  g_main_context_unref (settings->priv->main_context);
  g_object_unref (settings->priv->backend);
  g_object_unref (settings->priv->schema);
  g_free (settings->priv->schema_name);
  g_free (settings->priv->path);

  G_OBJECT_CLASS (g_settings_parent_class)->finalize (object);
}

static void
g_settings_init (GSettings *settings)
{
  settings->priv = G_TYPE_INSTANCE_GET_PRIVATE (settings,
                                                G_TYPE_SETTINGS,
                                                GSettingsPrivate);

  settings->priv->main_context = g_main_context_get_thread_default ();

  if (settings->priv->main_context == NULL)
    settings->priv->main_context = g_main_context_default ();

  g_main_context_ref (settings->priv->main_context);
}

static void
g_settings_class_init (GSettingsClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  class->writable_change_event = g_settings_real_writable_change_event;
  class->change_event = g_settings_real_change_event;

  object_class->set_property = g_settings_set_property;
  object_class->get_property = g_settings_get_property;
  object_class->constructed = g_settings_constructed;
  object_class->finalize = g_settings_finalize;

  g_type_class_add_private (object_class, sizeof (GSettingsPrivate));

  /**
   * GSettings::changed:
   * @settings: the object on which the signal was emitted
   * @key: the name of the key that changed
   *
   * The "changed" signal is emitted when a key has potentially changed.
   * You should call one of the g_settings_get() calls to check the new
   * value.
   *
   * This signal supports detailed connections.  You can connect to the
   * detailed signal "changed::x" in order to only receive callbacks
   * when key "x" changes.
   */
  g_settings_signals[SIGNAL_CHANGED] =
    g_signal_new ("changed", G_TYPE_SETTINGS,
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  G_STRUCT_OFFSET (GSettingsClass, changed),
                  NULL, NULL, g_cclosure_marshal_VOID__STRING, G_TYPE_NONE,
                  1, G_TYPE_STRING | G_SIGNAL_TYPE_STATIC_SCOPE);

  /**
   * GSettings::change-event:
   * @settings: the object on which the signal was emitted
   * @keys: an array of #GQuark<!-- -->s for the changed keys, or %NULL
   * @n_keys: the length of the @keys array, or 0
   * @returns: %TRUE to stop other handlers from being invoked for the
   *           event. FALSE to propagate the event further.
   *
   * The "change-event" signal is emitted once per change event that
   * affects this settings object.  You should connect to this signal
   * only if you are interested in viewing groups of changes before they
   * are split out into multiple emissions of the "changed" signal.
   * For most use cases it is more appropriate to use the "changed" signal.
   *
   * In the event that the change event applies to one or more specified
   * keys, @keys will be an array of #GQuark of length @n_keys.  In the
   * event that the change event applies to the #GSettings object as a
   * whole (ie: potentially every key has been changed) then @keys will
   * be %NULL and @n_keys will be 0.
   *
   * The default handler for this signal invokes the "changed" signal
   * for each affected key.  If any other connected handler returns
   * %TRUE then this default functionality will be supressed.
   */
  g_settings_signals[SIGNAL_CHANGE_EVENT] =
    g_signal_new ("change-event", G_TYPE_SETTINGS,
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GSettingsClass, change_event),
                  g_signal_accumulator_true_handled, NULL,
                  _gio_marshal_BOOL__POINTER_INT,
                  G_TYPE_BOOLEAN, 2, G_TYPE_POINTER, G_TYPE_INT);

  /**
   * GSettings::writable-changed:
   * @settings: the object on which the signal was emitted
   * @key: the key
   *
   * The "writable-changed" signal is emitted when the writability of a
   * key has potentially changed.  You should call
   * g_settings_is_writable() in order to determine the new status.
   *
   * This signal supports detailed connections.  You can connect to the
   * detailed signal "writable-changed::x" in order to only receive
   * callbacks when the writability of "x" changes.
   */
  g_settings_signals[SIGNAL_WRITABLE_CHANGED] =
    g_signal_new ("writable-changed", G_TYPE_SETTINGS,
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  G_STRUCT_OFFSET (GSettingsClass, changed),
                  NULL, NULL, g_cclosure_marshal_VOID__STRING, G_TYPE_NONE,
                  1, G_TYPE_STRING | G_SIGNAL_TYPE_STATIC_SCOPE);

  /**
   * GSettings::writable-change-event:
   * @settings: the object on which the signal was emitted
   * @key: the quark of the key, or 0
   * @returns: %TRUE to stop other handlers from being invoked for the
   *           event. FALSE to propagate the event further.
   *
   * The "writable-change-event" signal is emitted once per writability
   * change event that affects this settings object.  You should connect
   * to this signal if you are interested in viewing groups of changes
   * before they are split out into multiple emissions of the
   * "writable-changed" signal.  For most use cases it is more
   * appropriate to use the "writable-changed" signal.
   *
   * In the event that the writability change applies only to a single
   * key, @key will be set to the #GQuark for that key.  In the event
   * that the writability change affects the entire settings object,
   * @key will be 0.
   *
   * The default handler for this signal invokes the "writable-changed"
   * and "changed" signals for each affected key.  This is done because
   * changes in writability might also imply changes in value (if for
   * example, a new mandatory setting is introduced).  If any other
   * connected handler returns %TRUE then this default functionality
   * will be supressed.
   */
  g_settings_signals[SIGNAL_WRITABLE_CHANGE_EVENT] =
    g_signal_new ("writable-change-event", G_TYPE_SETTINGS,
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GSettingsClass, writable_change_event),
                  g_signal_accumulator_true_handled, NULL,
                  _gio_marshal_BOOLEAN__UINT, G_TYPE_BOOLEAN, 1, G_TYPE_UINT);

  /**
   * GSettings:context:
   *
   * The name of the context that the settings are stored in.
   */
  g_object_class_install_property (object_class, PROP_BACKEND,
    g_param_spec_object ("backend",
                         P_("GSettingsBackend"),
                         P_("The GSettingsBackend for this settings object"),
                         G_TYPE_SETTINGS_BACKEND, G_PARAM_CONSTRUCT_ONLY |
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * GSettings:schema:
   *
   * The name of the schema that describes the types of keys
   * for this #GSettings object.
   */
  g_object_class_install_property (object_class, PROP_SCHEMA,
    g_param_spec_string ("schema",
                         P_("Schema name"),
                         P_("The name of the schema for this settings object"),
                         NULL,
                         G_PARAM_CONSTRUCT_ONLY |
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

   /**
    * GSettings:path:
    *
    * The path within the backend where the settings are stored.
    */
   g_object_class_install_property (object_class, PROP_PATH,
     g_param_spec_string ("path",
                          P_("Base path"),
                          P_("The path within the backend where the settings are"),
                          NULL,
                          G_PARAM_CONSTRUCT_ONLY |
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

   /**
    * GSettings:has-unapplied:
    *
    * If this property is %TRUE, the #GSettings object has outstanding
    * changes that will be applied when g_settings_apply() is called.
    */
   g_object_class_install_property (object_class, PROP_HAS_UNAPPLIED,
     g_param_spec_boolean ("has-unapplied",
                           P_("Has unapplied changes"),
                           P_("TRUE if there are outstanding changes to apply()"),
                           FALSE,
                           G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

}

/* Construction (new, new_with_path, etc.) {{{1 */
/**
 * g_settings_new:
 * @schema: the name of the schema
 * @returns: a new #GSettings object
 *
 * Creates a new #GSettings object with a given schema.
 *
 * Signals on the newly created #GSettings object will be dispatched
 * via the thread-default #GMainContext in effect at the time of the
 * call to g_settings_new().  The new #GSettings will hold a reference
 * on the context.  See g_main_context_push_thread_default().
 *
 * Since: 2.26
 */
GSettings *
g_settings_new (const gchar *schema)
{
  g_return_val_if_fail (schema != NULL, NULL);

  return g_object_new (G_TYPE_SETTINGS,
                       "schema", schema,
                       NULL);
}

/**
 * g_settings_new_with_path:
 * @schema: the name of the schema
 * @path: the path to use
 * @returns: a new #GSettings object
 *
 * Creates a new #GSettings object with a given schema and path.
 *
 * You only need to do this if you want to directly create a settings
 * object with a schema that doesn't have a specified path of its own.
 * That's quite rare.
 *
 * It is a programmer error to call this function for a schema that
 * has an explicitly specified path.
 *
 * Since: 2.26
 */
GSettings *
g_settings_new_with_path (const gchar *schema,
                          const gchar *path)
{
  g_return_val_if_fail (schema != NULL, NULL);
  g_return_val_if_fail (path != NULL, NULL);

  return g_object_new (G_TYPE_SETTINGS,
                       "schema", schema,
                       "path", path,
                       NULL);
}

/**
 * g_settings_new_with_backend:
 * @schema: the name of the schema
 * @backend: the #GSettingsBackend to use
 * @returns: a new #GSettings object
 *
 * Creates a new #GSettings object with a given schema and backend.
 *
 * Creating settings objects with an different backend allows accessing settings
 * from a database other than the usual one.  For example, it may make
 * sense to pass a backend corresponding to the "defaults" settings database on
 * the system to get a settings object that modifies the system default
 * settings instead of the settings for this user.
 *
 * Since: 2.26
 */
GSettings *
g_settings_new_with_backend (const gchar      *schema,
                             GSettingsBackend *backend)
{
  g_return_val_if_fail (schema != NULL, NULL);
  g_return_val_if_fail (G_IS_SETTINGS_BACKEND (backend), NULL);

  return g_object_new (G_TYPE_SETTINGS,
                       "schema", schema,
                       "backend", backend,
                       NULL);
}

/**
 * g_settings_new_with_backend_and_path:
 * @schema: the name of the schema
 * @backend: the #GSettingsBackend to use
 * @path: the path to use
 * @returns: a new #GSettings object
 *
 * Creates a new #GSettings object with a given schema, backend and
 * path.
 *
 * This is a mix of g_settings_new_with_backend() and
 * g_settings_new_with_path().
 *
 * Since: 2.26
 */
GSettings *
g_settings_new_with_backend_and_path (const gchar      *schema,
                                      GSettingsBackend *backend,
                                      const gchar      *path)
{
  g_return_val_if_fail (schema != NULL, NULL);
  g_return_val_if_fail (G_IS_SETTINGS_BACKEND (backend), NULL);
  g_return_val_if_fail (path != NULL, NULL);

  return g_object_new (G_TYPE_SETTINGS,
                       "schema", schema,
                       "backend", backend,
                       "path", path,
                       NULL);
}

/* Internal read/write utilities, enum/flags conversion, validation {{{1 */
typedef struct
{
  GSettings *settings;
  const gchar *key;

  GSettingsSchema *schema;

  guint is_flags : 1;
  guint is_enum  : 1;

  const guint32 *strinfo;
  gsize strinfo_length;

  const gchar *unparsed;
  gchar lc_char;

  const GVariantType *type;
  GVariant *minimum, *maximum;
  GVariant *default_value;
} GSettingsKeyInfo;

static void
g_settings_get_key_info (GSettingsKeyInfo *info,
                         GSettings        *settings,
                         const gchar      *key)
{
  GVariantIter *iter;
  GVariant *data;
  guchar code;

  memset (info, 0, sizeof *info);

  iter = g_settings_schema_get_value (settings->priv->schema, key);

  info->default_value = g_variant_iter_next_value (iter);
  info->type = g_variant_get_type (info->default_value);
  info->settings = g_object_ref (settings);
  info->key = g_intern_string (key);

  while (g_variant_iter_next (iter, "(y*)", &code, &data))
    {
      switch (code)
        {
        case 'l':
          /* translation requested */
          g_variant_get (data, "(y&s)", &info->lc_char, &info->unparsed);
          break;

        case 'e':
          /* enumerated types... */
          info->is_enum = TRUE;
          goto choice;

        case 'f':
          /* flags... */
          info->is_flags = TRUE;
          goto choice;

        choice: case 'c':
          /* ..., choices, aliases */
          info->strinfo = g_variant_get_fixed_array (data,
                                                     &info->strinfo_length,
                                                     sizeof (guint32));
          break;

        case 'r':
          g_variant_get (data, "(**)", &info->minimum, &info->maximum);
          break;

        default:
          g_warning ("unknown schema extension '%c'", code);
          break;
        }

      g_variant_unref (data);
    }

  g_variant_iter_free (iter);
}

static void
g_settings_free_key_info (GSettingsKeyInfo *info)
{
  if (info->minimum)
    g_variant_unref (info->minimum);

  if (info->maximum)
    g_variant_unref (info->maximum);

  g_variant_unref (info->default_value);
  g_object_unref (info->settings);
}

static gboolean
g_settings_write_to_backend (GSettingsKeyInfo *info,
                             GVariant         *value)
{
  gboolean success;
  gchar *path;

  path = g_strconcat (info->settings->priv->path, info->key, NULL);
  success = g_settings_backend_write (info->settings->priv->backend,
                                      path, value, NULL);
  g_free (path);

  return success;
}

static gboolean
g_settings_type_check (GSettingsKeyInfo *info,
                       GVariant         *value)
{
  g_return_val_if_fail (value != NULL, FALSE);

  return g_variant_is_of_type (value, info->type);
}

static gboolean
g_settings_range_check (GSettingsKeyInfo *info,
                        GVariant         *value)
{
  if (info->minimum == NULL && info->strinfo == NULL)
    return TRUE;

  if (g_variant_is_container (value))
    {
      gboolean ok = TRUE;
      GVariantIter iter;
      GVariant *child;

      g_variant_iter_init (&iter, value);
      while (ok && (child = g_variant_iter_next_value (&iter)))
        {
          ok = g_settings_range_check (info, child);
          g_variant_unref (child);
        }

      return ok;
    }

  if (info->minimum)
    {
      return g_variant_compare (info->minimum, value) <= 0 &&
             g_variant_compare (value, info->maximum) <= 0;
    }

  return strinfo_is_string_valid (info->strinfo,
                                  info->strinfo_length,
                                  g_variant_get_string (value, NULL));
}

static GVariant *
g_settings_range_fixup (GSettingsKeyInfo *info,
                        GVariant         *value)
{
  const gchar *target;

  if (g_settings_range_check (info, value))
    return g_variant_ref (value);

  if (info->strinfo == NULL)
    return NULL;

  if (g_variant_is_container (value))
    {
      GVariantBuilder builder;
      GVariantIter iter;
      GVariant *child;

      g_variant_iter_init (&iter, value);
      g_variant_builder_init (&builder, g_variant_get_type (value));

      while ((child = g_variant_iter_next_value (&iter)))
        {
          GVariant *fixed;

          fixed = g_settings_range_fixup (info, child);
          g_variant_unref (child);

          if (fixed == NULL)
            {
              g_variant_builder_clear (&builder);
              return NULL;
            }

          g_variant_builder_add_value (&builder, fixed);
          g_variant_unref (fixed);
        }

      return g_variant_ref_sink (g_variant_builder_end (&builder));
    }

  target = strinfo_string_from_alias (info->strinfo, info->strinfo_length,
                                      g_variant_get_string (value, NULL));
  return target ? g_variant_ref_sink (g_variant_new_string (target)) : NULL;
}

static GVariant *
g_settings_read_from_backend (GSettingsKeyInfo *info)
{
  GVariant *value;
  GVariant *fixup;
  gchar *path;

  path = g_strconcat (info->settings->priv->path, info->key, NULL);
  value = g_settings_backend_read (info->settings->priv->backend,
                                   path, info->type, FALSE);
  g_free (path);

  if (value != NULL)
    {
      fixup = g_settings_range_fixup (info, value);
      g_variant_unref (value);
    }
  else
    fixup = NULL;

  return fixup;
}

static GVariant *
g_settings_get_translated_default (GSettingsKeyInfo *info)
{
  const gchar *translated;
  GError *error = NULL;
  const gchar *domain;
  GVariant *value;

  if (info->lc_char == '\0')
    /* translation not requested for this key */
    return NULL;

  domain = g_settings_schema_get_gettext_domain (info->settings->priv->schema);

  if (info->lc_char == 't')
    translated = g_dcgettext (domain, info->unparsed, LC_TIME);
  else
    translated = g_dgettext (domain, info->unparsed);

  if (translated == info->unparsed)
    /* the default value was not translated */
    return NULL;

  /* try to parse the translation of the unparsed default */
  value = g_variant_parse (info->type, translated, NULL, NULL, &error);

  if (value == NULL)
    {
      g_warning ("Failed to parse translated string `%s' for "
                 "key `%s' in schema `%s': %s", info->unparsed, info->key,
                 info->settings->priv->schema_name, error->message);
      g_warning ("Using untranslated default instead.");
      g_error_free (error);
    }

  else if (!g_settings_range_check (info, value))
    {
      g_warning ("Translated default `%s' for key `%s' in schema `%s' "
                 "is outside of valid range", info->unparsed, info->key,
                 info->settings->priv->schema_name);
      g_variant_unref (value);
      value = NULL;
    }

  return value;
}

static gint
g_settings_to_enum (GSettingsKeyInfo *info,
                    GVariant         *value)
{
  gboolean it_worked;
  guint result;

  it_worked = strinfo_enum_from_string (info->strinfo, info->strinfo_length,
                                        g_variant_get_string (value, NULL),
                                        &result);

  /* 'value' can only come from the backend after being filtered for validity,
   * from the translation after being filtered for validity, or from the schema
   * itself (which the schema compiler checks for validity).  If this assertion
   * fails then it's really a bug in GSettings or the schema compiler...
   */
  g_assert (it_worked);

  return result;
}

static GVariant *
g_settings_from_enum (GSettingsKeyInfo *info,
                      gint              value)
{
  const gchar *string;

  string = strinfo_string_from_enum (info->strinfo,
                                     info->strinfo_length,
                                     value);

  if (string == NULL)
    return NULL;

  return g_variant_new_string (string);
}

static guint
g_settings_to_flags (GSettingsKeyInfo *info,
                     GVariant         *value)
{
  GVariantIter iter;
  const gchar *flag;
  guint result;

  result = 0;
  g_variant_iter_init (&iter, value);
  while (g_variant_iter_next (&iter, "&s", &flag))
    {
      gboolean it_worked;
      guint flag_value;

      it_worked = strinfo_enum_from_string (info->strinfo,
                                            info->strinfo_length,
                                            flag, &flag_value);
      /* as in g_settings_to_enum() */
      g_assert (it_worked);

      result |= flag_value;
    }

  return result;
}

static GVariant *
g_settings_from_flags (GSettingsKeyInfo *info,
                       guint             value)
{
  GVariantBuilder builder;
  gint i;

  g_variant_builder_init (&builder, G_VARIANT_TYPE ("as"));

  for (i = 0; i < 32; i++)
    if (value & (1u << i))
      {
        const gchar *string;

        string = strinfo_string_from_enum (info->strinfo,
                                           info->strinfo_length,
                                           1u << i);

        if (string == NULL)
          {
            g_variant_builder_clear (&builder);
            return NULL;
          }

        g_variant_builder_add (&builder, "s", string);
      }

  return g_variant_builder_end (&builder);
}

/* Public Get/Set API {{{1 (get, get_value, set, set_value, get_mapped) */
/**
 * g_settings_get_value:
 * @settings: a #GSettings object
 * @key: the key to get the value for
 * @returns: a new #GVariant
 *
 * Gets the value that is stored in @settings for @key.
 *
 * It is a programmer error to give a @key that isn't contained in the
 * schema for @settings.
 *
 * Since: 2.26
 */
GVariant *
g_settings_get_value (GSettings   *settings,
                      const gchar *key)
{
  GSettingsKeyInfo info;
  GVariant *value;

  g_return_val_if_fail (G_IS_SETTINGS (settings), NULL);
  g_return_val_if_fail (key != NULL, NULL);

  g_settings_get_key_info (&info, settings, key);
  value = g_settings_read_from_backend (&info);

  if (value == NULL)
    value = g_settings_get_translated_default (&info);

  if (value == NULL)
    value = g_variant_ref (info.default_value);

  g_settings_free_key_info (&info);

  return value;
}

/**
 * g_settings_get_enum:
 * @settings: a #GSettings object
 * @key: the key to get the value for
 * @returns: the enum value
 *
 * Gets the value that is stored in @settings for @key and converts it
 * to the enum value that it represents.
 *
 * In order to use this function the type of the value must be a string
 * and it must be marked in the schema file as an enumerated type.
 *
 * It is a programmer error to give a @key that isn't contained in the
 * schema for @settings or is not marked as an enumerated type.
 *
 * If the value stored in the configuration database is not a valid
 * value for the enumerated type then this function will return the
 * default value.
 *
 * Since: 2.26
 **/
gint
g_settings_get_enum (GSettings   *settings,
                     const gchar *key)
{
  GSettingsKeyInfo info;
  GVariant *value;
  gint result;

  g_return_val_if_fail (G_IS_SETTINGS (settings), -1);
  g_return_val_if_fail (key != NULL, -1);

  g_settings_get_key_info (&info, settings, key);

  if (!info.is_enum)
    {
      g_critical ("g_settings_get_enum() called on key `%s' which is not "
                  "associated with an enumerated type", info.key);
      g_settings_free_key_info (&info);
      return -1;
    }

  value = g_settings_read_from_backend (&info);

  if (value == NULL)
    value = g_settings_get_translated_default (&info);

  if (value == NULL)
    value = g_variant_ref (info.default_value);

  result = g_settings_to_enum (&info, value);
  g_settings_free_key_info (&info);
  g_variant_unref (value);

  return result;
}

/**
 * g_settings_set_enum:
 * @settings: a #GSettings object
 * @key: a key, within @settings
 * @value: an enumerated value
 * @returns: %TRUE, if the set succeeds
 *
 * Looks up the enumerated type nick for @value and writes it to @key,
 * within @settings.
 *
 * It is a programmer error to give a @key that isn't contained in the
 * schema for @settings or is not marked as an enumerated type, or for
 * @value not to be a valid value for the named type.
 *
 * After performing the write, accessing @key directly with
 * g_settings_get_string() will return the 'nick' associated with
 * @value.
 **/
gboolean
g_settings_set_enum (GSettings   *settings,
                     const gchar *key,
                     gint         value)
{
  GSettingsKeyInfo info;
  GVariant *variant;
  gboolean success;

  g_return_val_if_fail (G_IS_SETTINGS (settings), FALSE);
  g_return_val_if_fail (key != NULL, FALSE);

  g_settings_get_key_info (&info, settings, key);

  if (!info.is_enum)
    {
      g_critical ("g_settings_set_enum() called on key `%s' which is not "
                  "associated with an enumerated type", info.key);
      return FALSE;
    }

  if (!(variant = g_settings_from_enum (&info, value)))
    {
      g_critical ("g_settings_set_enum(): invalid enum value %d for key `%s' "
                  "in schema `%s'.  Doing nothing.", value, info.key,
                  info.settings->priv->schema_name);
      g_settings_free_key_info (&info);
      return FALSE;
    }

  success = g_settings_write_to_backend (&info, variant);
  g_settings_free_key_info (&info);

  return success;
}

/**
 * g_settings_get_flags:
 * @settings: a #GSettings object
 * @key: the key to get the value for
 * @returns: the flags value
 *
 * Gets the value that is stored in @settings for @key and converts it
 * to the flags value that it represents.
 *
 * In order to use this function the type of the value must be an array
 * of strings and it must be marked in the schema file as an flags type.
 *
 * It is a programmer error to give a @key that isn't contained in the
 * schema for @settings or is not marked as a flags type.
 *
 * If the value stored in the configuration database is not a valid
 * value for the flags type then this function will return the default
 * value.
 *
 * Since: 2.26
 **/
guint
g_settings_get_flags (GSettings   *settings,
                      const gchar *key)
{
  GSettingsKeyInfo info;
  GVariant *value;
  guint result;

  g_return_val_if_fail (G_IS_SETTINGS (settings), -1);
  g_return_val_if_fail (key != NULL, -1);

  g_settings_get_key_info (&info, settings, key);

  if (!info.is_flags)
    {
      g_critical ("g_settings_get_flags() called on key `%s' which is not "
                  "associated with a flags type", info.key);
      g_settings_free_key_info (&info);
      return -1;
    }

  value = g_settings_read_from_backend (&info);

  if (value == NULL)
    value = g_settings_get_translated_default (&info);

  if (value == NULL)
    value = g_variant_ref (info.default_value);

  result = g_settings_to_flags (&info, value);
  g_settings_free_key_info (&info);
  g_variant_unref (value);

  return result;
}

/**
 * g_settings_set_flags:
 * @settings: a #GSettings object
 * @key: a key, within @settings
 * @value: a flags value
 * @returns: %TRUE, if the set succeeds
 *
 * Looks up the flags type nicks for the bits specified by @value, puts
 * them in an array of strings and writes the array to @key, withing
 * @settings.
 *
 * It is a programmer error to give a @key that isn't contained in the
 * schema for @settings or is not marked as a flags type, or for @value
 * to contain any bits that are not value for the named type.
 *
 * After performing the write, accessing @key directly with
 * g_settings_get_strv() will return an array of 'nicks'; one for each
 * bit in @value.
 **/
gboolean
g_settings_set_flags (GSettings   *settings,
                      const gchar *key,
                      guint        value)
{
  GSettingsKeyInfo info;
  GVariant *variant;
  gboolean success;

  g_return_val_if_fail (G_IS_SETTINGS (settings), FALSE);
  g_return_val_if_fail (key != NULL, FALSE);

  g_settings_get_key_info (&info, settings, key);

  if (!info.is_flags)
    {
      g_critical ("g_settings_set_flags() called on key `%s' which is not "
                  "associated with a flags type", info.key);
      return FALSE;
    }

  if (!(variant = g_settings_from_flags (&info, value)))
    {
      g_critical ("g_settings_set_flags(): invalid flags value 0x%08x "
                  "for key `%s' in schema `%s'.  Doing nothing.",
                  value, info.key, info.settings->priv->schema_name);
      g_settings_free_key_info (&info);
      return FALSE;
    }

  success = g_settings_write_to_backend (&info, variant);
  g_settings_free_key_info (&info);

  return success;
}

/**
 * g_settings_set_value:
 * @settings: a #GSettings object
 * @key: the name of the key to set
 * @value: a #GVariant of the correct type
 * @returns: %TRUE if setting the key succeeded,
 *     %FALSE if the key was not writable
 *
 * Sets @key in @settings to @value.
 *
 * It is a programmer error to give a @key that isn't contained in the
 * schema for @settings or for @value to have the incorrect type, per
 * the schema.
 *
 * If @value is floating then this function consumes the reference.
 *
 * Since: 2.26
 **/
gboolean
g_settings_set_value (GSettings   *settings,
                      const gchar *key,
                      GVariant    *value)
{
  GSettingsKeyInfo info;

  g_return_val_if_fail (G_IS_SETTINGS (settings), FALSE);
  g_return_val_if_fail (key != NULL, FALSE);

  g_settings_get_key_info (&info, settings, key);
  g_return_val_if_fail (g_settings_type_check (&info, value), FALSE);
  g_return_val_if_fail (g_settings_range_check (&info, value), FALSE);
  g_settings_free_key_info (&info);

  return g_settings_write_to_backend (&info, value);
}

/**
 * g_settings_get:
 * @settings: a #GSettings object
 * @key: the key to get the value for
 * @format: a #GVariant format string
 * @...: arguments as per @format
 *
 * Gets the value that is stored at @key in @settings.
 *
 * A convenience function that combines g_settings_get_value() with
 * g_variant_get().
 *
 * It is a programmer error to give a @key that isn't contained in the
 * schema for @settings or for the #GVariantType of @format to mismatch
 * the type given in the schema.
 *
 * Since: 2.26
 */
void
g_settings_get (GSettings   *settings,
                const gchar *key,
                const gchar *format,
                ...)
{
  GVariant *value;
  va_list ap;

  value = g_settings_get_value (settings, key);

  va_start (ap, format);
  g_variant_get_va (value, format, NULL, &ap);
  va_end (ap);

  g_variant_unref (value);
}

/**
 * g_settings_set:
 * @settings: a #GSettings object
 * @key: the name of the key to set
 * @format: a #GVariant format string
 * @...: arguments as per @format
 * @returns: %TRUE if setting the key succeeded,
 *     %FALSE if the key was not writable
 *
 * Sets @key in @settings to @value.
 *
 * A convenience function that combines g_settings_set_value() with
 * g_variant_new().
 *
 * It is a programmer error to give a @key that isn't contained in the
 * schema for @settings or for the #GVariantType of @format to mismatch
 * the type given in the schema.
 *
 * Since: 2.26
 */
gboolean
g_settings_set (GSettings   *settings,
                const gchar *key,
                const gchar *format,
                ...)
{
  GVariant *value;
  va_list ap;

  va_start (ap, format);
  value = g_variant_new_va (format, NULL, &ap);
  va_end (ap);

  return g_settings_set_value (settings, key, value);
}

/**
 * g_settings_get_mapped:
 * @settings: a #GSettings object
 * @key: the key to get the value for
 * @mapping: the function to map the value in the settings database to
 *           the value used by the application
 * @user_data: user data for @mapping
 * @returns: the result, which may be %NULL
 *
 * Gets the value that is stored at @key in @settings, subject to
 * application-level validation/mapping.
 *
 * You should use this function when the application needs to perform
 * some processing on the value of the key (for example, parsing).  The
 * @mapping function performs that processing.  If the function
 * indicates that the processing was unsuccessful (due to a parse error,
 * for example) then the mapping is tried again with another value.

 * This allows a robust 'fall back to defaults' behaviour to be
 * implemented somewhat automatically.
 *
 * The first value that is tried is the user's setting for the key.  If
 * the mapping function fails to map this value, other values may be
 * tried in an unspecified order (system or site defaults, translated
 * schema default values, untranslated schema default values, etc).
 *
 * If the mapping function fails for all possible values, one additional
 * attempt is made: the mapping function is called with a %NULL value.
 * If the mapping function still indicates failure at this point then
 * the application will be aborted.
 *
 * The result parameter for the @mapping function is pointed to a
 * #gpointer which is initially set to %NULL.  The same pointer is given
 * to each invocation of @mapping.  The final value of that #gpointer is
 * what is returned by this function.  %NULL is valid; it is returned
 * just as any other value would be.
 **/
gpointer
g_settings_get_mapped (GSettings           *settings,
                       const gchar         *key,
                       GSettingsGetMapping  mapping,
                       gpointer             user_data)
{
  gpointer result = NULL;
  GSettingsKeyInfo info;
  GVariant *value;
  gboolean okay;

  g_return_val_if_fail (G_IS_SETTINGS (settings), NULL);
  g_return_val_if_fail (key != NULL, NULL);
  g_return_val_if_fail (mapping != NULL, NULL);

  g_settings_get_key_info (&info, settings, key);

  if ((value = g_settings_read_from_backend (&info)))
    {
      okay = mapping (value, &result, user_data);
      g_variant_unref (value);
      if (okay) goto okay;
    }

  if ((value = g_settings_get_translated_default (&info)))
    {
      okay = mapping (value, &result, user_data);
      g_variant_unref (value);
      if (okay) goto okay;
    }

  if (mapping (info.default_value, &result, user_data))
    goto okay;

  if (!mapping (NULL, &result, user_data))
    g_error ("The mapping function given to g_settings_get_mapped() for key "
             "`%s' in schema `%s' returned FALSE when given a NULL value.",
             key, settings->priv->schema_name);

 okay:
  g_settings_free_key_info (&info);

  return result;
}

/* Convenience API (get, set_string, int, double, boolean, strv) {{{1 */
/**
 * g_settings_get_string:
 * @settings: a #GSettings object
 * @key: the key to get the value for
 * @returns: a newly-allocated string
 *
 * Gets the value that is stored at @key in @settings.
 *
 * A convenience variant of g_settings_get() for strings.
 *
 * It is a programmer error to give a @key that isn't specified as
 * having a string type in the schema for @settings.
 *
 * Since: 2.26
 */
gchar *
g_settings_get_string (GSettings   *settings,
                       const gchar *key)
{
  GVariant *value;
  gchar *result;

  value = g_settings_get_value (settings, key);
  result = g_variant_dup_string (value, NULL);
  g_variant_unref (value);

  return result;
}

/**
 * g_settings_set_string:
 * @settings: a #GSettings object
 * @key: the name of the key to set
 * @value: the value to set it to
 * @returns: %TRUE if setting the key succeeded,
 *     %FALSE if the key was not writable
 *
 * Sets @key in @settings to @value.
 *
 * A convenience variant of g_settings_set() for strings.
 *
 * It is a programmer error to give a @key that isn't specified as
 * having a string type in the schema for @settings.
 *
 * Since: 2.26
 */
gboolean
g_settings_set_string (GSettings   *settings,
                       const gchar *key,
                       const gchar *value)
{
  return g_settings_set_value (settings, key, g_variant_new_string (value));
}

/**
 * g_settings_get_int:
 * @settings: a #GSettings object
 * @key: the key to get the value for
 * @returns: an integer
 *
 * Gets the value that is stored at @key in @settings.
 *
 * A convenience variant of g_settings_get() for 32-bit integers.
 *
 * It is a programmer error to give a @key that isn't specified as
 * having a int32 type in the schema for @settings.
 *
 * Since: 2.26
 */
gint
g_settings_get_int (GSettings   *settings,
                    const gchar *key)
{
  GVariant *value;
  gint result;

  value = g_settings_get_value (settings, key);
  result = g_variant_get_int32 (value);
  g_variant_unref (value);

  return result;
}

/**
 * g_settings_set_int:
 * @settings: a #GSettings object
 * @key: the name of the key to set
 * @value: the value to set it to
 * @returns: %TRUE if setting the key succeeded,
 *     %FALSE if the key was not writable
 *
 * Sets @key in @settings to @value.
 *
 * A convenience variant of g_settings_set() for 32-bit integers.
 *
 * It is a programmer error to give a @key that isn't specified as
 * having a int32 type in the schema for @settings.
 *
 * Since: 2.26
 */
gboolean
g_settings_set_int (GSettings   *settings,
                    const gchar *key,
                    gint         value)
{
  return g_settings_set_value (settings, key, g_variant_new_int32 (value));
}

/**
 * g_settings_get_double:
 * @settings: a #GSettings object
 * @key: the key to get the value for
 * @returns: a double
 *
 * Gets the value that is stored at @key in @settings.
 *
 * A convenience variant of g_settings_get() for doubles.
 *
 * It is a programmer error to give a @key that isn't specified as
 * having a 'double' type in the schema for @settings.
 *
 * Since: 2.26
 */
gdouble
g_settings_get_double (GSettings   *settings,
                       const gchar *key)
{
  GVariant *value;
  gdouble result;

  value = g_settings_get_value (settings, key);
  result = g_variant_get_double (value);
  g_variant_unref (value);

  return result;
}

/**
 * g_settings_set_double:
 * @settings: a #GSettings object
 * @key: the name of the key to set
 * @value: the value to set it to
 * @returns: %TRUE if setting the key succeeded,
 *     %FALSE if the key was not writable
 *
 * Sets @key in @settings to @value.
 *
 * A convenience variant of g_settings_set() for doubles.
 *
 * It is a programmer error to give a @key that isn't specified as
 * having a 'double' type in the schema for @settings.
 *
 * Since: 2.26
 */
gboolean
g_settings_set_double (GSettings   *settings,
                       const gchar *key,
                       gdouble      value)
{
  return g_settings_set_value (settings, key, g_variant_new_double (value));
}

/**
 * g_settings_get_boolean:
 * @settings: a #GSettings object
 * @key: the key to get the value for
 * @returns: a boolean
 *
 * Gets the value that is stored at @key in @settings.
 *
 * A convenience variant of g_settings_get() for booleans.
 *
 * It is a programmer error to give a @key that isn't specified as
 * having a boolean type in the schema for @settings.
 *
 * Since: 2.26
 */
gboolean
g_settings_get_boolean (GSettings  *settings,
                       const gchar *key)
{
  GVariant *value;
  gboolean result;

  value = g_settings_get_value (settings, key);
  result = g_variant_get_boolean (value);
  g_variant_unref (value);

  return result;
}

/**
 * g_settings_set_boolean:
 * @settings: a #GSettings object
 * @key: the name of the key to set
 * @value: the value to set it to
 * @returns: %TRUE if setting the key succeeded,
 *     %FALSE if the key was not writable
 *
 * Sets @key in @settings to @value.
 *
 * A convenience variant of g_settings_set() for booleans.
 *
 * It is a programmer error to give a @key that isn't specified as
 * having a boolean type in the schema for @settings.
 *
 * Since: 2.26
 */
gboolean
g_settings_set_boolean (GSettings  *settings,
                       const gchar *key,
                       gboolean     value)
{
  return g_settings_set_value (settings, key, g_variant_new_boolean (value));
}

/**
 * g_settings_get_strv:
 * @settings: a #GSettings object
 * @key: the key to get the value for
 * @returns: a newly-allocated, %NULL-terminated array of strings
 *
 * Gets the value that is stored at @key in @settings.
 *
 * A convenience variant of g_settings_get() for string arrays.
 *
 * It is a programmer error to give a @key that isn't specified as
 * having an array of strings type in the schema for @settings.
 *
 * Since: 2.26
 */
gchar **
g_settings_get_strv (GSettings   *settings,
                     const gchar *key)
{
  GVariant *value;
  gchar **result;

  value = g_settings_get_value (settings, key);
  result = g_variant_dup_strv (value, NULL);
  g_variant_unref (value);

  return result;
}

/**
 * g_settings_set_strv:
 * @settings: a #GSettings object
 * @key: the name of the key to set
 * @value: (allow-none): the value to set it to, or %NULL
 * @returns: %TRUE if setting the key succeeded,
 *     %FALSE if the key was not writable
 *
 * Sets @key in @settings to @value.
 *
 * A convenience variant of g_settings_set() for string arrays.  If
 * @value is %NULL, then @key is set to be the empty array.
 *
 * It is a programmer error to give a @key that isn't specified as
 * having an array of strings type in the schema for @settings.
 *
 * Since: 2.26
 */
gboolean
g_settings_set_strv (GSettings           *settings,
                     const gchar         *key,
                     const gchar * const *value)
{
  GVariant *array;

  if (value != NULL)
    array = g_variant_new_strv (value, -1);
  else
    array = g_variant_new_strv (NULL, 0);

  return g_settings_set_value (settings, key, array);
}

/* Delayed apply (delay, apply, revert, get_has_unapplied) {{{1 */
/**
 * g_settings_delay:
 * @settings: a #GSettings object
 *
 * Changes the #GSettings object into 'delay-apply' mode. In this
 * mode, changes to @settings are not immediately propagated to the
 * backend, but kept locally until g_settings_apply() is called.
 *
 * Since: 2.26
 */
void
g_settings_delay (GSettings *settings)
{
  g_return_if_fail (G_IS_SETTINGS (settings));

  if (settings->priv->delayed)
    return;

  settings->priv->delayed =
    g_delayed_settings_backend_new (settings->priv->backend,
                                    settings,
                                    settings->priv->main_context);
  g_settings_backend_unwatch (settings->priv->backend, G_OBJECT (settings));
  g_object_unref (settings->priv->backend);

  settings->priv->backend = G_SETTINGS_BACKEND (settings->priv->delayed);
  g_settings_backend_watch (settings->priv->backend,
                            &listener_vtable, G_OBJECT (settings),
                            settings->priv->main_context);
}

/**
 * g_settings_apply:
 * @settings: a #GSettings instance
 *
 * Applies any changes that have been made to the settings.  This
 * function does nothing unless @settings is in 'delay-apply' mode;
 * see g_settings_delay().  In the normal case settings are always
 * applied immediately.
 **/
void
g_settings_apply (GSettings *settings)
{
  if (settings->priv->delayed)
    {
      GDelayedSettingsBackend *delayed;

      delayed = G_DELAYED_SETTINGS_BACKEND (settings->priv->backend);
      g_delayed_settings_backend_apply (delayed);
    }
}

/**
 * g_settings_revert:
 * @settings: a #GSettings instance
 *
 * Reverts all non-applied changes to the settings.  This function
 * does nothing unless @settings is in 'delay-apply' mode; see
 * g_settings_delay().  In the normal case settings are always applied
 * immediately.
 *
 * Change notifications will be emitted for affected keys.
 **/
void
g_settings_revert (GSettings *settings)
{
  if (settings->priv->delayed)
    {
      GDelayedSettingsBackend *delayed;

      delayed = G_DELAYED_SETTINGS_BACKEND (settings->priv->backend);
      g_delayed_settings_backend_revert (delayed);
    }
}

/**
 * g_settings_get_has_unapplied:
 * @settings: a #GSettings object
 * @returns: %TRUE if @settings has unapplied changes
 *
 * Returns whether the #GSettings object has any unapplied
 * changes.  This can only be the case if it is in 'delayed-apply' mode.
 *
 * Since: 2.26
 */
gboolean
g_settings_get_has_unapplied (GSettings *settings)
{
  g_return_val_if_fail (G_IS_SETTINGS (settings), FALSE);

  return settings->priv->delayed &&
         g_delayed_settings_backend_get_has_unapplied (
           G_DELAYED_SETTINGS_BACKEND (settings->priv->backend));
}

/* Extra API (reset, sync, get_child, is_writable, list_*) {{{1 */
/**
 * g_settings_reset:
 * @settings: a #GSettings object
 * @key: the name of a key
 *
 * Resets @key to its default value.
 *
 * This call resets the key, as much as possible, to its default value.
 * That might the value specified in the schema or the one set by the
 * administrator.
 **/
void
g_settings_reset (GSettings *settings,
                  const gchar *key)
{
  gchar *path;

  path = g_strconcat (settings->priv->path, key, NULL);
  g_settings_backend_reset (settings->priv->backend, path, NULL);
  g_free (path);
}

/**
 * g_settings_sync:
 *
 * Ensures that all pending operations for the given are complete for
 * the default backend.
 *
 * Writes made to a #GSettings are handled asynchronously.  For this
 * reason, it is very unlikely that the changes have it to disk by the
 * time g_settings_set() returns.
 *
 * This call will block until all of the writes have made it to the
 * backend.  Since the mainloop is not running, no change notifications
 * will be dispatched during this call (but some may be queued by the
 * time the call is done).
 **/
void
g_settings_sync (void)
{
  g_settings_backend_sync_default ();
}

/**
 * g_settings_is_writable:
 * @settings: a #GSettings object
 * @name: the name of a key
 * @returns: %TRUE if the key @name is writable
 *
 * Finds out if a key can be written or not
 *
 * Since: 2.26
 */
gboolean
g_settings_is_writable (GSettings   *settings,
                        const gchar *name)
{
  gboolean writable;
  gchar *path;

  g_return_val_if_fail (G_IS_SETTINGS (settings), FALSE);

  path = g_strconcat (settings->priv->path, name, NULL);
  writable = g_settings_backend_get_writable (settings->priv->backend, path);
  g_free (path);

  return writable;
}

/**
 * g_settings_get_child:
 * @settings: a #GSettings object
 * @name: the name of the 'child' schema
 * @returns: a 'child' settings object
 *
 * Creates a 'child' settings object which has a base path of
 * <replaceable>base-path</replaceable>/@name", where
 * <replaceable>base-path</replaceable> is the base path of @settings.
 *
 * The schema for the child settings object must have been declared
 * in the schema of @settings using a <tag class="starttag">child</tag> element.
 *
 * Since: 2.26
 */
GSettings *
g_settings_get_child (GSettings   *settings,
                      const gchar *name)
{
  const gchar *child_schema;
  gchar *child_path;
  gchar *child_name;
  GSettings *child;

  g_return_val_if_fail (G_IS_SETTINGS (settings), NULL);

  child_name = g_strconcat (name, "/", NULL);
  child_schema = g_settings_schema_get_string (settings->priv->schema,
                                               child_name);
  if (child_schema == NULL)
    g_error ("Schema '%s' has no child '%s'",
             settings->priv->schema_name, name);

  child_path = g_strconcat (settings->priv->path, child_name, NULL);
  child = g_object_new (G_TYPE_SETTINGS,
                        "schema", child_schema,
                        "path", child_path,
                        NULL);
  g_free (child_path);
  g_free (child_name);

  return child;
}

/**
 * g_settings_list_keys:
 * @settings: a #GSettings object
 * @returns: a list of the keys on @settings
 *
 * Introspects the list of keys on @settings.
 *
 * You should probably not be calling this function from "normal" code
 * (since you should already know what keys are in your schema).  This
 * function is intended for introspection reasons.
 *
 * You should free the return value with g_strfreev() when you are done
 * with it.
 */
gchar **
g_settings_list_keys (GSettings *settings)
{
  const GQuark *keys;
  gchar **strv;
  gint n_keys;
  gint i, j;

  keys = g_settings_schema_list (settings->priv->schema, &n_keys);
  strv = g_new (gchar *, n_keys + 1);
  for (i = j = 0; i < n_keys; i++)
    {
      const gchar *key = g_quark_to_string (keys[i]);

      if (!g_str_has_suffix (key, "/"))
        strv[j++] = g_strdup (key);
    }
  strv[j] = NULL;

  return strv;
}

/**
 * g_settings_list_children:
 * @settings: a #GSettings object
 * @returns: a list of the children on @settings
 *
 * Gets the list of children on @settings.
 *
 * The list is exactly the list of strings for which it is not an error
 * to call g_settings_get_child().
 *
 * For GSettings objects that are lists, this value can change at any
 * time and you should connect to the "children-changed" signal to watch
 * for those changes.  Note that there is a race condition here: you may
 * request a child after listing it only for it to have been destroyed
 * in the meantime.  For this reason, g_settings_get_chuld() may return
 * %NULL even for a child that was listed by this function.
 *
 * For GSettings objects that are not lists, you should probably not be
 * calling this function from "normal" code (since you should already
 * know what children are in your schema).  This function may still be
 * useful there for introspection reasons, however.
 *
 * You should free the return value with g_strfreev() when you are done
 * with it.
 */
gchar **
g_settings_list_children (GSettings *settings)
{
  const GQuark *keys;
  gchar **strv;
  gint n_keys;
  gint i, j;

  keys = g_settings_schema_list (settings->priv->schema, &n_keys);
  strv = g_new (gchar *, n_keys + 1);
  for (i = j = 0; i < n_keys; i++)
    {
      const gchar *key = g_quark_to_string (keys[i]);

      if (g_str_has_suffix (key, "/"))
        {
          gint length = strlen (key);

          strv[j] = g_memdup (key, length);
          strv[j][length - 1] = '\0';
          j++;
        }
    }
  strv[j] = NULL;

  return strv;
}

/* Binding {{{1 */
typedef struct
{
  GSettingsKeyInfo info;
  GObject *object;

  GSettingsBindGetMapping get_mapping;
  GSettingsBindSetMapping set_mapping;
  gpointer user_data;
  GDestroyNotify destroy;

  guint writable_handler_id;
  guint property_handler_id;
  const GParamSpec *property;
  guint key_handler_id;

  /* prevent recursion */
  gboolean running;
} GSettingsBinding;

static void
g_settings_binding_free (gpointer data)
{
  GSettingsBinding *binding = data;

  g_assert (!binding->running);

  if (binding->writable_handler_id)
    g_signal_handler_disconnect (binding->info.settings,
                                 binding->writable_handler_id);

  if (binding->key_handler_id)
    g_signal_handler_disconnect (binding->info.settings,
                                 binding->key_handler_id);

  if (g_signal_handler_is_connected (binding->object,
                                     binding->property_handler_id))
  g_signal_handler_disconnect (binding->object,
                               binding->property_handler_id);

  g_settings_free_key_info (&binding->info);

  if (binding->destroy)
    binding->destroy (binding->user_data);

  g_slice_free (GSettingsBinding, binding);
}

static GQuark
g_settings_binding_quark (const char *property)
{
  GQuark quark;
  gchar *tmp;

  tmp = g_strdup_printf ("gsettingsbinding-%s", property);
  quark = g_quark_from_string (tmp);
  g_free (tmp);

  return quark;
}

static void
g_settings_binding_key_changed (GSettings   *settings,
                                const gchar *key,
                                gpointer     user_data)
{
  GSettingsBinding *binding = user_data;
  GValue value = { 0, };
  GVariant *variant;

  g_assert (settings == binding->info.settings);
  g_assert (key == binding->info.key);

  if (binding->running)
    return;

  binding->running = TRUE;

  g_value_init (&value, binding->property->value_type);

  variant = g_settings_read_from_backend (&binding->info);
  if (variant && !binding->get_mapping (&value, variant, binding->user_data))
    {
      /* silently ignore errors in the user's config database */
      g_variant_unref (variant);
      variant = NULL;
    }

  if (variant == NULL)
    {
      variant = g_settings_get_translated_default (&binding->info);
      if (variant &&
          !binding->get_mapping (&value, variant, binding->user_data))
        {
          /* flag translation errors with a warning */
          g_warning ("Translated default `%s' for key `%s' in schema `%s' "
                     "was rejected by the binding mapping function",
                     binding->info.unparsed, binding->info.key,
                     binding->info.settings->priv->schema_name);
          g_variant_unref (variant);
          variant = NULL;
        }
    }

  if (variant == NULL)
    {
      variant = g_variant_ref (binding->info.default_value);
      if (!binding->get_mapping (&value, variant, binding->user_data))
        g_error ("The schema default value for key `%s' in schema `%s' "
                 "was rejected by the binding mapping function.",
                 binding->info.key,
                 binding->info.settings->priv->schema_name);
    }

  g_object_set_property (binding->object, binding->property->name, &value);
  g_variant_unref (variant);
  g_value_unset (&value);

  binding->running = FALSE;
}

static void
g_settings_binding_property_changed (GObject          *object,
                                     const GParamSpec *pspec,
                                     gpointer          user_data)
{
  GSettingsBinding *binding = user_data;
  GValue value = { 0, };
  GVariant *variant;

  g_assert (object == binding->object);
  g_assert (pspec == binding->property);

  if (binding->running)
    return;

  binding->running = TRUE;

  g_value_init (&value, pspec->value_type);
  g_object_get_property (object, pspec->name, &value);
  if ((variant = binding->set_mapping (&value, binding->info.type,
                                       binding->user_data)))
    {
      if (g_variant_is_floating (variant))
        g_variant_ref_sink (variant);

      if (!g_settings_type_check (&binding->info, variant))
        {
          g_critical ("binding mapping function for key `%s' returned "
                      "GVariant of type `%s' when type `%s' was requested",
                      binding->info.key, g_variant_get_type_string (variant),
                      g_variant_type_dup_string (binding->info.type));
          return;
        }

      if (!g_settings_range_check (&binding->info, variant))
        {
          g_critical ("GObject property `%s' on a `%s' object is out of "
                      "schema-specified range for key `%s' of `%s': %s",
                      binding->property->name,
                      g_type_name (binding->property->owner_type),
                      binding->info.key,
                      binding->info.settings->priv->schema_name,
                      g_variant_print (variant, TRUE));
          return;
        }

      g_settings_write_to_backend (&binding->info, variant);
      g_variant_unref (variant);
    }
  g_value_unset (&value);

  binding->running = FALSE;
}

static gboolean
g_settings_bind_invert_boolean_get_mapping (GValue   *value,
                                            GVariant *variant,
                                            gpointer  user_data)
{
  g_value_set_boolean (value, !g_variant_get_boolean (variant));
  return TRUE;
}

static GVariant *
g_settings_bind_invert_boolean_set_mapping (const GValue       *value,
                                            const GVariantType *expected_type,
                                            gpointer            user_data)
{
  return g_variant_new_boolean (!g_value_get_boolean (value));
}

/**
 * g_settings_bind:
 * @settings: a #GSettings object
 * @key: the key to bind
 * @object: a #GObject
 * @property: the name of the property to bind
 * @flags: flags for the binding
 *
 * Create a binding between the @key in the @settings object
 * and the property @property of @object.
 *
 * The binding uses the default GIO mapping functions to map
 * between the settings and property values. These functions
 * handle booleans, numeric types and string types in a
 * straightforward way. Use g_settings_bind_with_mapping() if
 * you need a custom mapping, or map between types that are not
 * supported by the default mapping functions.
 *
 * Unless the @flags include %G_SETTINGS_BIND_NO_SENSITIVITY, this
 * function also establishes a binding between the writability of
 * @key and the "sensitive" property of @object (if @object has
 * a boolean property by that name). See g_settings_bind_writable()
 * for more details about writable bindings.
 *
 * Note that the lifecycle of the binding is tied to the object,
 * and that you can have only one binding per object property.
 * If you bind the same property twice on the same object, the second
 * binding overrides the first one.
 *
 * Since: 2.26
 */
void
g_settings_bind (GSettings          *settings,
                 const gchar        *key,
                 gpointer            object,
                 const gchar        *property,
                 GSettingsBindFlags  flags)
{
  GSettingsBindGetMapping get_mapping = NULL;
  GSettingsBindSetMapping set_mapping = NULL;

  if (flags & G_SETTINGS_BIND_INVERT_BOOLEAN)
    {
      get_mapping = g_settings_bind_invert_boolean_get_mapping;
      set_mapping = g_settings_bind_invert_boolean_set_mapping;

      /* can't pass this flag to g_settings_bind_with_mapping() */
      flags &= ~G_SETTINGS_BIND_INVERT_BOOLEAN;
    }

  g_settings_bind_with_mapping (settings, key, object, property, flags,
                                get_mapping, set_mapping, NULL, NULL);
}

/**
 * g_settings_bind_with_mapping:
 * @settings: a #GSettings object
 * @key: the key to bind
 * @object: a #GObject
 * @property: the name of the property to bind
 * @flags: flags for the binding
 * @get_mapping: a function that gets called to convert values
 *     from @settings to @object, or %NULL to use the default GIO mapping
 * @set_mapping: a function that gets called to convert values
 *     from @object to @settings, or %NULL to use the default GIO mapping
 * @user_data: data that gets passed to @get_mapping and @set_mapping
 * @destroy: #GDestroyNotify function for @user_data
 *
 * Create a binding between the @key in the @settings object
 * and the property @property of @object.
 *
 * The binding uses the provided mapping functions to map between
 * settings and property values.
 *
 * Note that the lifecycle of the binding is tied to the object,
 * and that you can have only one binding per object property.
 * If you bind the same property twice on the same object, the second
 * binding overrides the first one.
 *
 * Since: 2.26
 */
void
g_settings_bind_with_mapping (GSettings               *settings,
                              const gchar             *key,
                              gpointer                 object,
                              const gchar             *property,
                              GSettingsBindFlags       flags,
                              GSettingsBindGetMapping  get_mapping,
                              GSettingsBindSetMapping  set_mapping,
                              gpointer                 user_data,
                              GDestroyNotify           destroy)
{
  GSettingsBinding *binding;
  GObjectClass *objectclass;
  gchar *detailed_signal;
  GQuark binding_quark;

  g_return_if_fail (G_IS_SETTINGS (settings));
  g_return_if_fail (~flags & G_SETTINGS_BIND_INVERT_BOOLEAN);

  objectclass = G_OBJECT_GET_CLASS (object);

  binding = g_slice_new0 (GSettingsBinding);
  g_settings_get_key_info (&binding->info, settings, key);
  binding->object = object;
  binding->property = g_object_class_find_property (objectclass, property);
  binding->user_data = user_data;
  binding->destroy = destroy;
  binding->get_mapping = get_mapping ? get_mapping : g_settings_get_mapping;
  binding->set_mapping = set_mapping ? set_mapping : g_settings_set_mapping;

  if (!(flags & (G_SETTINGS_BIND_GET | G_SETTINGS_BIND_SET)))
    flags |= G_SETTINGS_BIND_GET | G_SETTINGS_BIND_SET;

  if (binding->property == NULL)
    {
      g_critical ("g_settings_bind: no property '%s' on class '%s'",
                  property, G_OBJECT_TYPE_NAME (object));
      return;
    }

  if ((flags & G_SETTINGS_BIND_GET) &&
      (binding->property->flags & G_PARAM_WRITABLE) == 0)
    {
      g_critical ("g_settings_bind: property '%s' on class '%s' is not "
                  "writable", property, G_OBJECT_TYPE_NAME (object));
      return;
    }
  if ((flags & G_SETTINGS_BIND_SET) &&
      (binding->property->flags & G_PARAM_READABLE) == 0)
    {
      g_critical ("g_settings_bind: property '%s' on class '%s' is not "
                  "readable", property, G_OBJECT_TYPE_NAME (object));
      return;
    }

  if (get_mapping == g_settings_bind_invert_boolean_get_mapping)
    {
      /* g_settings_bind_invert_boolean_get_mapping() is a private
       * function, so if we are here it means that g_settings_bind() was
       * called with G_SETTINGS_BIND_INVERT_BOOLEAN.
       *
       * Ensure that both sides are boolean.
       */

      if (binding->property->value_type != G_TYPE_BOOLEAN)
        {
          g_critical ("g_settings_bind: G_SETTINGS_BIND_INVERT_BOOLEAN "
                      "was specified, but property `%s' on type `%s' has "
                      "type `%s'", property, G_OBJECT_TYPE_NAME (object),
                      g_type_name ((binding->property->value_type)));
          return;
        }

      if (!g_variant_type_equal (binding->info.type, G_VARIANT_TYPE_BOOLEAN))
        {
          g_critical ("g_settings_bind: G_SETTINGS_BIND_INVERT_BOOLEAN "
                      "was specified, but key `%s' on schema `%s' has "
                      "type `%s'", key, settings->priv->schema_name,
                      g_variant_type_dup_string (binding->info.type));
          return;
        }

    }

  else if (((get_mapping == NULL && (flags & G_SETTINGS_BIND_GET)) ||
            (set_mapping == NULL && (flags & G_SETTINGS_BIND_SET))) &&
           !g_settings_mapping_is_compatible (binding->property->value_type,
                                              binding->info.type))
    {
      g_critical ("g_settings_bind: property '%s' on class '%s' has type "
                  "'%s' which is not compatible with type '%s' of key '%s' "
                  "on schema '%s'", property, G_OBJECT_TYPE_NAME (object),
                  g_type_name (binding->property->value_type),
                  g_variant_type_dup_string (binding->info.type), key,
                  settings->priv->schema_name);
      return;
    }

  if ((flags & G_SETTINGS_BIND_SET) &&
      (~flags & G_SETTINGS_BIND_NO_SENSITIVITY))
    {
      GParamSpec *sensitive;

      sensitive = g_object_class_find_property (objectclass, "sensitive");

      if (sensitive && sensitive->value_type == G_TYPE_BOOLEAN &&
          (sensitive->flags & G_PARAM_WRITABLE))
        g_settings_bind_writable (settings, binding->info.key,
                                  object, "sensitive", FALSE);
    }

  if (flags & G_SETTINGS_BIND_SET)
    {
      detailed_signal = g_strdup_printf ("notify::%s", property);
      binding->property_handler_id =
        g_signal_connect (object, detailed_signal,
                          G_CALLBACK (g_settings_binding_property_changed),
                          binding);
      g_free (detailed_signal);

      if (~flags & G_SETTINGS_BIND_GET)
        g_settings_binding_property_changed (object,
                                             binding->property,
                                             binding);
    }

  if (flags & G_SETTINGS_BIND_GET)
    {
      if (~flags & G_SETTINGS_BIND_GET_NO_CHANGES)
        {
          detailed_signal = g_strdup_printf ("changed::%s", key);
          binding->key_handler_id =
            g_signal_connect (settings, detailed_signal,
                              G_CALLBACK (g_settings_binding_key_changed),
                              binding);
          g_free (detailed_signal);
        }

      g_settings_binding_key_changed (settings, binding->info.key, binding);
    }

  binding_quark = g_settings_binding_quark (property);
  g_object_set_qdata_full (object, binding_quark,
                           binding, g_settings_binding_free);
}

/* Writability binding {{{1 */
typedef struct
{
  GSettings *settings;
  gpointer object;
  const gchar *key;
  const gchar *property;
  gboolean inverted;
  gulong handler_id;
} GSettingsWritableBinding;

static void
g_settings_writable_binding_free (gpointer data)
{
  GSettingsWritableBinding *binding = data;

  g_signal_handler_disconnect (binding->settings, binding->handler_id);
  g_object_unref (binding->settings);
  g_slice_free (GSettingsWritableBinding, binding);
}

static void
g_settings_binding_writable_changed (GSettings   *settings,
                                     const gchar *key,
                                     gpointer     user_data)
{
  GSettingsWritableBinding *binding = user_data;
  gboolean writable;

  g_assert (settings == binding->settings);
  g_assert (key == binding->key);

  writable = g_settings_is_writable (settings, key);

  if (binding->inverted)
    writable = !writable;

  g_object_set (binding->object, binding->property, writable, NULL);
}

/**
 * g_settings_bind_writable:
 * @settings: a #GSettings object
 * @key: the key to bind
 * @object: a #GObject
 * @property: the name of a boolean property to bind
 * @inverted: whether to 'invert' the value
 *
 * Create a binding between the writability of @key in the
 * @settings object and the property @property of @object.
 * The property must be boolean; "sensitive" or "visible"
 * properties of widgets are the most likely candidates.
 *
 * Writable bindings are always uni-directional; changes of the
 * writability of the setting will be propagated to the object
 * property, not the other way.
 *
 * When the @inverted argument is %TRUE, the binding inverts the
 * value as it passes from the setting to the object, i.e. @property
 * will be set to %TRUE if the key is <emphasis>not</emphasis>
 * writable.
 *
 * Note that the lifecycle of the binding is tied to the object,
 * and that you can have only one binding per object property.
 * If you bind the same property twice on the same object, the second
 * binding overrides the first one.
 *
 * Since: 2.26
 */
void
g_settings_bind_writable (GSettings   *settings,
                          const gchar *key,
                          gpointer     object,
                          const gchar *property,
                          gboolean     inverted)
{
  GSettingsWritableBinding *binding;
  gchar *detailed_signal;
  GParamSpec *pspec;

  g_return_if_fail (G_IS_SETTINGS (settings));

  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (object), property);
  if (pspec == NULL)
    {
      g_critical ("g_settings_bind_writable: no property '%s' on class '%s'",
                  property, G_OBJECT_TYPE_NAME (object));
      return;
    }
  if ((pspec->flags & G_PARAM_WRITABLE) == 0)
    {
      g_critical ("g_settings_bind_writable: property '%s' on class '%s' is not writable",
                  property, G_OBJECT_TYPE_NAME (object));
      return;
    }

  binding = g_slice_new (GSettingsWritableBinding);
  binding->settings = g_object_ref (settings);
  binding->object = object;
  binding->key = g_intern_string (key);
  binding->property = g_intern_string (property);
  binding->inverted = inverted;

  detailed_signal = g_strdup_printf ("writable-changed::%s", key);
  binding->handler_id =
    g_signal_connect (settings, detailed_signal,
                      G_CALLBACK (g_settings_binding_writable_changed),
                      binding);
  g_free (detailed_signal);

  g_object_set_qdata_full (object, g_settings_binding_quark (property),
                           binding, g_settings_writable_binding_free);

  g_settings_binding_writable_changed (settings, binding->key, binding);
}

/**
 * g_settings_unbind:
 * @object: the object
 * @property: the property whose binding is removed
 *
 * Removes an existing binding for @property on @object.
 *
 * Note that bindings are automatically removed when the
 * object is finalized, so it is rarely necessary to call this
 * function.
 *
 * Since: 2.26
 */
void
g_settings_unbind (gpointer     object,
                   const gchar *property)
{
  GQuark binding_quark;

  binding_quark = g_settings_binding_quark (property);
  g_object_set_qdata (object, binding_quark, NULL);
}

/* Epilogue {{{1 */

/* vim:set foldmethod=marker: */
