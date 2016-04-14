/*
 * Copyright © 2010 Codethink Limited
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

/* Prologue {{{1 */

#include "gtimezoneprivate.h"

#include <string.h>
#include <stdlib.h>
#include <signal.h>

#include "gmappedfile.h"
#include "gtestutils.h"
#include "gfileutils.h"
#include "gstrfuncs.h"
#include "ghash.h"
#include "gthread.h"
#include "gbuffer.h"

/**
 * SECTION:timezone
 * @title: GTimeZone
 * @short_description: A structure representing a time zone
 * @see_also: #GDateTime
 *
 * #GTimeZone is a structure that represents a time zone, at no
 * particular point in time.  It is refcounted and immutable.
 *
 * #GTimeZone is available since GLib 2.26.
 */

/**
 * GTimeZone:
 *
 * #GDateTime is an opaque structure whose members cannot be accessed
 * directly.
 *
 * Since: 2.26
 **/

/* zoneinfo file format {{{1 */

/* unaligned */
typedef struct { gchar bytes[8]; } gint64_be;
typedef struct { gchar bytes[4]; } gint32_be;
typedef struct { gchar bytes[4]; } guint32_be;

static inline gint64 gint64_from_be (const gint64_be be) {
  gint64 tmp; memcpy (&tmp, &be, sizeof tmp); return GINT64_FROM_BE (tmp);
}

static inline gint32 gint32_from_be (const gint32_be be) {
  gint32 tmp; memcpy (&tmp, &be, sizeof tmp); return GINT32_FROM_BE (tmp);
}

static inline guint32 guint32_from_be (const guint32_be be) {
  guint32 tmp; memcpy (&tmp, &be, sizeof tmp); return GUINT32_FROM_BE (tmp);
}

struct tzhead
{
  gchar      tzh_magic[4];
  gchar      tzh_version;
  guchar     tzh_reserved[15];

  guint32_be tzh_ttisgmtcnt;
  guint32_be tzh_ttisstdcnt;
  guint32_be tzh_leapcnt;
  guint32_be tzh_timecnt;
  guint32_be tzh_typecnt;
  guint32_be tzh_charcnt;
};

struct ttinfo
{
  gint32_be tt_gmtoff;
  guint8    tt_isdst;
  guint8    tt_abbrind;
};

/* GTimeZone structure and lifecycle {{{1 */
struct _GTimeZone
{
  gchar   *name;

  GBuffer *zoneinfo;

  const struct tzhead *header;
  const struct ttinfo *infos;
  const gint64_be     *trans;
  const guint8        *indices;
  const gchar         *abbrs;
  gint                 timecnt;

  gint     ref_count;
};

G_LOCK_DEFINE_STATIC (time_zones);
static GHashTable/*<string?, GTimeZone>*/ *time_zones;

static guint
g_str_hash0 (gconstpointer data)
{
  return data ? g_str_hash (data) : 0;
}

static gboolean
g_str_equal0 (gconstpointer a,
              gconstpointer b)
{
  if (a == b)
    return TRUE;

  if (!a || !b)
    return FALSE;

  return g_str_equal (a, b);
}

/**
 * g_time_zone_unref:
 * @tz: a #GTimeZone
 *
 * Decreases the reference count on @tz.
 *
 * Since: 2.26
 **/
void
g_time_zone_unref (GTimeZone *tz)
{
  g_assert (tz->ref_count > 0);

  if (g_atomic_int_dec_and_test (&tz->ref_count))
    {
      G_LOCK(time_zones);
      g_hash_table_remove (time_zones, tz->name);
      G_UNLOCK(time_zones);

      if (tz->zoneinfo)
        g_buffer_unref (tz->zoneinfo);

      g_free (tz->name);

      g_slice_free (GTimeZone, tz);
    }
}

/**
 * g_time_zone_ref:
 * @tz: a #GTimeZone
 *
 * Increases the reference count on @tz.
 *
 * Returns: a new reference to @tz.
 *
 * Since: 2.26
 **/
GTimeZone *
g_time_zone_ref (GTimeZone *tz)
{
  g_assert (tz->ref_count > 0);

  g_atomic_int_inc (&tz->ref_count);

  return tz;
}

/* fake zoneinfo creation (for RFC3339/ISO 8601 timezones) {{{1 */
/*
 * parses strings of the form 'hh' 'hhmm' or 'hh:mm' where:
 *  - hh is 00 to 23
 *  - mm is 00 to 59
 */
gboolean
parse_time (const gchar *time,
            gint32      *offset)
{
  if (*time < '0' || '2' < *time)
    return FALSE;

  *offset = 10 * 60 * 60 * (*time++ - '0');

  if (*time < '0' || '9' < *time)
    return FALSE;

  *offset += 60 * 60 * (*time++ - '0');

  if (*offset > 23 * 60 * 60)
    return FALSE;

  if (*time == '\0')
    return TRUE;

  if (*time == ':')
    time++;

  if (*time < '0' || '5' < *time)
    return FALSE;

  *offset += 10 * 60 * (*time++ - '0');

  if (*time < '0' || '9' < *time)
    return FALSE;

  *offset += 60 * (*time++ - '0');

  return *time == '\0';
}

gboolean
parse_constant_offset (const gchar *name,
                       gint32      *offset)
{
  switch (*name++)
    {
    case 'Z':
      *offset = 0;
      return !*name;

    case '+':
      return parse_time (name, offset);

    case '-':
      if (parse_time (name, offset))
        {
          *offset = -*offset;
          return TRUE;
        }

    default:
      return FALSE;
    }
}

static GBuffer *
zone_for_constant_offset (const gchar *name)
{
  const gchar fake_zoneinfo_headers[] =
    "TZif" "2..." "...." "...." "...."
    "\0\0\0\0" "\0\0\0\0" "\0\0\0\0" "\0\0\0\0" "\0\0\0\0" "\0\0\0\0"
    "TZif" "2..." "...." "...." "...."
    "\0\0\0\0" "\0\0\0\0" "\0\0\0\0" "\0\0\0\0" "\0\0\0\1" "\0\0\0\7";
  struct {
    struct tzhead headers[2];
    struct ttinfo info;
    gchar abbr[8];
  } *fake;
  gint32 offset;

  if (name == NULL || !parse_constant_offset (name, &offset))
    return NULL;

  offset = GINT32_TO_BE (offset);

  fake = g_malloc (sizeof *fake);
  memcpy (fake, fake_zoneinfo_headers, sizeof fake_zoneinfo_headers);
  memcpy (&fake->info.tt_gmtoff, &offset, sizeof offset);
  fake->info.tt_isdst = FALSE;
  fake->info.tt_abbrind = 0;
  strcpy (fake->abbr, name);

  return g_buffer_new_take_data (fake, sizeof *fake);
}

/* Construction {{{1 */
/**
 * g_time_zone_new:
 * @identifier: (allow-none): a timezone identifier
 *
 * Creates a #GTimeZone corresponding to @identifier.
 *
 * @identifier can either be an RFC3339/ISO 8601 time offset or
 * something that would pass as a valid value for the
 * <varname>TZ</varname> environment variable (including %NULL).
 *
 * Valid RFC3339 time offsets are <literal>"Z"</literal> (for UTC) or
 * <literal>"±hh:mm"</literal>.  ISO 8601 additionally specifies
 * <literal>"±hhmm"</literal> and <literal>"±hh"</literal>.
 *
 * The <varname>TZ</varname> environment variable typically corresponds
 * to the name of a file in the zoneinfo database, but there are many
 * other possibilities.  Note that those other possibilities are not
 * currently implemented, but are planned.
 *
 * g_time_zone_new_local() calls this function with the value of the
 * <varname>TZ</varname> environment variable.  This function itself is
 * independent of the value of <varname>TZ</varname>, but if @identifier
 * is %NULL then <filename>/etc/localtime</filename> will be consulted
 * to discover the correct timezone.
 *
 * See <ulink
 * url='http://tools.ietf.org/html/rfc3339#section-5.6'>RFC3339
 * §5.6</ulink> for a precise definition of valid RFC3339 time offsets
 * (the <varname>time-offset</varname> expansion) and ISO 8601 for the
 * full list of valid time offsets.  See <ulink
 * url='http://www.gnu.org/s/libc/manual/html_node/TZ-Variable.html'>The
 * GNU C Library manual</ulink> for an explanation of the possible
 * values of the <varname>TZ</varname> environment variable.
 *
 * You should release the return value by calling g_time_zone_unref()
 * when you are done with it.
 *
 * Returns: the requested timezone
 *
 * Since: 2.26
 **/
GTimeZone *
g_time_zone_new (const gchar *identifier)
{
  GTimeZone *tz;

  G_LOCK (time_zones);
  if (time_zones == NULL)
    time_zones = g_hash_table_new (g_str_hash0,
                                   g_str_equal0);

  tz = g_hash_table_lookup (time_zones, identifier);
  if (tz == NULL)
    {
      tz = g_slice_new0 (GTimeZone);
      tz->name = g_strdup (identifier);
      tz->ref_count = 0;

      tz->zoneinfo = zone_for_constant_offset (identifier);

      if (tz->zoneinfo == NULL)
        {
          gchar *filename;

          if (identifier != NULL)
            {
              const gchar *tzdir;

              tzdir = getenv ("TZDIR");
              if (tzdir == NULL)
                tzdir = "/usr/share/zoneinfo";

              filename = g_build_filename (tzdir, identifier, NULL);
            }
          else
            filename = g_strdup ("/etc/localtime");

          tz->zoneinfo = (GBuffer *) g_mapped_file_new (filename, FALSE, NULL);
          g_free (filename);
        }

      if (tz->zoneinfo != NULL)
        {
          const struct tzhead *header = tz->zoneinfo->data;
          gsize size = tz->zoneinfo->size;

          /* we only bother to support version 2 */
          if (size < sizeof (struct tzhead) || memcmp (header, "TZif2", 5))
            {
              g_buffer_unref (tz->zoneinfo);
              tz->zoneinfo = NULL;
            }
          else
            {
              gint typecnt;

              /* we trust the file completely. */
              tz->header = (const struct tzhead *)
                (((const gchar *) (header + 1)) +
                  guint32_from_be(header->tzh_ttisgmtcnt) +
                  guint32_from_be(header->tzh_ttisstdcnt) +
                  8 * guint32_from_be(header->tzh_leapcnt) +
                  5 * guint32_from_be(header->tzh_timecnt) +
                  6 * guint32_from_be(header->tzh_typecnt) +
                  guint32_from_be(header->tzh_charcnt));

              typecnt     = guint32_from_be (tz->header->tzh_typecnt);
              tz->timecnt = guint32_from_be (tz->header->tzh_timecnt);
              tz->trans   = (gconstpointer) (tz->header + 1);
              tz->indices = (gconstpointer) (tz->trans + tz->timecnt);
              tz->infos   = (gconstpointer) (tz->indices + tz->timecnt);
              tz->abbrs   = (gconstpointer) (tz->infos + typecnt);
            }
        }

      g_hash_table_insert (time_zones, tz->name, tz);
    }
  g_atomic_int_inc (&tz->ref_count);
  G_UNLOCK (time_zones);

  return tz;
}

/**
 * g_time_zone_new_utc:
 *
 * Creates a #GTimeZone corresponding to UTC.
 *
 * This is equivalent to calling g_time_zone_new() with a value like
 * "Z", "UTC", "+00", etc.
 *
 * You should release the return value by calling g_time_zone_unref()
 * when you are done with it.
 *
 * Returns: the universal timezone
 *
 * Since: 2.26
 **/
GTimeZone *
g_time_zone_new_utc (void)
{
  return g_time_zone_new ("UTC");
}

/**
 * g_time_zone_new_local:
 *
 * Creates a #GTimeZone corresponding to local time.
 *
 * This is equivalent to calling g_time_zone_new() with the value of the
 * <varname>TZ</varname> environment variable (including the possibility
 * of %NULL).  Changes made to <varname>TZ</varname> after the first
 * call to this function may or may not be noticed by future calls.
 *
 * You should release the return value by calling g_time_zone_unref()
 * when you are done with it.
 *
 * Returns: the local timezone
 *
 * Since: 2.26
 **/
GTimeZone *
g_time_zone_new_local (void)
{
  return g_time_zone_new (getenv ("TZ"));
}

/* Internal helpers {{{1 */
inline static const struct ttinfo *
interval_info (GTimeZone *tz,
               gint       interval)
{
  if (interval)
    return tz->infos + tz->indices[interval - 1];

  return tz->infos;
}

inline static gint64
interval_start (GTimeZone *tz,
                gint       interval)
{
  if (interval)
    return gint64_from_be (tz->trans[interval - 1]);

  return G_MININT64;
}

inline static gint64
interval_end (GTimeZone *tz,
              gint       interval)
{
  if (interval < tz->timecnt)
    return gint64_from_be (tz->trans[interval]) - 1;

  return G_MAXINT64;
}

inline static gint32
interval_offset (GTimeZone *tz,
                 gint       interval)
{
  return gint32_from_be (interval_info (tz, interval)->tt_gmtoff);
}

inline static gboolean
interval_isdst (GTimeZone *tz,
                gint       interval)
{
  return interval_info (tz, interval)->tt_isdst;
}

inline static guint8
interval_abbrind (GTimeZone *tz,
                  gint       interval)
{
  return interval_info (tz, interval)->tt_abbrind;
}

inline static gint64
interval_local_start (GTimeZone *tz,
                      gint       interval)
{
  if (interval)
    return interval_start (tz, interval) + interval_offset (tz, interval);

  return G_MININT64;
}

inline static gint64
interval_local_end (GTimeZone *tz,
                    gint       interval)
{
  if (interval < tz->timecnt)
    return interval_end (tz, interval) + interval_offset (tz, interval);

  return G_MAXINT64;
}

static gboolean
interval_valid (GTimeZone *tz,
                gint       interval)
{
  return interval <= tz->timecnt;
}

/* g_time_zone_find_interval() {{{1 */

/**
 * g_time_zone_adjust_time:
 * @tz: a #GTimeZone
 * @type: the #GTimeType of @time
 * @time: a pointer to a number of seconds since January 1, 1970
 *
 * Finds an interval within @tz that corresponds to the given @time,
 * possibly adjusting @time if required to fit into an interval.
 * The meaning of @time depends on @type.
 *
 * This function is similar to g_time_zone_find_interval(), with the
 * difference that it always succeeds (by making the adjustments
 * described below).
 *
 * In any of the cases where g_time_zone_find_interval() succeeds then
 * this function returns the same value, without modifying @time.
 *
 * This function may, however, modify @time in order to deal with
 * non-existent times.  If the non-existent local @time of 02:30 were
 * requested on March 13th 2010 in Toronto then this function would
 * adjust @time to be 03:00 and return the interval containing the
 * adjusted time.
 *
 * Returns: the interval containing @time, never -1
 *
 * Since: 2.26
 **/
gint
g_time_zone_adjust_time (GTimeZone *tz,
                         GTimeType  type,
                         gint64    *time)
{
  gint i;

  if (tz->zoneinfo == NULL)
    return 0;

  /* find the interval containing *time UTC
   * TODO: this could be binary searched (or better) */
  for (i = 0; i < tz->timecnt; i++)
    if (*time <= interval_end (tz, i))
      break;

  g_assert (interval_start (tz, i) <= *time && *time <= interval_end (tz, i));

  if (type != G_TIME_TYPE_UNIVERSAL)
    {
      if (*time < interval_local_start (tz, i))
        /* if time came before the start of this interval... */
        {
          i--;

          /* if it's not in the previous interval... */
          if (*time > interval_local_end (tz, i))
            {
              /* it doesn't exist.  fast-forward it. */
              i++;
              *time = interval_local_start (tz, i);
            }
        }

      else if (*time > interval_local_end (tz, i))
        /* if time came after the end of this interval... */
        {
          i++;

          /* if it's not in the next interval... */
          if (*time < interval_local_start (tz, i))
            /* it doesn't exist.  fast-forward it. */
            *time = interval_local_start (tz, i);
        }

      else if (interval_isdst (tz, i) != type)
        /* it's in this interval, but dst flag doesn't match.
         * check neighbours for a better fit. */
        {
          if (i && *time <= interval_local_end (tz, i - 1))
            i--;

          else if (i < tz->timecnt &&
                   *time >= interval_local_start (tz, i + 1))
            i++;
        }
    }

  return i;
}

/**
 * g_time_zone_find_interval:
 * @tz: a #GTimeZone
 * @type: the #GTimeType of @time
 * @time: a number of seconds since January 1, 1970
 *
 * Finds an the interval within @tz that corresponds to the given @time.
 * The meaning of @time depends on @type.
 *
 * If @type is %G_TIME_TYPE_UNIVERSAL then this function will always
 * succeed (since universal time is monotonic and continuous).
 *
 * Otherwise @time is treated is local time.  The distinction between
 * %G_TIME_TYPE_STANDARD and %G_TIME_TYPE_DAYLIGHT is ignored except in
 * the case that the given @time is ambiguous.  In Toronto, for example,
 * 01:30 on November 7th 2010 occured twice (once inside of daylight
 * savings time and the next, an hour later, outside of daylight savings
 * time).  In this case, the different value of @type would result in a
 * different interval being returned.
 *
 * It is still possible for this function to fail.  In Toronto, for
 * example, 02:00 on March 14th 2010 does not exist (due to the leap
 * forward to begin daylight savings time).  -1 is returned in that
 * case.
 *
 * Returns: the interval containing @time, or -1 in case of failure
 *
 * Since: 2.26
 */
gint
g_time_zone_find_interval (GTimeZone *tz,
                           GTimeType  type,
                           gint64     time)
{
  gint i;

  if (tz->zoneinfo == NULL)
    return 0;

  for (i = 0; i < tz->timecnt; i++)
    if (time <= interval_end (tz, i))
      break;

  if (type == G_TIME_TYPE_UNIVERSAL)
    return i;

  if (time < interval_local_start (tz, i))
    {
      if (time > interval_local_end (tz, --i))
        return -1;
    }

  else if (time > interval_local_end (tz, i))
    {
      if (time < interval_local_start (tz, ++i))
        return -1;
    }

  else if (interval_isdst (tz, i) != type)
    {
      if (i && time <= interval_local_end (tz, i - 1))
        i--;

      else if (i < tz->timecnt && time >= interval_local_start (tz, i + 1))
        i++;
    }

  return i;
}

/* Public API accessors {{{1 */

/**
 * g_time_zone_get_abbreviation:
 * @tz: a #GTimeZone
 * @interval: an interval within the timezone
 *
 * Determines the time zone abbreviation to be used during a particular
 * @interval of time in the time zone @tz.
 *
 * For example, in Toronto this is currently "EST" during the winter
 * months and "EDT" during the summer months when daylight savings time
 * is in effect.
 *
 * Returns: the time zone abbreviation, which belongs to @tz
 *
 * Since: 2.26
 **/
const gchar *
g_time_zone_get_abbreviation (GTimeZone *tz,
                              gint       interval)
{
  g_return_val_if_fail (interval_valid (tz, interval), NULL);

  if (tz->header == NULL)
    return "UTC";

  return tz->abbrs + interval_abbrind (tz, interval);
}

/**
 * g_time_zone_get_offset:
 * @tz: a #GTimeZone
 * @interval: an interval within the timezone
 *
 * Determines the offset to UTC in effect during a particular @interval
 * of time in the time zone @tz.
 *
 * The offset is the number of seconds that you add to UTC time to
 * arrive at local time for @tz (ie: negative numbers for time zones
 * west of GMT, positive numbers for east).
 *
 * Returns: the number of seconds that should be added to UTC to get the
 *          local time in @tz
 *
 * Since: 2.26
 **/
gint32
g_time_zone_get_offset (GTimeZone *tz,
                        gint       interval)
{
  g_return_val_if_fail (interval_valid (tz, interval), 0);

  if (tz->header == NULL)
    return 0;

  return interval_offset (tz, interval);
}

/**
 * g_time_zone_is_dst:
 * @tz: a #GTimeZone
 * @interval: an interval within the timezone
 *
 * Determines if daylight savings time is in effect during a particular
 * @interval of time in the time zone @tz.
 *
 * Returns: %TRUE if daylight savings time is in effect
 *
 * Since: 2.26
 **/
gboolean
g_time_zone_is_dst (GTimeZone *tz,
                    gint       interval)
{
  g_return_val_if_fail (interval_valid (tz, interval), FALSE);

  if (tz->header == NULL)
    return FALSE;

  return interval_isdst (tz, interval);
}

/* Epilogue {{{1 */
/* vim:set foldmethod=marker: */
