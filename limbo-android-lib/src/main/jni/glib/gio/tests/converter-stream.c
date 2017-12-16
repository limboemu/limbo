/* GLib testing framework examples and tests
 * Copyright (C) 2009 Red Hat, Inc.
 * Authors: Alexander Larsson <alexl@redhat.com>
 *
 * This work is provided "as is"; redistribution and modification
 * in whole or in part, in any medium, physical or electronic is
 * permitted without restriction.
 *
 * This work is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * In no event shall the authors or contributors be liable for any
 * direct, indirect, incidental, special, exemplary, or consequential
 * damages (including, but not limited to, procurement of substitute
 * goods or services; loss of use, data, or profits; or business
 * interruption) however caused and on any theory of liability, whether
 * in contract, strict liability, or tort (including negligence or
 * otherwise) arising in any way out of the use of this software, even
 * if advised of the possibility of such damage.
 */

#include <glib/glib.h>
#include <gio/gio.h>
#include <stdlib.h>
#include <string.h>

#define G_TYPE_EXPANDER_CONVERTER         (g_expander_converter_get_type ())
#define G_EXPANDER_CONVERTER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), G_TYPE_EXPANDER_CONVERTER, GExpanderConverter))
#define G_EXPANDER_CONVERTER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), G_TYPE_EXPANDER_CONVERTER, GExpanderConverterClass))
#define G_IS_EXPANDER_CONVERTER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), G_TYPE_EXPANDER_CONVERTER))
#define G_IS_EXPANDER_CONVERTER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), G_TYPE_EXPANDER_CONVERTER))
#define G_EXPANDER_CONVERTER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), G_TYPE_EXPANDER_CONVERTER, GExpanderConverterClass))

typedef struct _GExpanderConverter       GExpanderConverter;
typedef struct _GExpanderConverterClass  GExpanderConverterClass;

struct _GExpanderConverterClass
{
  GObjectClass parent_class;
};

GType       g_expander_converter_get_type (void) G_GNUC_CONST;
GConverter *g_expander_converter_new      (void);



static void g_expander_converter_iface_init          (GConverterIface *iface);

struct _GExpanderConverter
{
  GObject parent_instance;
};

G_DEFINE_TYPE_WITH_CODE (GExpanderConverter, g_expander_converter, G_TYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (G_TYPE_CONVERTER,
						g_expander_converter_iface_init))

static void
g_expander_converter_class_init (GExpanderConverterClass *klass)
{
}

static void
g_expander_converter_init (GExpanderConverter *local)
{
}

GConverter *
g_expander_converter_new (void)
{
  GConverter *conv;

  conv = g_object_new (G_TYPE_EXPANDER_CONVERTER, NULL);

  return conv;
}

static void
g_expander_converter_reset (GConverter *converter)
{
}

static GConverterResult
g_expander_converter_convert (GConverter *converter,
			      const void *inbuf,
			      gsize       inbuf_size,
			      void       *outbuf,
			      gsize       outbuf_size,
			      GConverterFlags flags,
			      gsize      *bytes_read,
			      gsize      *bytes_written,
			      GError    **error)
{
  GExpanderConverter  *conv;
  const guint8 *in, *in_end;
  guint8 v, *out;
  int i;
  gsize block_size;

  conv = G_EXPANDER_CONVERTER (converter);

  in = inbuf;
  out = outbuf;
  in_end = in + inbuf_size;

  while (in < in_end)
    {
      v = *in;

      if (v == 0)
	block_size = 10;
      else
	block_size = v * 1000;

      if (outbuf_size < block_size)
	{
	  if (*bytes_read > 0)
	    return G_CONVERTER_CONVERTED;

	  g_set_error_literal (error, G_IO_ERROR,
			       G_IO_ERROR_NO_SPACE,
			       "No space in dest");
	  return G_CONVERTER_ERROR;
	}

      in++;
      *bytes_read += 1;
      *bytes_written += block_size;
      outbuf_size -= block_size;
      for (i = 0; i < block_size; i++)
	*out++ = v;
    }

  if (in == in_end && (flags & G_CONVERTER_INPUT_AT_END))
    return G_CONVERTER_FINISHED;
  return G_CONVERTER_CONVERTED;
}

static void
g_expander_converter_iface_init (GConverterIface *iface)
{
  iface->convert = g_expander_converter_convert;
  iface->reset = g_expander_converter_reset;
}

#define G_TYPE_COMPRESSOR_CONVERTER         (g_compressor_converter_get_type ())
#define G_COMPRESSOR_CONVERTER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), G_TYPE_COMPRESSOR_CONVERTER, GCompressorConverter))
#define G_COMPRESSOR_CONVERTER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), G_TYPE_COMPRESSOR_CONVERTER, GCompressorConverterClass))
#define G_IS_COMPRESSOR_CONVERTER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), G_TYPE_COMPRESSOR_CONVERTER))
#define G_IS_COMPRESSOR_CONVERTER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), G_TYPE_COMPRESSOR_CONVERTER))
#define G_COMPRESSOR_CONVERTER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), G_TYPE_COMPRESSOR_CONVERTER, GCompressorConverterClass))

typedef struct _GCompressorConverter       GCompressorConverter;
typedef struct _GCompressorConverterClass  GCompressorConverterClass;

struct _GCompressorConverterClass
{
  GObjectClass parent_class;
};

GType       g_compressor_converter_get_type (void) G_GNUC_CONST;
GConverter *g_compressor_converter_new      (void);



static void g_compressor_converter_iface_init          (GConverterIface *iface);

struct _GCompressorConverter
{
  GObject parent_instance;
};

G_DEFINE_TYPE_WITH_CODE (GCompressorConverter, g_compressor_converter, G_TYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (G_TYPE_CONVERTER,
						g_compressor_converter_iface_init))

static void
g_compressor_converter_class_init (GCompressorConverterClass *klass)
{
}

static void
g_compressor_converter_init (GCompressorConverter *local)
{
}

GConverter *
g_compressor_converter_new (void)
{
  GConverter *conv;

  conv = g_object_new (G_TYPE_COMPRESSOR_CONVERTER, NULL);

  return conv;
}

static void
g_compressor_converter_reset (GConverter *converter)
{
}

static GConverterResult
g_compressor_converter_convert (GConverter *converter,
				const void *inbuf,
				gsize       inbuf_size,
				void       *outbuf,
				gsize       outbuf_size,
				GConverterFlags flags,
				gsize      *bytes_read,
				gsize      *bytes_written,
				GError    **error)
{
  GCompressorConverter  *conv;
  const guint8 *in, *in_end;
  guint8 v, *out;
  int i;
  gsize block_size;

  conv = G_COMPRESSOR_CONVERTER (converter);

  in = inbuf;
  out = outbuf;
  in_end = in + inbuf_size;

  while (in < in_end)
    {
      v = *in;

      if (v == 0)
	{
	  block_size = 0;
	  while (in+block_size < in_end && *(in+block_size) == 0)
	    block_size ++;
	}
      else
	block_size = v * 1000;

      /* Not enough data */
      if (in_end - in < block_size)
	{
	  if (*bytes_read > 0)
	    break;
	  g_set_error_literal (error, G_IO_ERROR,
			       G_IO_ERROR_PARTIAL_INPUT,
			       "Need more data");
	  return G_CONVERTER_ERROR;
	}

      for (i = 0; i < block_size; i++)
	{
	  if (*(in + i) != v)
	    {
	      if (*bytes_read > 0)
		break;
	      g_set_error_literal (error, G_IO_ERROR,
				   G_IO_ERROR_INVALID_DATA,
				   "invalid data");
	      return G_CONVERTER_ERROR;
	    }
	}

      if (v == 0 && in_end - in == block_size && (flags & G_CONVERTER_INPUT_AT_END) == 0)
	{
	  if (*bytes_read > 0)
	    break;
	  g_set_error_literal (error, G_IO_ERROR,
			       G_IO_ERROR_PARTIAL_INPUT,
			       "Need more data");
	  return G_CONVERTER_ERROR;
	}

      in += block_size;
      *out++ = v;
      *bytes_read += block_size;
      *bytes_written += 1;
    }

  if (in == in_end && (flags & G_CONVERTER_INPUT_AT_END))
    return G_CONVERTER_FINISHED;
  return G_CONVERTER_CONVERTED;
}

static void
g_compressor_converter_iface_init (GConverterIface *iface)
{
  iface->convert = g_compressor_converter_convert;
  iface->reset = g_compressor_converter_reset;
}

guint8 unexpanded_data[] = { 0,1,3,4,5,6,7,3,12,0,0};

static void
test_expander (void)
{
  guint8 *converted1, *converted2, *ptr;
  gsize n_read, n_written;
  gsize total_read;
  gssize res;
  GConverterResult cres;
  GInputStream *mem, *cstream;
  GOutputStream *mem_out, *cstream_out;
  GConverter *expander;
  GConverter *converter;
  GError *error;
  int i;

  expander = g_expander_converter_new ();

  converted1 = g_malloc (100*1000); /* Large enough */
  converted2 = g_malloc (100*1000); /* Large enough */

  cres = g_converter_convert (expander,
			      unexpanded_data, sizeof(unexpanded_data),
			      converted1, 100*1000,
			      G_CONVERTER_INPUT_AT_END,
			      &n_read, &n_written, NULL);

  g_assert (cres == G_CONVERTER_FINISHED);
  g_assert (n_read == 11);
  g_assert (n_written == 41030);

  g_converter_reset (expander);

  mem = g_memory_input_stream_new_from_data (unexpanded_data,
					     sizeof (unexpanded_data),
					     NULL);
  cstream = g_converter_input_stream_new (mem, expander);
  g_assert (g_converter_input_stream_get_converter (G_CONVERTER_INPUT_STREAM (cstream)) == expander);
  g_object_get (cstream, "converter", &converter, NULL);
  g_assert (converter == expander);
  g_object_unref (converter);
  g_object_unref (mem);

  total_read = 0;
  ptr = converted2;
  while (TRUE)
    {
      error = NULL;
      res = g_input_stream_read (cstream,
				 ptr, 1,
				 NULL, &error);
      g_assert (res != -1);
      if (res == 0)
	break;
      ptr += res;
      total_read += res;
    }

  g_assert (total_read == n_written);
  g_assert (memcmp (converted1, converted2, n_written)  == 0);

  g_converter_reset (expander);

  mem_out = g_memory_output_stream_new (NULL, 0, g_realloc, g_free);
  cstream_out = g_converter_output_stream_new (mem_out, expander);
  g_assert (g_converter_output_stream_get_converter (G_CONVERTER_OUTPUT_STREAM (cstream_out)) == expander);
  g_object_get (cstream_out, "converter", &converter, NULL);
  g_assert (converter == expander);
  g_object_unref (converter);
  g_object_unref (mem_out);

  for (i = 0; i < sizeof(unexpanded_data); i++)
    {
      error = NULL;
      res = g_output_stream_write (cstream_out,
				   unexpanded_data + i, 1,
				   NULL, &error);
      g_assert (res != -1);
      if (res == 0)
	{
	  g_assert (i == sizeof(unexpanded_data) -1);
	  break;
	}
      g_assert (res == 1);
    }

  g_output_stream_close (cstream_out, NULL, NULL);

  g_assert (g_memory_output_stream_get_data_size (G_MEMORY_OUTPUT_STREAM (mem_out)) == n_written);
  g_assert (memcmp (g_memory_output_stream_get_data (G_MEMORY_OUTPUT_STREAM (mem_out)),
		    converted1,
		    n_written)  == 0);

  g_free (converted1);
  g_free (converted2);
  g_object_unref (cstream);
  g_object_unref (cstream_out);
  g_object_unref (expander);
}

static void
test_compressor (void)
{
  guint8 *converted, *expanded, *ptr;
  gsize n_read, expanded_size;
  gsize total_read;
  gssize res;
  GConverterResult cres;
  GInputStream *mem, *cstream;
  GOutputStream *mem_out, *cstream_out;
  GConverter *expander, *compressor;
  GError *error;
  int i;

  expander = g_expander_converter_new ();
  expanded = g_malloc (100*1000); /* Large enough */
  cres = g_converter_convert (expander,
			      unexpanded_data, sizeof(unexpanded_data),
			      expanded, 100*1000,
			      G_CONVERTER_INPUT_AT_END,
			      &n_read, &expanded_size, NULL);
  g_assert (cres == G_CONVERTER_FINISHED);
  g_assert (n_read == 11);
  g_assert (expanded_size == 41030);

  compressor = g_compressor_converter_new ();

  converted = g_malloc (100*1000); /* Large enough */

  mem = g_memory_input_stream_new_from_data (expanded,
					     expanded_size,
					     NULL);
  cstream = g_converter_input_stream_new (mem, compressor);
  g_object_unref (mem);

  total_read = 0;
  ptr = converted;
  while (TRUE)
    {
      error = NULL;
      res = g_input_stream_read (cstream,
				 ptr, 1,
				 NULL, &error);
      g_assert (res != -1);
      if (res == 0)
	break;
      ptr += res;
      total_read += res;
    }

  g_assert (total_read == n_read - 1); /* Last 2 zeros are combined */
  g_assert (memcmp (converted, unexpanded_data, total_read)  == 0);

  g_object_unref (cstream);

  g_converter_reset (compressor);

  mem_out = g_memory_output_stream_new (NULL, 0, g_realloc, g_free);
  cstream_out = g_converter_output_stream_new (mem_out, compressor);
  g_object_unref (mem_out);

  for (i = 0; i < expanded_size; i++)
    {
      error = NULL;
      res = g_output_stream_write (cstream_out,
				   expanded + i, 1,
				   NULL, &error);
      g_assert (res != -1);
      if (res == 0)
	{
	  g_assert (i == expanded_size -1);
	  break;
	}
      g_assert (res == 1);
    }

  g_output_stream_close (cstream_out, NULL, NULL);

  g_assert (g_memory_output_stream_get_data_size (G_MEMORY_OUTPUT_STREAM (mem_out)) == n_read - 1); /* Last 2 zeros are combined */
  g_assert (memcmp (g_memory_output_stream_get_data (G_MEMORY_OUTPUT_STREAM (mem_out)),
		    unexpanded_data,
		    g_memory_output_stream_get_data_size (G_MEMORY_OUTPUT_STREAM (mem_out)))  == 0);

  g_object_unref (cstream_out);

  g_converter_reset (compressor);

  memset (expanded, 5, 5*1000*2);

  mem = g_memory_input_stream_new_from_data (expanded,
					     5*1000,
					     NULL);
  cstream = g_converter_input_stream_new (mem, compressor);
  g_object_unref (mem);

  total_read = 0;
  ptr = converted;
  while (TRUE)
    {
      error = NULL;
      res = g_input_stream_read (cstream,
				 ptr, 1,
				 NULL, &error);
      g_assert (res != -1);
      if (res == 0)
	break;
      ptr += res;
      total_read += res;
    }

  g_assert (total_read == 1);
  g_assert (*converted == 5);

  g_object_unref (cstream);

  mem = g_memory_input_stream_new_from_data (expanded,
					     5*1000 * 2,
					     NULL);
  cstream = g_converter_input_stream_new (mem, compressor);
  g_object_unref (mem);

  total_read = 0;
  ptr = converted;
  while (TRUE)
    {
      error = NULL;
      res = g_input_stream_read (cstream,
				 ptr, 1,
				 NULL, &error);
      g_assert (res != -1);
      if (res == 0)
	break;
      ptr += res;
      total_read += res;
    }

  g_assert (total_read == 2);
  g_assert (converted[0] == 5);
  g_assert (converted[1] == 5);

  g_object_unref (cstream);

  g_converter_reset (compressor);

  mem = g_memory_input_stream_new_from_data (expanded,
					     5*1000 * 2 - 1,
					     NULL);
  cstream = g_converter_input_stream_new (mem, compressor);
  g_object_unref (mem);

  total_read = 0;
  ptr = converted;
  while (TRUE)
    {
      error = NULL;
      res = g_input_stream_read (cstream,
				 ptr, 1,
				 NULL, &error);
      if (res == -1)
	{
	  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_PARTIAL_INPUT);
          g_error_free (error);
	  break;
	}

      g_assert (res != 0);
      ptr += res;
      total_read += res;
    }

  g_assert (total_read == 1);
  g_assert (converted[0] == 5);

  g_object_unref (cstream);

  g_free (expanded);
  g_free (converted);
  g_object_unref (expander);
  g_object_unref (compressor);
}

#define DATA_LENGTH 1000000

static void
test_corruption (GZlibCompressorFormat format, gint level)
{
  GError *error = NULL;
  guint32 *data0, *data1;
  gsize data1_size;
  gint i;
  GInputStream *istream0, *istream1, *cistream1;
  GOutputStream *ostream1, *ostream2, *costream1;
  GConverter *compressor, *decompressor;
  GZlibCompressorFormat fmt;
  gint lvl;

  data0 = g_malloc (DATA_LENGTH * sizeof (guint32));
  for (i = 0; i < DATA_LENGTH; i++)
    data0[i] = g_random_int ();

  istream0 = g_memory_input_stream_new_from_data (data0,
    DATA_LENGTH * sizeof (guint32), NULL);

  ostream1 = g_memory_output_stream_new (NULL, 0, g_realloc, g_free);
  compressor = G_CONVERTER (g_zlib_compressor_new (format, level));
  costream1 = g_converter_output_stream_new (ostream1, compressor);
  g_assert (g_converter_output_stream_get_converter (G_CONVERTER_OUTPUT_STREAM (costream1)) == compressor);

  g_output_stream_splice (costream1, istream0, 0, NULL, &error);
  g_assert_no_error (error);

  g_object_unref (costream1);

  g_converter_reset (compressor);
  g_object_get (compressor, "format", &fmt, "level", &lvl, NULL);
  g_assert_cmpint (fmt, ==, format);
  g_assert_cmpint (lvl, ==, level);
  g_object_unref (compressor);
  data1 = g_memory_output_stream_steal_data (G_MEMORY_OUTPUT_STREAM (ostream1));
  data1_size = g_memory_output_stream_get_data_size (
    G_MEMORY_OUTPUT_STREAM (ostream1));
  g_object_unref (ostream1);
  g_object_unref (istream0);

  istream1 = g_memory_input_stream_new_from_data (data1, data1_size, g_free);
  decompressor = G_CONVERTER (g_zlib_decompressor_new (format));
  cistream1 = g_converter_input_stream_new (istream1, decompressor);

  ostream2 = g_memory_output_stream_new (NULL, 0, g_realloc, g_free);

  g_output_stream_splice (ostream2, cistream1, 0, NULL, &error);
  g_assert_no_error (error);

  g_assert_cmpuint (DATA_LENGTH * sizeof (guint32), ==,
    g_memory_output_stream_get_data_size (G_MEMORY_OUTPUT_STREAM (ostream2)));
  g_assert (memcmp (data0, g_memory_output_stream_get_data (
    G_MEMORY_OUTPUT_STREAM (ostream2)), DATA_LENGTH * sizeof (guint32)) == 0);
  g_object_unref (istream1);
  g_converter_reset (decompressor);
  g_object_get (decompressor, "format", &fmt, NULL);
  g_assert_cmpint (fmt, ==, format);
  g_object_unref (decompressor);
  g_object_unref (cistream1);
  g_object_unref (ostream2);
  g_free (data0);
}

typedef struct {
  const gchar *path;
  GZlibCompressorFormat format;
  gint level;
} CompressorTest;

static void
test_roundtrip (gconstpointer data)
{
  const CompressorTest *test = data;

  g_test_bug ("162549");

  test_corruption (test->format, test->level);
}

typedef struct {
  const gchar *path;
  const gchar *charset_in;
  const gchar *text_in;
  const gchar *charset_out;
  const gchar *text_out;
  gint n_fallbacks;
} CharsetTest;

static void
test_charset (gconstpointer data)
{
  const CharsetTest *test = data;
  GInputStream *in, *in2;
  GConverter *conv;
  gchar *buffer;
  gsize count;
  gsize bytes_read;
  GError *error;
  gboolean fallback;

  conv = (GConverter *)g_charset_converter_new (test->charset_out, test->charset_in, NULL);
  g_object_get (conv, "use-fallback", &fallback, NULL);
  g_assert (!fallback);

  in = g_memory_input_stream_new_from_data (test->text_in, -1, NULL);
  in2 = g_converter_input_stream_new (in, conv);

  count = 2 * strlen (test->text_out);
  buffer = g_malloc0 (count);
  error = NULL;
  g_input_stream_read_all (in2, buffer, count, &bytes_read, NULL, &error);
  if (test->n_fallbacks == 0)
    {
      g_assert_no_error (error);
      g_assert_cmpint (bytes_read, ==, strlen (test->text_out));
      g_assert_cmpstr (buffer, ==, test->text_out);
    }
  else
    {
      g_assert_error (error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA);
      g_error_free (error);
    }

  g_free (buffer);
  g_object_unref (in2);
  g_object_unref (in);

  if (test->n_fallbacks == 0)
    {
       g_object_unref (conv);
       return;
    }

  g_converter_reset (conv);

  g_assert (!g_charset_converter_get_use_fallback (G_CHARSET_CONVERTER (conv)));
  g_charset_converter_set_use_fallback (G_CHARSET_CONVERTER (conv), TRUE);

  in = g_memory_input_stream_new_from_data (test->text_in, -1, NULL);
  in2 = g_converter_input_stream_new (in, conv);

  count = 2 * strlen (test->text_out);
  buffer = g_malloc0 (count);
  error = NULL;
  g_input_stream_read_all (in2, buffer, count, &bytes_read, NULL, &error);
  g_assert_no_error (error);
  g_assert_cmpstr (buffer, ==, test->text_out);
  g_assert_cmpint (bytes_read, ==, strlen (test->text_out));
  g_assert_cmpint (test->n_fallbacks, ==, g_charset_converter_get_num_fallbacks (G_CHARSET_CONVERTER (conv)));

  g_free (buffer);
  g_object_unref (in2);
  g_object_unref (in);

  g_object_unref (conv);
}

int
main (int   argc,
      char *argv[])
{
  CompressorTest compressor_tests[] = {
    { "/converter-output-stream/corruption/zlib-0", G_ZLIB_COMPRESSOR_FORMAT_ZLIB, 0 },
    { "/converter-output-stream/corruption/zlib-9", G_ZLIB_COMPRESSOR_FORMAT_ZLIB, 9 },
    { "/converter-output-stream/corruption/gzip-0", G_ZLIB_COMPRESSOR_FORMAT_GZIP, 0 },
    { "/converter-output-stream/corruption/gzip-9", G_ZLIB_COMPRESSOR_FORMAT_GZIP, 9 },
    { "/converter-output-stream/corruption/raw-0", G_ZLIB_COMPRESSOR_FORMAT_RAW, 0 },
    { "/converter-output-stream/corruption/raw-9", G_ZLIB_COMPRESSOR_FORMAT_RAW, 9 },
  };
  CharsetTest charset_tests[] = {
    { "/converter-input-stream/charset/utf8->latin1", "UTF-8", "\303\205rr Sant\303\251", "ISO-8859-1", "\305rr Sant\351", 0 },
    { "/converter-input-stream/charset/latin1->utf8", "ISO-8859-1", "\305rr Sant\351", "UTF-8", "\303\205rr Sant\303\251", 0 },
    { "/converter-input-stream/charset/fallbacks", "UTF-8", "Some characters just don't fit into latin1: πא", "ISO-8859-1", "Some characters just don't fit into latin1: \\CF\\80\\D7\\90", 4 },

  };

  gint i;

  g_type_init ();
  g_test_init (&argc, &argv, NULL);

  g_test_bug_base ("http://bugzilla.gnome.org/");

  g_test_add_func ("/converter-input-stream/expander", test_expander);
  g_test_add_func ("/converter-input-stream/compressor", test_compressor);

  for (i = 0; i < G_N_ELEMENTS (compressor_tests); i++)
    g_test_add_data_func (compressor_tests[i].path, &compressor_tests[i], test_roundtrip);

  for (i = 0; i < G_N_ELEMENTS (charset_tests); i++)
    g_test_add_data_func (charset_tests[i].path, &charset_tests[i], test_charset);


  return g_test_run();
}
