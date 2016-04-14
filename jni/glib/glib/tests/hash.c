/* GLIB - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 * Copyright (C) 1999 The Free Software Foundation
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GLib Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GLib Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GLib at ftp://ftp.gtk.org/pub/gtk/. 
 */

#undef G_DISABLE_ASSERT
#undef G_LOG_DOMAIN

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#if STDC_HEADERS
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#endif

#include <glib.h>



int array[10000];

static void
fill_hash_table_and_array (GHashTable *hash_table)
{
  int i;

  for (i = 0; i < 10000; i++)
    {
      array[i] = i;
      g_hash_table_insert (hash_table, &array[i], &array[i]);
    }
}

static void
init_result_array (int result_array[10000])
{
  int i;

  for (i = 0; i < 10000; i++)
    result_array[i] = -1;
}

static void
verify_result_array (int array[10000])
{
  int i;

  for (i = 0; i < 10000; i++)
    g_assert (array[i] == i);
}

static void
handle_pair (gpointer key, gpointer value, int result_array[10000])
{
  int n;

  g_assert (key == value);

  n = *((int *) value);

  g_assert (n >= 0 && n < 10000);
  g_assert (result_array[n] == -1);

  result_array[n] = n;
}

static gboolean
my_hash_callback_remove (gpointer key,
                         gpointer value,
                         gpointer user_data)
{
  int *d = value;

  if ((*d) % 2)
    return TRUE;

  return FALSE;
}

static void
my_hash_callback_remove_test (gpointer key,
                              gpointer value,
                              gpointer user_data)
{
  int *d = value;

  if ((*d) % 2)
    g_assert_not_reached ();
}

static void
my_hash_callback (gpointer key,
                  gpointer value,
                  gpointer user_data)
{
  handle_pair (key, value, user_data);
}

static guint
my_hash (gconstpointer key)
{
  return (guint) *((const gint*) key);
}

static gboolean
my_hash_equal (gconstpointer a,
               gconstpointer b)
{
  return *((const gint*) a) == *((const gint*) b);
}



/*
 * This is a simplified version of the pathalias hashing function.
 * Thanks to Steve Belovin and Peter Honeyman
 *
 * hash a string into a long int.  31 bit crc (from andrew appel).
 * the crc table is computed at run time by crcinit() -- we could
 * precompute, but it takes 1 clock tick on a 750.
 *
 * This fast table calculation works only if POLY is a prime polynomial
 * in the field of integers modulo 2.  Since the coefficients of a
 * 32-bit polynomial won't fit in a 32-bit word, the high-order bit is
 * implicit.  IT MUST ALSO BE THE CASE that the coefficients of orders
 * 31 down to 25 are zero.  Happily, we have candidates, from
 * E. J.  Watson, "Primitive Polynomials (Mod 2)", Math. Comp. 16 (1962):
 *      x^32 + x^7 + x^5 + x^3 + x^2 + x^1 + x^0
 *      x^31 + x^3 + x^0
 *
 * We reverse the bits to get:
 *      111101010000000000000000000000001 but drop the last 1
 *         f   5   0   0   0   0   0   0
 *      010010000000000000000000000000001 ditto, for 31-bit crc
 *         4   8   0   0   0   0   0   0
 */

#define POLY 0x48000000L        /* 31-bit polynomial (avoids sign problems) */

static guint CrcTable[128];

/*
 - crcinit - initialize tables for hash function
 */
static void crcinit(void)
{
        int i, j;
        guint sum;

        for (i = 0; i < 128; ++i) {
                sum = 0L;
                for (j = 7 - 1; j >= 0; --j)
                        if (i & (1 << j))
                                sum ^= POLY >> j;
                CrcTable[i] = sum;
        }
}

/*
 - hash - Honeyman's nice hashing function
 */
static guint honeyman_hash(gconstpointer key)
{
        const gchar *name = (const gchar *) key;
        gint size;
        guint sum = 0;

        g_assert (name != NULL);
        g_assert (*name != 0);

        size = strlen(name);

        while (size--) {
                sum = (sum >> 7) ^ CrcTable[(sum ^ (*name++)) & 0x7f];
        }

        return(sum);
}


static gboolean second_hash_cmp (gconstpointer a, gconstpointer b)
{
  return (strcmp (a, b) == 0);
}



static guint one_hash(gconstpointer key)
{
  return 1;
}


static void not_even_foreach (gpointer       key,
                                 gpointer       value,
                                 gpointer       user_data)
{
  const char *_key = (const char *) key;
  const char *_value = (const char *) value;
  int i;
  char val [20];

  g_assert (_key != NULL);
  g_assert (*_key != 0);
  g_assert (_value != NULL);
  g_assert (*_value != 0);

  i = atoi (_key);

  sprintf (val, "%d value", i);
  g_assert (strcmp (_value, val) == 0);

  g_assert ((i % 2) != 0);
  g_assert (i != 3);
}


static gboolean remove_even_foreach (gpointer       key,
                                 gpointer       value,
                                 gpointer       user_data)
{
  const char *_key = (const char *) key;
  const char *_value = (const char *) value;
  int i;
  char val [20];

  g_assert (_key != NULL);
  g_assert (*_key != 0);
  g_assert (_value != NULL);
  g_assert (*_value != 0);

  i = atoi (_key);

  sprintf (val, "%d value", i);
  g_assert (strcmp (_value, val) == 0);

  return ((i % 2) == 0) ? TRUE : FALSE;
}




static void second_hash_test (gconstpointer d)
{
     gboolean simple_hash = GPOINTER_TO_INT (d);

     int       i;
     char      key[20] = "", val[20]="", *v, *orig_key, *orig_val;
     GHashTable     *h;
     gboolean found;

     crcinit ();

     h = g_hash_table_new_full (simple_hash ? one_hash : honeyman_hash,
                                second_hash_cmp,
                                g_free, g_free);
     g_assert (h != NULL);
     for (i=0; i<20; i++)
          {
          sprintf (key, "%d", i);
          g_assert (atoi (key) == i);

          sprintf (val, "%d value", i);
          g_assert (atoi (val) == i);

          g_hash_table_insert (h, g_strdup (key), g_strdup (val));
          }

     g_assert (g_hash_table_size (h) == 20);

     for (i=0; i<20; i++)
          {
          sprintf (key, "%d", i);
          g_assert (atoi(key) == i);

          v = (char *) g_hash_table_lookup (h, key);

          g_assert (v != NULL);
          g_assert (*v != 0);
          g_assert (atoi (v) == i);
          }

     sprintf (key, "%d", 3);
     g_hash_table_remove (h, key);
     g_assert (g_hash_table_size (h) == 19);
     g_hash_table_foreach_remove (h, remove_even_foreach, NULL);
     g_assert (g_hash_table_size (h) == 9);
     g_hash_table_foreach (h, not_even_foreach, NULL);

     for (i=0; i<20; i++)
          {
          sprintf (key, "%d", i);
          g_assert (atoi(key) == i);

          sprintf (val, "%d value", i);
          g_assert (atoi (val) == i);

          orig_key = orig_val = NULL;
          found = g_hash_table_lookup_extended (h, key,
                                                (gpointer)&orig_key,
                                                (gpointer)&orig_val);
          if ((i % 2) == 0 || i == 3)
            {
              g_assert (!found);
              continue;
            }

          g_assert (found);

          g_assert (orig_key != NULL);
          g_assert (strcmp (key, orig_key) == 0);

          g_assert (orig_val != NULL);
          g_assert (strcmp (val, orig_val) == 0);
          }

    g_hash_table_destroy (h);
}

static gboolean find_first     (gpointer key, 
                                gpointer value, 
                                gpointer user_data)
{
  gint *v = value; 
  gint *test = user_data;
  return (*v == *test);
}


static void direct_hash_test (void)
{
     gint       i, rc;
     GHashTable     *h;

     h = g_hash_table_new (NULL, NULL);
     g_assert (h != NULL);
     for (i=1; i<=20; i++)
          {
          g_hash_table_insert (h, GINT_TO_POINTER (i),
                               GINT_TO_POINTER (i + 42));
          }

     g_assert (g_hash_table_size (h) == 20);

     for (i=1; i<=20; i++)
          {
          rc = GPOINTER_TO_INT (
                g_hash_table_lookup (h, GINT_TO_POINTER (i)));

          g_assert (rc != 0);
          g_assert ((rc - 42) == i);
          }

    g_hash_table_destroy (h);
}

static void int64_hash_test (void)
{
     gint       i, rc;
     GHashTable     *h;
     gint64     values[20];
     gint64 key;

     h = g_hash_table_new (g_int64_hash, g_int64_equal);
     g_assert (h != NULL);
     for (i=0; i<20; i++)
          {
          values[i] = i + 42;
          g_hash_table_insert (h, &values[i], GINT_TO_POINTER (i + 42));
          }

     g_assert (g_hash_table_size (h) == 20);

     for (i=0; i<20; i++)
          {
          key = i + 42;
          rc = GPOINTER_TO_INT (g_hash_table_lookup (h, &key));

          g_assert_cmpint (rc, ==, i + 42);
          }

    g_hash_table_destroy (h);
}

static void double_hash_test (void)
{
     gint       i, rc;
     GHashTable     *h;
     gdouble values[20];
     gdouble key;

     h = g_hash_table_new (g_double_hash, g_double_equal);
     g_assert (h != NULL);
     for (i=0; i<20; i++)
          {
          values[i] = i + 42.5;
          g_hash_table_insert (h, &values[i], GINT_TO_POINTER (i + 42));
          }

     g_assert (g_hash_table_size (h) == 20);

     for (i=0; i<20; i++)
          {
          key = i + 42.5;
          rc = GPOINTER_TO_INT (g_hash_table_lookup (h, &key));

          g_assert_cmpint (rc, ==, i + 42);
          }

    g_hash_table_destroy (h);
}

static void
string_free (gpointer data)
{
  GString *s = data;

  g_string_free (s, TRUE);
}

static void string_hash_test (void)
{
     gint       i, rc;
     GHashTable     *h;
     GString *s;

     h = g_hash_table_new_full ((GHashFunc)g_string_hash, (GEqualFunc)g_string_equal, string_free, NULL);
     g_assert (h != NULL);
     for (i=0; i<20; i++)
          {
          s = g_string_new ("");
          g_string_append_printf (s, "%d", i + 42);
          g_string_append_c (s, '.');
          g_string_prepend_unichar (s, 0x2301);
          g_hash_table_insert (h, s, GINT_TO_POINTER (i + 42));
          }

     g_assert (g_hash_table_size (h) == 20);

     s = g_string_new ("");
     for (i=0; i<20; i++)
          {
          g_string_assign (s, "");
          g_string_append_printf (s, "%d", i + 42);
          g_string_append_c (s, '.');
          g_string_prepend_unichar (s, 0x2301);
          rc = GPOINTER_TO_INT (g_hash_table_lookup (h, s));

          g_assert_cmpint (rc, ==, i + 42);
          }

    g_string_free (s, TRUE);
    g_hash_table_destroy (h);
}

static void
test_hash_misc (void)
{
  GHashTable *hash_table;
  gint i;
  gint value = 120;
  gint *pvalue;
  GList *keys, *values;
  gint keys_len, values_len;
  GHashTableIter iter;
  gpointer ikey, ivalue;
  int result_array[10000];

  hash_table = g_hash_table_new (my_hash, my_hash_equal);
  fill_hash_table_and_array (hash_table);
  pvalue = g_hash_table_find (hash_table, find_first, &value);
  if (!pvalue || *pvalue != value)
    g_assert_not_reached();

  keys = g_hash_table_get_keys (hash_table);
  if (!keys)
    g_assert_not_reached ();

  values = g_hash_table_get_values (hash_table);
  if (!values)
    g_assert_not_reached ();

  keys_len = g_list_length (keys);
  values_len = g_list_length (values);
  if (values_len != keys_len &&  keys_len != g_hash_table_size (hash_table))
    g_assert_not_reached ();

  g_list_free (keys);
  g_list_free (values);

  init_result_array (result_array);
  g_hash_table_iter_init (&iter, hash_table);
  for (i = 0; i < 10000; i++)
    {
      g_assert (g_hash_table_iter_next (&iter, &ikey, &ivalue));

      handle_pair (ikey, ivalue, result_array);

      if (i % 2)
        g_hash_table_iter_remove (&iter);
    }
  g_assert (! g_hash_table_iter_next (&iter, &ikey, &ivalue));
  g_assert (g_hash_table_size (hash_table) == 5000);
  verify_result_array (result_array);

  fill_hash_table_and_array (hash_table);

  init_result_array (result_array);
  g_hash_table_foreach (hash_table, my_hash_callback, result_array);
  verify_result_array (result_array);

  for (i = 0; i < 10000; i++)
    g_hash_table_remove (hash_table, &array[i]);

  fill_hash_table_and_array (hash_table);

  if (g_hash_table_foreach_remove (hash_table, my_hash_callback_remove, NULL) != 5000 ||
      g_hash_table_size (hash_table) != 5000)
    g_assert_not_reached();

  g_hash_table_foreach (hash_table, my_hash_callback_remove_test, NULL);
  g_hash_table_destroy (hash_table);
}

static gint destroy_counter;

static void
value_destroy (gpointer value)
{
  destroy_counter++;
}

static void
test_hash_ref (void)
{
  GHashTable *h;
  GHashTableIter iter;
  gchar *key, *value;
  gboolean abc_seen = FALSE;
  gboolean cde_seen = FALSE;
  gboolean xyz_seen = FALSE;

  h = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, value_destroy);
  g_hash_table_insert (h, "abc", "ABC");
  g_hash_table_insert (h, "cde", "CDE");
  g_hash_table_insert (h, "xyz", "XYZ");

  g_assert_cmpint (g_hash_table_size (h), == , 3);

  g_hash_table_iter_init (&iter, h);

  while (g_hash_table_iter_next (&iter, (gpointer*)&key, (gpointer*)&value))
    {
      if (strcmp (key, "abc") == 0)
        {
          g_assert_cmpstr (value, ==, "ABC");
          abc_seen = TRUE;
          g_hash_table_iter_steal (&iter);
        }
      else if (strcmp (key, "cde") == 0)
        {
          g_assert_cmpstr (value, ==, "CDE");
          cde_seen = TRUE;
        }
      else if (strcmp (key, "xyz") == 0)
        {
          g_assert_cmpstr (value, ==, "XYZ");
          xyz_seen = TRUE;
        }
    }
  g_assert_cmpint (destroy_counter, ==, 0);

  g_assert (g_hash_table_iter_get_hash_table (&iter) == h);
  g_assert (abc_seen && cde_seen && xyz_seen);
  g_assert_cmpint (g_hash_table_size (h), == , 2);

  g_hash_table_ref (h);
  g_hash_table_destroy (h);
  g_assert_cmpint (g_hash_table_size (h), == , 0);
  g_assert_cmpint (destroy_counter, ==, 2);
  g_hash_table_insert (h, "uvw", "UVW");
  g_hash_table_unref (h);
  g_assert_cmpint (destroy_counter, ==, 3);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/hash/misc", test_hash_misc);
  g_test_add_data_func ("/hash/one", GINT_TO_POINTER (TRUE), second_hash_test);
  g_test_add_data_func ("/hash/honeyman", GINT_TO_POINTER (FALSE), second_hash_test);
  g_test_add_func ("/hash/direct", direct_hash_test);
  g_test_add_func ("/hash/int64", int64_hash_test);
  g_test_add_func ("/hash/double", double_hash_test);
  g_test_add_func ("/hash/string", string_hash_test);
  g_test_add_func ("/hash/ref", test_hash_ref);

  return g_test_run ();

}
