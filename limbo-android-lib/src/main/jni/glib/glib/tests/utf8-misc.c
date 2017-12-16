/* Unit tests for utilities
 * Copyright (C) 2010 Red Hat, Inc.
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
 *
 * Author: Matthias Clasen
 */

#include "glib.h"

static void
test_utf8_strlen (void)
{
  const gchar *string = "\xe2\x82\xa0gh\xe2\x82\xa4jl";

  g_assert_cmpint (g_utf8_strlen (string, -1), ==, 6);
  g_assert_cmpint (g_utf8_strlen (string, 0), ==, 0);
  g_assert_cmpint (g_utf8_strlen (string, 1), ==, 0);
  g_assert_cmpint (g_utf8_strlen (string, 2), ==, 0);
  g_assert_cmpint (g_utf8_strlen (string, 3), ==, 1);
  g_assert_cmpint (g_utf8_strlen (string, 4), ==, 2);
  g_assert_cmpint (g_utf8_strlen (string, 5), ==, 3);
  g_assert_cmpint (g_utf8_strlen (string, 6), ==, 3);
  g_assert_cmpint (g_utf8_strlen (string, 7), ==, 3);
  g_assert_cmpint (g_utf8_strlen (string, 8), ==, 4);
  g_assert_cmpint (g_utf8_strlen (string, 9), ==, 5);
  g_assert_cmpint (g_utf8_strlen (string, 10), ==, 6);
}

static void
test_utf8_strncpy (void)
{
  const gchar *string = "\xe2\x82\xa0gh\xe2\x82\xa4jl";
  gchar dest[20];

  g_utf8_strncpy (dest, string, 0);
  g_assert_cmpstr (dest, ==, "");

  g_utf8_strncpy (dest, string, 1);
  g_assert_cmpstr (dest, ==, "\xe2\x82\xa0");

  g_utf8_strncpy (dest, string, 2);
  g_assert_cmpstr (dest, ==, "\xe2\x82\xa0g");

  g_utf8_strncpy (dest, string, 3);
  g_assert_cmpstr (dest, ==, "\xe2\x82\xa0gh");

  g_utf8_strncpy (dest, string, 4);
  g_assert_cmpstr (dest, ==, "\xe2\x82\xa0gh\xe2\x82\xa4");

  g_utf8_strncpy (dest, string, 5);
  g_assert_cmpstr (dest, ==, "\xe2\x82\xa0gh\xe2\x82\xa4j");

  g_utf8_strncpy (dest, string, 6);
  g_assert_cmpstr (dest, ==, "\xe2\x82\xa0gh\xe2\x82\xa4jl");

  g_utf8_strncpy (dest, string, 20);
  g_assert_cmpstr (dest, ==, "\xe2\x82\xa0gh\xe2\x82\xa4jl");
}

static void
test_utf8_strrchr (void)
{
  const gchar *string = "\xe2\x82\xa0gh\xe2\x82\xa4jl\xe2\x82\xa4jl";

  g_assert (g_utf8_strrchr (string, -1, 'j') == string + 13);
  g_assert (g_utf8_strrchr (string, -1, 8356) == string + 10);
  g_assert (g_utf8_strrchr (string, 9, 8356) == string + 5);
  g_assert (g_utf8_strrchr (string, 3, 'j') == NULL);
  g_assert (g_utf8_strrchr (string, -1, 'x') == NULL);
}

static void
test_utf8_reverse (void)
{
  gchar *r;

  r = g_utf8_strreverse ("abcdef", -1);
  g_assert_cmpstr (r, ==, "fedcba");
  g_free (r);

  r = g_utf8_strreverse ("abcdef", 4);
  g_assert_cmpstr (r, ==, "dcba");
  g_free (r);

  /* U+0B0B Oriya Letter Vocalic R
   * U+10900 Phoenician Letter Alf
   * U+0041 Latin Capital Letter A
   * U+1EB6 Latin Capital Letter A With Breve And Dot Below
   */
  r = g_utf8_strreverse ("\340\254\213\360\220\244\200\101\341\272\266", -1);
  g_assert_cmpstr (r, ==, "\341\272\266\101\360\220\244\200\340\254\213");
  g_free (r);


}

static void
test_unichar_validate (void)
{
  g_assert (g_unichar_validate ('j'));
  g_assert (g_unichar_validate (8356));
  g_assert (g_unichar_validate (8356));
  g_assert (!g_unichar_validate (0xfdd1));
  g_assert (g_unichar_validate (917760));
  g_assert (!g_unichar_validate (0x110000));
}

static void
test_unichar_character_type (void)
{
  gint i;
  struct {
    GUnicodeType type;
    gunichar     c;
  } examples[] = {
    { G_UNICODE_CONTROL,              0x000D },
    { G_UNICODE_FORMAT,               0x200E },
     /* G_UNICODE_UNASSIGNED */
    { G_UNICODE_PRIVATE_USE,          0xE000 },
    { G_UNICODE_SURROGATE,            0xD800 },
    { G_UNICODE_LOWERCASE_LETTER,     0x0061 },
    { G_UNICODE_MODIFIER_LETTER,      0x02B0 },
    { G_UNICODE_OTHER_LETTER,         0x3400 },
    { G_UNICODE_TITLECASE_LETTER,     0x01C5 },
    { G_UNICODE_UPPERCASE_LETTER,     0xFF21 },
    { G_UNICODE_COMBINING_MARK,       0x0903 },
    { G_UNICODE_ENCLOSING_MARK,       0x20DD },
    { G_UNICODE_NON_SPACING_MARK,     0xA806 },
    { G_UNICODE_DECIMAL_NUMBER,       0xFF10 },
    { G_UNICODE_LETTER_NUMBER,        0x16EE },
    { G_UNICODE_OTHER_NUMBER,         0x17F0 },
    { G_UNICODE_CONNECT_PUNCTUATION,  0x005F },
    { G_UNICODE_DASH_PUNCTUATION,     0x058A },
    { G_UNICODE_CLOSE_PUNCTUATION,    0x0F3B },
    { G_UNICODE_FINAL_PUNCTUATION,    0x2019 },
    { G_UNICODE_INITIAL_PUNCTUATION,  0x2018 },
    { G_UNICODE_OTHER_PUNCTUATION,    0x2016 },
    { G_UNICODE_OPEN_PUNCTUATION,     0x0F3A },
    { G_UNICODE_CURRENCY_SYMBOL,      0x20A0 },
    { G_UNICODE_MODIFIER_SYMBOL,      0x309B },
    { G_UNICODE_MATH_SYMBOL,          0xFB29 },
    { G_UNICODE_OTHER_SYMBOL,         0x00A6 },
    { G_UNICODE_LINE_SEPARATOR,       0x2028 },
    { G_UNICODE_PARAGRAPH_SEPARATOR,  0x2029 },
    { G_UNICODE_SPACE_SEPARATOR,      0x202F },
  };

  for (i = 0; i < G_N_ELEMENTS (examples); i++)
    {
      g_assert (g_unichar_type (examples[i].c) == examples[i].type);
    }
}

static void
test_unichar_break_type (void)
{
  gint i;
  struct {
    GUnicodeBreakType type;
    gunichar          c;
  } examples[] = {
    { G_UNICODE_BREAK_MANDATORY,           0x2028 },
    { G_UNICODE_BREAK_CARRIAGE_RETURN,     0x000D },
    { G_UNICODE_BREAK_LINE_FEED,           0x000A },
    { G_UNICODE_BREAK_COMBINING_MARK,      0x0300 },
    { G_UNICODE_BREAK_SURROGATE,           0xD800 },
    { G_UNICODE_BREAK_ZERO_WIDTH_SPACE,    0x200B },
    { G_UNICODE_BREAK_INSEPARABLE,         0x2024 },
    { G_UNICODE_BREAK_NON_BREAKING_GLUE,   0x00A0 },
    { G_UNICODE_BREAK_CONTINGENT,          0xFFFC },
    { G_UNICODE_BREAK_SPACE,               0x0020 },
    { G_UNICODE_BREAK_AFTER,               0x05BE },
    { G_UNICODE_BREAK_BEFORE,              0x02C8 },
    { G_UNICODE_BREAK_BEFORE_AND_AFTER,    0x2014 },
    { G_UNICODE_BREAK_HYPHEN,              0x002D },
    { G_UNICODE_BREAK_NON_STARTER,         0x17D6 },
    { G_UNICODE_BREAK_OPEN_PUNCTUATION,    0x0028 },
    { G_UNICODE_BREAK_CLOSE_PUNCTUATION,   0x0029 },
    { G_UNICODE_BREAK_QUOTATION,           0x0022 },
    { G_UNICODE_BREAK_EXCLAMATION,         0x0021 },
    { G_UNICODE_BREAK_IDEOGRAPHIC,         0x2E80 },
    { G_UNICODE_BREAK_NUMERIC,             0x0030 },
    { G_UNICODE_BREAK_INFIX_SEPARATOR,     0x002C },
    { G_UNICODE_BREAK_SYMBOL,              0x002F },
    { G_UNICODE_BREAK_ALPHABETIC,          0x0023 },
    { G_UNICODE_BREAK_PREFIX,              0x0024 },
    { G_UNICODE_BREAK_POSTFIX,             0x0025 },
    { G_UNICODE_BREAK_COMPLEX_CONTEXT,     0x0E01 },
    { G_UNICODE_BREAK_AMBIGUOUS,           0x00F7 },
    { G_UNICODE_BREAK_UNKNOWN,             0xE000 },
    { G_UNICODE_BREAK_NEXT_LINE,           0x0085 },
    { G_UNICODE_BREAK_WORD_JOINER,         0x2060 },
    { G_UNICODE_BREAK_HANGUL_L_JAMO,       0x1100 },
    { G_UNICODE_BREAK_HANGUL_V_JAMO,       0x1160 },
    { G_UNICODE_BREAK_HANGUL_T_JAMO,       0x11A8 },
    { G_UNICODE_BREAK_HANGUL_LV_SYLLABLE,  0xAC00 },
    { G_UNICODE_BREAK_HANGUL_LVT_SYLLABLE, 0xAC01 }
  };

  for (i = 0; i < G_N_ELEMENTS (examples); i++)
    {
      g_assert (g_unichar_break_type (examples[i].c) == examples[i].type);
    }
}

static void
test_unichar_script (void)
{
  gint i;
  struct {
    GUnicodeScript script;
    gunichar          c;
  } examples[] = {
    { G_UNICODE_SCRIPT_COMMON,                  0x002A },
    /* { G_UNICODE_SCRIPT_INHERITED,               0x1CED }, 5.2 addition */
    { G_UNICODE_SCRIPT_INHERITED,               0x0670 },
    { G_UNICODE_SCRIPT_ARABIC,                  0x060D },
    { G_UNICODE_SCRIPT_ARMENIAN,                0x0559 },
    { G_UNICODE_SCRIPT_BENGALI,                 0x09CD },
    { G_UNICODE_SCRIPT_BOPOMOFO,                0x31B6 },
    { G_UNICODE_SCRIPT_CHEROKEE,                0x13A2 },
    { G_UNICODE_SCRIPT_COPTIC,                  0x2CFD },
    { G_UNICODE_SCRIPT_CYRILLIC,                0x0482 },
    { G_UNICODE_SCRIPT_DESERET,                0x10401 },
    { G_UNICODE_SCRIPT_DEVANAGARI,              0x094D },
    { G_UNICODE_SCRIPT_ETHIOPIC,                0x1258 },
    { G_UNICODE_SCRIPT_GEORGIAN,                0x10FC },
    { G_UNICODE_SCRIPT_GOTHIC,                 0x10341 },
    { G_UNICODE_SCRIPT_GREEK,                   0x0375 },
    { G_UNICODE_SCRIPT_GUJARATI,                0x0A83 },
    { G_UNICODE_SCRIPT_GURMUKHI,                0x0A3C },
    { G_UNICODE_SCRIPT_HAN,                     0x3005 },
    { G_UNICODE_SCRIPT_HANGUL,                  0x1100 },
    { G_UNICODE_SCRIPT_HEBREW,                  0x05BF },
    { G_UNICODE_SCRIPT_HIRAGANA,                0x309F },
    { G_UNICODE_SCRIPT_KANNADA,                 0x0CBC },
    { G_UNICODE_SCRIPT_KATAKANA,                0x30FF },
    { G_UNICODE_SCRIPT_KHMER,                   0x17DD },
    { G_UNICODE_SCRIPT_LAO,                     0x0EDD },
    { G_UNICODE_SCRIPT_LATIN,                   0x0061 },
    { G_UNICODE_SCRIPT_MALAYALAM,               0x0D3D },
    { G_UNICODE_SCRIPT_MONGOLIAN,               0x1843 },
    { G_UNICODE_SCRIPT_MYANMAR,                 0x1031 },
    { G_UNICODE_SCRIPT_OGHAM,                   0x169C },
    { G_UNICODE_SCRIPT_OLD_ITALIC,             0x10322 },
    { G_UNICODE_SCRIPT_ORIYA,                   0x0B3C },
    { G_UNICODE_SCRIPT_RUNIC,                   0x16EF },
    { G_UNICODE_SCRIPT_SINHALA,                 0x0DBD },
    { G_UNICODE_SCRIPT_SYRIAC,                  0x0711 },
    { G_UNICODE_SCRIPT_TAMIL,                   0x0B82 },
    { G_UNICODE_SCRIPT_TELUGU,                  0x0C03 },
    { G_UNICODE_SCRIPT_THAANA,                  0x07B1 },
    { G_UNICODE_SCRIPT_THAI,                    0x0E31 },
    { G_UNICODE_SCRIPT_TIBETAN,                 0x0FD4 },
    /* { G_UNICODE_SCRIPT_CANADIAN_ABORIGINAL,     0x1400 }, 5.2 addition */
    { G_UNICODE_SCRIPT_CANADIAN_ABORIGINAL,     0x1401 },
    { G_UNICODE_SCRIPT_YI,                      0xA015 },
    { G_UNICODE_SCRIPT_TAGALOG,                 0x1700 },
    { G_UNICODE_SCRIPT_HANUNOO,                 0x1720 },
    { G_UNICODE_SCRIPT_BUHID,                   0x1740 },
    { G_UNICODE_SCRIPT_TAGBANWA,                0x1760 },
    { G_UNICODE_SCRIPT_BRAILLE,                 0x2800 },
    { G_UNICODE_SCRIPT_CYPRIOT,                0x10808 },
    { G_UNICODE_SCRIPT_LIMBU,                   0x1932 },
    { G_UNICODE_SCRIPT_OSMANYA,                0x10480 },
    { G_UNICODE_SCRIPT_SHAVIAN,                0x10450 },
    { G_UNICODE_SCRIPT_LINEAR_B,               0x10000 },
    { G_UNICODE_SCRIPT_TAI_LE,                  0x1950 },
    { G_UNICODE_SCRIPT_UGARITIC,               0x1039F },
    { G_UNICODE_SCRIPT_NEW_TAI_LUE,             0x1980 },
    { G_UNICODE_SCRIPT_BUGINESE,                0x1A1F },
    { G_UNICODE_SCRIPT_GLAGOLITIC,              0x2C00 },
    { G_UNICODE_SCRIPT_TIFINAGH,                0x2D6F },
    { G_UNICODE_SCRIPT_SYLOTI_NAGRI,            0xA800 },
    { G_UNICODE_SCRIPT_OLD_PERSIAN,            0x103D0 },
    { G_UNICODE_SCRIPT_KHAROSHTHI,             0x10A3F },
    /* G_UNICODE_SCRIPT_UNKNOWN */
    { G_UNICODE_SCRIPT_BALINESE,                0x1B04 },
    { G_UNICODE_SCRIPT_CUNEIFORM,              0x12000 },
    { G_UNICODE_SCRIPT_PHOENICIAN,             0x10900 },
    { G_UNICODE_SCRIPT_PHAGS_PA,                0xA840 },
    { G_UNICODE_SCRIPT_NKO,                     0x07C0 },
    { G_UNICODE_SCRIPT_KAYAH_LI,                0xA900 },
    { G_UNICODE_SCRIPT_LEPCHA,                  0x1C00 },
    { G_UNICODE_SCRIPT_REJANG,                  0xA930 },
    { G_UNICODE_SCRIPT_SUNDANESE,               0x1B80 },
    { G_UNICODE_SCRIPT_SAURASHTRA,              0xA880 },
    { G_UNICODE_SCRIPT_CHAM,                    0xAA00 },
    { G_UNICODE_SCRIPT_OL_CHIKI,                0x1C50 },
    { G_UNICODE_SCRIPT_VAI,                     0xA500 },
    { G_UNICODE_SCRIPT_CARIAN,                 0x102A0 },
    { G_UNICODE_SCRIPT_LYCIAN,                 0x10280 },
    { G_UNICODE_SCRIPT_LYDIAN,                 0x1093F },
/* 5.2 additions
    { G_UNICODE_SCRIPT_AVESTAN,                0x10B00 },
    { G_UNICODE_SCRIPT_BAMUM,                   0xA6A0 },
    { G_UNICODE_SCRIPT_EGYPTIAN_HIEROGLYPHS,   0x13000 },
    { G_UNICODE_SCRIPT_IMPERIAL_ARAMAIC,       0x10840 },
    { G_UNICODE_SCRIPT_INSCRIPTIONAL_PAHLAVI,  0x10B60 },
    { G_UNICODE_SCRIPT_INSCRIPTIONAL_PARTHIAN, 0x10B40 },
    { G_UNICODE_SCRIPT_JAVANESE,                0xA980 },
    { G_UNICODE_SCRIPT_KAITHI,                 0x11082 },
    { G_UNICODE_SCRIPT_LISU,                    0xA4D0 },
    { G_UNICODE_SCRIPT_MEETEI_MAYEK,            0xABE5 },
    { G_UNICODE_SCRIPT_OLD_SOUTH_ARABIAN,      0x10A60 },
    { G_UNICODE_SCRIPT_OLD_TURKISH,            0x10C00 },
    { G_UNICODE_SCRIPT_SAMARITAN,               0x0800 },
    { G_UNICODE_SCRIPT_TAI_THAM,                0x1A20 },
    { G_UNICODE_SCRIPT_TAI_VIET,                0xAA80 }
*/
  };
  for (i = 0; i < G_N_ELEMENTS (examples); i++)
    {
      g_assert (g_unichar_get_script (examples[i].c) == examples[i].script);
    }
}

static void
test_combining_class (void)
{
  gint i;
  struct {
    gint class;
    gunichar          c;
  } examples[] = {
    {   0, 0x0020 },
    {   1, 0x0334 },
    {   7, 0x093C },
    {   8, 0x3099 },
    {   9, 0x094D },
    {  10, 0x05B0 },
    {  11, 0x05B1 },
    {  12, 0x05B2 },
    {  13, 0x05B3 },
    {  14, 0x05B4 },
    {  15, 0x05B5 },
    {  16, 0x05B6 },
    {  17, 0x05B7 },
    {  18, 0x05B8 },
    {  19, 0x05B9 },
    {  20, 0x05BB },
    {  21, 0x05BC },
    {  22, 0x05BD },
    {  23, 0x05BF },
    {  24, 0x05C1 },
    {  25, 0x05C2 },
    {  26, 0xFB1E },
    {  27, 0x064B },
    {  28, 0x064C },
    {  29, 0x064D },
    /* ... */
    { 228, 0x05AE },
    { 230, 0x0300 },
    { 232, 0x302C },
    { 233, 0x0362 },
    { 234, 0x0360 },
    /* { 234, 0x1DCD }, 5.1 addition */
    { 240, 0x0345 }
  };
  for (i = 0; i < G_N_ELEMENTS (examples); i++)
    {
      g_assert (g_unichar_combining_class (examples[i].c) == examples[i].class);
    }
}

static void
test_mirror (void)
{
  gunichar mirror;

  g_assert (g_unichar_get_mirror_char ('(', &mirror));
  g_assert_cmpint (mirror, ==, ')');
  g_assert (g_unichar_get_mirror_char (')', &mirror));
  g_assert_cmpint (mirror, ==, '(');
  g_assert (g_unichar_get_mirror_char ('{', &mirror));
  g_assert_cmpint (mirror, ==, '}');
  g_assert (g_unichar_get_mirror_char ('}', &mirror));
  g_assert_cmpint (mirror, ==, '{');
  g_assert (g_unichar_get_mirror_char (0x208D, &mirror));
  g_assert_cmpint (mirror, ==, 0x208E);
  g_assert (g_unichar_get_mirror_char (0x208E, &mirror));
  g_assert_cmpint (mirror, ==, 0x208D);
  g_assert (!g_unichar_get_mirror_char ('a', &mirror));
}

static void
test_mark (void)
{
  g_assert (g_unichar_ismark (0x0903));
  g_assert (g_unichar_ismark (0x20DD));
  g_assert (g_unichar_ismark (0xA806));
  g_assert (!g_unichar_ismark ('a'));
}

static void
test_title (void)
{
  g_assert (g_unichar_istitle (0x01c5));
  g_assert (g_unichar_istitle (0x1f88));
  g_assert (g_unichar_istitle (0x1fcc));
  g_assert (!g_unichar_ismark ('a'));

  g_assert (g_unichar_totitle (0x01c6) == 0x01c5);
  g_assert (g_unichar_totitle (0x01c4) == 0x01c5);
  g_assert (g_unichar_totitle (0x01c5) == 0x01c5);
  g_assert (g_unichar_totitle (0x1f80) == 0x1f88);
  g_assert (g_unichar_totitle (0x1f88) == 0x1f88);
  g_assert (g_unichar_totitle ('a') == 'A');
  g_assert (g_unichar_totitle ('A') == 'A');
}

int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/utf8/strlen", test_utf8_strlen);
  g_test_add_func ("/utf8/strncpy", test_utf8_strncpy);
  g_test_add_func ("/utf8/strrchr", test_utf8_strrchr);
  g_test_add_func ("/utf8/reverse", test_utf8_reverse);
  g_test_add_func ("/unicode/validate", test_unichar_validate);
  g_test_add_func ("/unicode/character-type", test_unichar_character_type);
  g_test_add_func ("/unicode/break-type", test_unichar_break_type);
  g_test_add_func ("/unicode/script", test_unichar_script);
  g_test_add_func ("/unicode/combining-class", test_combining_class);
  g_test_add_func ("/unicode/mirror", test_mirror);
  g_test_add_func ("/unicode/mark", test_mark);
  g_test_add_func ("/unicode/title", test_title);

  return g_test_run();
}
