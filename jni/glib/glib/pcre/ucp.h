/*************************************************
*          Unicode Property Table handler        *
*************************************************/

#ifndef _UCP_H
#define _UCP_H

/* This file contains definitions of the property values that are returned by
the UCD access macros. New values that are added for new releases of Unicode
should always be at the end of each enum, for backwards compatibility. */

/* These are the general character categories. */

enum {
  ucp_C,     /* Other */
  ucp_L,     /* Letter */
  ucp_M,     /* Mark */
  ucp_N,     /* Number */
  ucp_P,     /* Punctuation */
  ucp_S,     /* Symbol */
  ucp_Z      /* Separator */
};

/* These are the particular character types. */

enum {
  ucp_Cc,    /* Control */
  ucp_Cf,    /* Format */
  ucp_Cn,    /* Unassigned */
  ucp_Co,    /* Private use */
  ucp_Cs,    /* Surrogate */
  ucp_Ll,    /* Lower case letter */
  ucp_Lm,    /* Modifier letter */
  ucp_Lo,    /* Other letter */
  ucp_Lt,    /* Title case letter */
  ucp_Lu,    /* Upper case letter */
  ucp_Mc,    /* Spacing mark */
  ucp_Me,    /* Enclosing mark */
  ucp_Mn,    /* Non-spacing mark */
  ucp_Nd,    /* Decimal number */
  ucp_Nl,    /* Letter number */
  ucp_No,    /* Other number */
  ucp_Pc,    /* Connector punctuation */
  ucp_Pd,    /* Dash punctuation */
  ucp_Pe,    /* Close punctuation */
  ucp_Pf,    /* Final punctuation */
  ucp_Pi,    /* Initial punctuation */
  ucp_Po,    /* Other punctuation */
  ucp_Ps,    /* Open punctuation */
  ucp_Sc,    /* Currency symbol */
  ucp_Sk,    /* Modifier symbol */
  ucp_Sm,    /* Mathematical symbol */
  ucp_So,    /* Other symbol */
  ucp_Zl,    /* Line separator */
  ucp_Zp,    /* Paragraph separator */
  ucp_Zs     /* Space separator */
};

/* These are the script identifications. */

enum {
  ucp_Arabic = G_UNICODE_SCRIPT_ARABIC,
  ucp_Armenian = G_UNICODE_SCRIPT_ARMENIAN,
  ucp_Bengali = G_UNICODE_SCRIPT_BENGALI,
  ucp_Bopomofo = G_UNICODE_SCRIPT_BOPOMOFO,
  ucp_Braille = G_UNICODE_SCRIPT_BRAILLE,
  ucp_Buginese = G_UNICODE_SCRIPT_BUGINESE,
  ucp_Buhid = G_UNICODE_SCRIPT_BUHID,
  ucp_Canadian_Aboriginal = G_UNICODE_SCRIPT_CANADIAN_ABORIGINAL,
  ucp_Cherokee = G_UNICODE_SCRIPT_CHEROKEE,
  ucp_Common = G_UNICODE_SCRIPT_COMMON,
  ucp_Coptic = G_UNICODE_SCRIPT_COPTIC,
  ucp_Cypriot = G_UNICODE_SCRIPT_CYPRIOT,
  ucp_Cyrillic = G_UNICODE_SCRIPT_CYRILLIC,
  ucp_Deseret = G_UNICODE_SCRIPT_DESERET,
  ucp_Devanagari = G_UNICODE_SCRIPT_DEVANAGARI,
  ucp_Ethiopic = G_UNICODE_SCRIPT_ETHIOPIC,
  ucp_Georgian = G_UNICODE_SCRIPT_GEORGIAN,
  ucp_Glagolitic = G_UNICODE_SCRIPT_GLAGOLITIC,
  ucp_Gothic = G_UNICODE_SCRIPT_GOTHIC,
  ucp_Greek = G_UNICODE_SCRIPT_GREEK,
  ucp_Gujarati = G_UNICODE_SCRIPT_GUJARATI,
  ucp_Gurmukhi = G_UNICODE_SCRIPT_GURMUKHI,
  ucp_Han = G_UNICODE_SCRIPT_HAN,
  ucp_Hangul = G_UNICODE_SCRIPT_HANGUL,
  ucp_Hanunoo = G_UNICODE_SCRIPT_HANUNOO,
  ucp_Hebrew = G_UNICODE_SCRIPT_HEBREW,
  ucp_Hiragana = G_UNICODE_SCRIPT_HIRAGANA,
  ucp_Inherited = G_UNICODE_SCRIPT_INHERITED,
  ucp_Kannada = G_UNICODE_SCRIPT_KANNADA,
  ucp_Katakana = G_UNICODE_SCRIPT_KATAKANA,
  ucp_Kharoshthi = G_UNICODE_SCRIPT_KHAROSHTHI,
  ucp_Khmer = G_UNICODE_SCRIPT_KHMER,
  ucp_Lao = G_UNICODE_SCRIPT_LAO,
  ucp_Latin = G_UNICODE_SCRIPT_LATIN,
  ucp_Limbu = G_UNICODE_SCRIPT_LIMBU,
  ucp_Linear_B = G_UNICODE_SCRIPT_LINEAR_B,
  ucp_Malayalam = G_UNICODE_SCRIPT_MALAYALAM,
  ucp_Mongolian = G_UNICODE_SCRIPT_MONGOLIAN,
  ucp_Myanmar = G_UNICODE_SCRIPT_MYANMAR,
  ucp_New_Tai_Lue = G_UNICODE_SCRIPT_NEW_TAI_LUE,
  ucp_Ogham = G_UNICODE_SCRIPT_OGHAM,
  ucp_Old_Italic = G_UNICODE_SCRIPT_OLD_ITALIC,
  ucp_Old_Persian = G_UNICODE_SCRIPT_OLD_PERSIAN,
  ucp_Oriya = G_UNICODE_SCRIPT_ORIYA,
  ucp_Osmanya = G_UNICODE_SCRIPT_OSMANYA,
  ucp_Runic = G_UNICODE_SCRIPT_RUNIC,
  ucp_Shavian = G_UNICODE_SCRIPT_SHAVIAN,
  ucp_Sinhala = G_UNICODE_SCRIPT_SINHALA,
  ucp_Syloti_Nagri = G_UNICODE_SCRIPT_SYLOTI_NAGRI,
  ucp_Syriac = G_UNICODE_SCRIPT_SYRIAC,
  ucp_Tagalog = G_UNICODE_SCRIPT_TAGALOG,
  ucp_Tagbanwa = G_UNICODE_SCRIPT_TAGBANWA,
  ucp_Tai_Le = G_UNICODE_SCRIPT_TAI_LE,
  ucp_Tamil = G_UNICODE_SCRIPT_TAMIL,
  ucp_Telugu = G_UNICODE_SCRIPT_TELUGU,
  ucp_Thaana = G_UNICODE_SCRIPT_THAANA,
  ucp_Thai = G_UNICODE_SCRIPT_THAI,
  ucp_Tibetan = G_UNICODE_SCRIPT_TIBETAN,
  ucp_Tifinagh = G_UNICODE_SCRIPT_TIFINAGH,
  ucp_Ugaritic = G_UNICODE_SCRIPT_UGARITIC,
  ucp_Yi = G_UNICODE_SCRIPT_YI,
  ucp_Balinese = G_UNICODE_SCRIPT_BALINESE,
  ucp_Cuneiform = G_UNICODE_SCRIPT_CUNEIFORM,
  ucp_Nko = G_UNICODE_SCRIPT_NKO,
  ucp_Phags_Pa = G_UNICODE_SCRIPT_PHAGS_PA,
  ucp_Phoenician = G_UNICODE_SCRIPT_PHOENICIAN,
  ucp_Carian = G_UNICODE_SCRIPT_CARIAN,
  ucp_Cham = G_UNICODE_SCRIPT_CHAM,
  ucp_Kayah_Li = G_UNICODE_SCRIPT_KAYAH_LI,
  ucp_Lepcha = G_UNICODE_SCRIPT_LEPCHA,
  ucp_Lycian = G_UNICODE_SCRIPT_LYCIAN,
  ucp_Lydian = G_UNICODE_SCRIPT_LYDIAN,
  ucp_Ol_Chiki = G_UNICODE_SCRIPT_OL_CHIKI,
  ucp_Rejang = G_UNICODE_SCRIPT_REJANG,
  ucp_Saurashtra = G_UNICODE_SCRIPT_SAURASHTRA,
  ucp_Sundanese = G_UNICODE_SCRIPT_SUNDANESE,
  ucp_Vai = G_UNICODE_SCRIPT_VAI,
  ucp_Avestan = G_UNICODE_SCRIPT_AVESTAN,
  ucp_Bamum = G_UNICODE_SCRIPT_BAMUM,
  ucp_Egyptian_Hieroglyphs = G_UNICODE_SCRIPT_EGYPTIAN_HIEROGLYPHS,
  ucp_Imperial_Aramaic = G_UNICODE_SCRIPT_IMPERIAL_ARAMAIC,
  ucp_Inscriptional_Pahlavi = G_UNICODE_SCRIPT_INSCRIPTIONAL_PAHLAVI,
  ucp_Inscriptional_Parthian = G_UNICODE_SCRIPT_INSCRIPTIONAL_PARTHIAN,
  ucp_Javanese = G_UNICODE_SCRIPT_JAVANESE,
  ucp_Kaithi = G_UNICODE_SCRIPT_KAITHI,
  ucp_Lisu = G_UNICODE_SCRIPT_LISU,
  ucp_Meetei_Mayek = G_UNICODE_SCRIPT_MEETEI_MAYEK,
  ucp_Old_South_Arabian = G_UNICODE_SCRIPT_OLD_SOUTH_ARABIAN,
  ucp_Old_Turkic = G_UNICODE_SCRIPT_OLD_TURKISH,
  ucp_Samaritan = G_UNICODE_SCRIPT_SAMARITAN,
  ucp_Tai_Tham = G_UNICODE_SCRIPT_TAI_THAM,
  ucp_Tai_Viet = G_UNICODE_SCRIPT_TAI_VIET
};

#endif

/* End of ucp.h */

