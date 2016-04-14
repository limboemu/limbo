#ifndef __G_TIME_ZONE_PRIVATE_H__
#define __G_TIME_ZONE_PRIVATE_H__

#include "gtimezone.h"

/*< internal >
 * GTimeType:
 * @G_TIME_TYPE_STANDARD: the time is in local standard time
 * @G_TIME_TYPE_DAYLIGHT: the time is in local daylight time
 * @G_TIME_TYPE_UNIVERSAL: the time is in UTC
 *
 * Disambiguates a given time in two ways.
 *
 * First, specifies if the given time is in universal or local time.
 *
 * Second, if the time is in local time, specifies if it is local
 * standard time or local daylight time.  This is important for the case
 * where the same local time occurs twice (during daylight savings time
 * transitions, for example).
 */
typedef enum
{
  G_TIME_TYPE_STANDARD,
  G_TIME_TYPE_DAYLIGHT,
  G_TIME_TYPE_UNIVERSAL
} GTimeType;

G_GNUC_INTERNAL
gint                    g_time_zone_find_interval                       (GTimeZone   *tz,
                                                                         GTimeType    type,
                                                                         gint64       time);

G_GNUC_INTERNAL
gint                    g_time_zone_adjust_time                         (GTimeZone   *tz,
                                                                         GTimeType    type,
                                                                         gint64      *time);

G_GNUC_INTERNAL
const gchar *           g_time_zone_get_abbreviation                    (GTimeZone   *tz,
                                                                         gint         interval);
G_GNUC_INTERNAL
gint32                  g_time_zone_get_offset                          (GTimeZone   *tz,
                                                                         gint         interval);
G_GNUC_INTERNAL
gboolean                g_time_zone_is_dst                              (GTimeZone   *tz,
                                                                         gint         interval);

#endif /* __G_TIME_ZONE_PRIVATE_H__ */
