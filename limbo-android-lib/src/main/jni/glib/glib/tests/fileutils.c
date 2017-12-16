/* Unit tests for gfileutils
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
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

#include <string.h>
#include <errno.h>
#include <glib.h>
#include <glib/gstdio.h>

#define S G_DIR_SEPARATOR_S

static void
check_string (gchar *str, gchar *expected)
{
  g_assert (str != NULL);
  g_assert_cmpstr (str, ==, expected);
  g_free (str);
}

static void
test_build_path (void)
{
/*  check_string (g_build_path ("", NULL), "");*/
  check_string (g_build_path ("", "", NULL), "");
  check_string (g_build_path ("", "x", NULL), "x");
  check_string (g_build_path ("", "x", "y",  NULL), "xy");
  check_string (g_build_path ("", "x", "y", "z", NULL), "xyz");

/*  check_string (g_build_path (":", NULL), "");*/
  check_string (g_build_path (":", ":", NULL), ":");
  check_string (g_build_path (":", ":x", NULL), ":x");
  check_string (g_build_path (":", "x:", NULL), "x:");
  check_string (g_build_path (":", "", "x", NULL), "x");
  check_string (g_build_path (":", "", ":x", NULL), ":x");
  check_string (g_build_path (":", ":", "x", NULL), ":x");
  check_string (g_build_path (":", "::", "x", NULL), "::x");
  check_string (g_build_path (":", "x", "", NULL), "x");
  check_string (g_build_path (":", "x:", "", NULL), "x:");
  check_string (g_build_path (":", "x", ":", NULL), "x:");
  check_string (g_build_path (":", "x", "::", NULL), "x::");
  check_string (g_build_path (":", "x", "y",  NULL), "x:y");
  check_string (g_build_path (":", ":x", "y", NULL), ":x:y");
  check_string (g_build_path (":", "x", "y:", NULL), "x:y:");
  check_string (g_build_path (":", ":x:", ":y:", NULL), ":x:y:");
  check_string (g_build_path (":", ":x::", "::y:", NULL), ":x:y:");
  check_string (g_build_path (":", "x", "","y",  NULL), "x:y");
  check_string (g_build_path (":", "x", ":", "y",  NULL), "x:y");
  check_string (g_build_path (":", "x", "::", "y",  NULL), "x:y");
  check_string (g_build_path (":", "x", "y", "z", NULL), "x:y:z");
  check_string (g_build_path (":", ":x:", ":y:", ":z:", NULL), ":x:y:z:");
  check_string (g_build_path (":", "::x::", "::y::", "::z::", NULL), "::x:y:z::");

/*  check_string (g_build_path ("::", NULL), "");*/
  check_string (g_build_path ("::", "::", NULL), "::");
  check_string (g_build_path ("::", ":::", NULL), ":::");
  check_string (g_build_path ("::", "::x", NULL), "::x");
  check_string (g_build_path ("::", "x::", NULL), "x::");
  check_string (g_build_path ("::", "", "x", NULL), "x");
  check_string (g_build_path ("::", "", "::x", NULL), "::x");
  check_string (g_build_path ("::", "::", "x", NULL), "::x");
  check_string (g_build_path ("::", "::::", "x", NULL), "::::x");
  check_string (g_build_path ("::", "x", "", NULL), "x");
  check_string (g_build_path ("::", "x::", "", NULL), "x::");
  check_string (g_build_path ("::", "x", "::", NULL), "x::");

  /* This following is weird, but keeps the definition simple */
  check_string (g_build_path ("::", "x", ":::", NULL), "x:::::");
  check_string (g_build_path ("::", "x", "::::", NULL), "x::::");
  check_string (g_build_path ("::", "x", "y",  NULL), "x::y");
  check_string (g_build_path ("::", "::x", "y", NULL), "::x::y");
  check_string (g_build_path ("::", "x", "y::", NULL), "x::y::");
  check_string (g_build_path ("::", "::x::", "::y::", NULL), "::x::y::");
  check_string (g_build_path ("::", "::x:::", ":::y::", NULL), "::x::::y::");
  check_string (g_build_path ("::", "::x::::", "::::y::", NULL), "::x::y::");
  check_string (g_build_path ("::", "x", "", "y",  NULL), "x::y");
  check_string (g_build_path ("::", "x", "::", "y",  NULL), "x::y");
  check_string (g_build_path ("::", "x", "::::", "y",  NULL), "x::y");
  check_string (g_build_path ("::", "x", "y", "z", NULL), "x::y::z");
  check_string (g_build_path ("::", "::x::", "::y::", "::z::", NULL), "::x::y::z::");
  check_string (g_build_path ("::", ":::x:::", ":::y:::", ":::z:::", NULL), ":::x::::y::::z:::");
  check_string (g_build_path ("::", "::::x::::", "::::y::::", "::::z::::", NULL), "::::x::y::z::::");
}

static void
test_build_pathv (void)
{
  gchar *args[10];

  args[0] = NULL;
  check_string (g_build_pathv ("", args), "");
  args[0] = ""; args[1] = NULL;
  check_string (g_build_pathv ("", args), "");
  args[0] = "x"; args[1] = NULL;
  check_string (g_build_pathv ("", args), "x");
  args[0] = "x"; args[1] = "y"; args[2] = NULL;
  check_string (g_build_pathv ("", args), "xy");
  args[0] = "x"; args[1] = "y"; args[2] = "z", args[3] = NULL;
  check_string (g_build_pathv ("", args), "xyz");

  args[0] = NULL;
  check_string (g_build_pathv (":", args), "");
  args[0] = ":"; args[1] = NULL;
  check_string (g_build_pathv (":", args), ":");
  args[0] = ":x"; args[1] = NULL;
  check_string (g_build_pathv (":", args), ":x");
  args[0] = "x:"; args[1] = NULL;
  check_string (g_build_pathv (":", args), "x:");
  args[0] = ""; args[1] = "x"; args[2] = NULL;
  check_string (g_build_pathv (":", args), "x");
  args[0] = ""; args[1] = ":x"; args[2] = NULL;
  check_string (g_build_pathv (":", args), ":x");
  args[0] = ":"; args[1] = "x"; args[2] = NULL;
  check_string (g_build_pathv (":", args), ":x");
  args[0] = "::"; args[1] = "x"; args[2] = NULL;
  check_string (g_build_pathv (":", args), "::x");
  args[0] = "x"; args[1] = ""; args[2] = NULL;
  check_string (g_build_pathv (":", args), "x");
  args[0] = "x:"; args[1] = ""; args[2] = NULL;
  check_string (g_build_pathv (":", args), "x:");
  args[0] = "x"; args[1] = ":"; args[2] = NULL;
  check_string (g_build_pathv (":", args), "x:");
  args[0] = "x"; args[1] = "::"; args[2] = NULL;
  check_string (g_build_pathv (":", args), "x::");
  args[0] = "x"; args[1] = "y"; args[2] = NULL;
  check_string (g_build_pathv (":", args), "x:y");
  args[0] = ":x"; args[1] = "y"; args[2] = NULL;
  check_string (g_build_pathv (":", args), ":x:y");
  args[0] = "x"; args[1] = "y:"; args[2] = NULL;
  check_string (g_build_pathv (":", args), "x:y:");
  args[0] = ":x:"; args[1] = ":y:"; args[2] = NULL;
  check_string (g_build_pathv (":", args), ":x:y:");
  args[0] = ":x::"; args[1] = "::y:"; args[2] = NULL;
  check_string (g_build_pathv (":", args), ":x:y:");
  args[0] = "x"; args[1] = ""; args[2] = "y"; args[3] = NULL;
  check_string (g_build_pathv (":", args), "x:y");
  args[0] = "x"; args[1] = ":"; args[2] = "y"; args[3] = NULL;
  check_string (g_build_pathv (":", args), "x:y");
  args[0] = "x"; args[1] = "::"; args[2] = "y"; args[3] = NULL;
  check_string (g_build_pathv (":", args), "x:y");
  args[0] = "x"; args[1] = "y"; args[2] = "z"; args[3] = NULL;
  check_string (g_build_pathv (":", args), "x:y:z");
  args[0] = ":x:"; args[1] = ":y:"; args[2] = ":z:"; args[3] = NULL;
  check_string (g_build_pathv (":", args), ":x:y:z:");
  args[0] = "::x::"; args[1] = "::y::"; args[2] = "::z::"; args[3] = NULL;
  check_string (g_build_pathv (":", args), "::x:y:z::");

  args[0] = NULL;
  check_string (g_build_pathv ("::", args), "");
  args[0] = "::"; args[1] = NULL;
  check_string (g_build_pathv ("::", args), "::");
  args[0] = ":::"; args[1] = NULL;
  check_string (g_build_pathv ("::", args), ":::");
  args[0] = "::x"; args[1] = NULL;
  check_string (g_build_pathv ("::", args), "::x");
  args[0] = "x::"; args[1] = NULL;
  check_string (g_build_pathv ("::", args), "x::");
  args[0] = ""; args[1] = "x"; args[2] = NULL;
  check_string (g_build_pathv ("::", args), "x");
  args[0] = ""; args[1] = "::x"; args[2] = NULL;
  check_string (g_build_pathv ("::", args), "::x");
  args[0] = "::"; args[1] = "x"; args[2] = NULL;
  check_string (g_build_pathv ("::", args), "::x");
  args[0] = "::::"; args[1] = "x"; args[2] = NULL;
  check_string (g_build_pathv ("::", args), "::::x");
  args[0] = "x"; args[1] = ""; args[2] = NULL;
  check_string (g_build_pathv ("::", args), "x");
  args[0] = "x::"; args[1] = ""; args[2] = NULL;
  check_string (g_build_pathv ("::", args), "x::");
  args[0] = "x"; args[1] = "::"; args[2] = NULL;
  check_string (g_build_pathv ("::", args), "x::");
  /* This following is weird, but keeps the definition simple */
  args[0] = "x"; args[1] = ":::"; args[2] = NULL;
  check_string (g_build_pathv ("::", args), "x:::::");
  args[0] = "x"; args[1] = "::::"; args[2] = NULL;
  check_string (g_build_pathv ("::", args), "x::::");
  args[0] = "x"; args[1] = "y"; args[2] = NULL;
  check_string (g_build_pathv ("::", args), "x::y");
  args[0] = "::x"; args[1] = "y"; args[2] = NULL;
  check_string (g_build_pathv ("::", args), "::x::y");
  args[0] = "x"; args[1] = "y::"; args[2] = NULL;
  check_string (g_build_pathv ("::", args), "x::y::");
  args[0] = "::x::"; args[1] = "::y::"; args[2] = NULL;
  check_string (g_build_pathv ("::", args), "::x::y::");
  args[0] = "::x:::"; args[1] = ":::y::"; args[2] = NULL;
  check_string (g_build_pathv ("::", args), "::x::::y::");
  args[0] = "::x::::"; args[1] = "::::y::"; args[2] = NULL;
  check_string (g_build_pathv ("::", args), "::x::y::");
  args[0] = "x"; args[1] = ""; args[2] = "y"; args[3] = NULL;
  check_string (g_build_pathv ("::", args), "x::y");
  args[0] = "x"; args[1] = "::"; args[2] = "y"; args[3] = NULL;
  check_string (g_build_pathv ("::", args), "x::y");
  args[0] = "x"; args[1] = "::::"; args[2] = "y"; args[3] = NULL;
  check_string (g_build_pathv ("::", args), "x::y");
  args[0] = "x"; args[1] = "y"; args[2] = "z"; args[3] = NULL;
  check_string (g_build_pathv ("::", args), "x::y::z");
  args[0] = "::x::"; args[1] = "::y::"; args[2] = "::z::"; args[3] = NULL;
  check_string (g_build_pathv ("::", args), "::x::y::z::");
  args[0] = ":::x:::"; args[1] = ":::y:::"; args[2] = ":::z:::"; args[3] = NULL;
  check_string (g_build_pathv ("::", args), ":::x::::y::::z:::");
  args[0] = "::::x::::"; args[1] = "::::y::::"; args[2] = "::::z::::"; args[3] = NULL;
  check_string (g_build_pathv ("::", args), "::::x::y::z::::");
}

static void
test_build_filename (void)
{
/*  check_string (g_build_filename (NULL), "");*/
  check_string (g_build_filename (S, NULL), S);
  check_string (g_build_filename (S"x", NULL), S"x");
  check_string (g_build_filename ("x"S, NULL), "x"S);
  check_string (g_build_filename ("", "x", NULL), "x");
  check_string (g_build_filename ("", S"x", NULL), S"x");
  check_string (g_build_filename (S, "x", NULL), S"x");
  check_string (g_build_filename (S S, "x", NULL), S S"x");
  check_string (g_build_filename ("x", "", NULL), "x");
  check_string (g_build_filename ("x"S, "", NULL), "x"S);
  check_string (g_build_filename ("x", S, NULL), "x"S);
  check_string (g_build_filename ("x", S S, NULL), "x"S S);
  check_string (g_build_filename ("x", "y",  NULL), "x"S"y");
  check_string (g_build_filename (S"x", "y", NULL), S"x"S"y");
  check_string (g_build_filename ("x", "y"S, NULL), "x"S"y"S);
  check_string (g_build_filename (S"x"S, S"y"S, NULL), S"x"S"y"S);
  check_string (g_build_filename (S"x"S S, S S"y"S, NULL), S"x"S"y"S);
  check_string (g_build_filename ("x", "", "y",  NULL), "x"S"y");
  check_string (g_build_filename ("x", S, "y",  NULL), "x"S"y");
  check_string (g_build_filename ("x", S S, "y",  NULL), "x"S"y");
  check_string (g_build_filename ("x", "y", "z", NULL), "x"S"y"S"z");
  check_string (g_build_filename (S"x"S, S"y"S, S"z"S, NULL), S"x"S"y"S"z"S);
  check_string (g_build_filename (S S"x"S S, S S"y"S S, S S"z"S S, NULL), S S"x"S"y"S"z"S S);

#ifdef G_OS_WIN32

  /* Test also using the slash as file name separator */
#define U "/"
  check_string (g_build_filename (NULL), "");
  check_string (g_build_filename (U, NULL), U);
  check_string (g_build_filename (U"x", NULL), U"x");
  check_string (g_build_filename ("x"U, NULL), "x"U);
  check_string (g_build_filename ("", U"x", NULL), U"x");
  check_string (g_build_filename ("", U"x", NULL), U"x");
  check_string (g_build_filename (U, "x", NULL), U"x");
  check_string (g_build_filename (U U, "x", NULL), U U"x");
  check_string (g_build_filename (U S, "x", NULL), U S"x");
  check_string (g_build_filename ("x"U, "", NULL), "x"U);
  check_string (g_build_filename ("x"S"y", "z"U"a", NULL), "x"S"y"S"z"U"a");
  check_string (g_build_filename ("x", U, NULL), "x"U);
  check_string (g_build_filename ("x", U U, NULL), "x"U U);
  check_string (g_build_filename ("x", S U, NULL), "x"S U);
  check_string (g_build_filename (U"x", "y", NULL), U"x"U"y");
  check_string (g_build_filename ("x", "y"U, NULL), "x"U"y"U);
  check_string (g_build_filename (U"x"U, U"y"U, NULL), U"x"U"y"U);
  check_string (g_build_filename (U"x"U U, U U"y"U, NULL), U"x"U"y"U);
  check_string (g_build_filename ("x", U, "y",  NULL), "x"U"y");
  check_string (g_build_filename ("x", U U, "y",  NULL), "x"U"y");
  check_string (g_build_filename ("x", U S, "y",  NULL), "x"S"y");
  check_string (g_build_filename ("x", S U, "y",  NULL), "x"U"y");
  check_string (g_build_filename ("x", U "y", "z", NULL), "x"U"y"U"z");
  check_string (g_build_filename ("x", S "y", "z", NULL), "x"S"y"S"z");
  check_string (g_build_filename ("x", S "y", "z", U, "a", "b", NULL), "x"S"y"S"z"U"a"U"b");
  check_string (g_build_filename (U"x"U, U"y"U, U"z"U, NULL), U"x"U"y"U"z"U);
  check_string (g_build_filename (U U"x"U U, U U"y"U U, U U"z"U U, NULL), U U"x"U"y"U"z"U U);

#undef U

#endif /* G_OS_WIN32 */

}

static void
test_build_filenamev (void)
{
  gchar *args[10];

  args[0] = NULL;
  check_string (g_build_filenamev (args), "");
  args[0] = S; args[1] = NULL;
  check_string (g_build_filenamev (args), S);
  args[0] = S"x"; args[1] = NULL;
  check_string (g_build_filenamev (args), S"x");
  args[0] = "x"S; args[1] = NULL;
  check_string (g_build_filenamev (args), "x"S);
  args[0] = ""; args[1] = "x"; args[2] = NULL;
  check_string (g_build_filenamev (args), "x");
  args[0] = ""; args[1] = S"x"; args[2] = NULL;
  check_string (g_build_filenamev (args), S"x");
  args[0] = S; args[1] = "x"; args[2] = NULL;
  check_string (g_build_filenamev (args), S"x");
  args[0] = S S; args[1] = "x"; args[2] = NULL;
  check_string (g_build_filenamev (args), S S"x");
  args[0] = "x"; args[1] = ""; args[2] = NULL;
  check_string (g_build_filenamev (args), "x");
  args[0] = "x"S; args[1] = ""; args[2] = NULL;
  check_string (g_build_filenamev (args), "x"S);
  args[0] = "x"; args[1] = S; args[2] = NULL;
  check_string (g_build_filenamev (args), "x"S);
  args[0] = "x"; args[1] = S S; args[2] = NULL;
  check_string (g_build_filenamev (args), "x"S S);
  args[0] = "x"; args[1] = "y"; args[2] = NULL;
  check_string (g_build_filenamev (args), "x"S"y");
  args[0] = S"x"; args[1] = "y"; args[2] = NULL;
  check_string (g_build_filenamev (args), S"x"S"y");
  args[0] = "x"; args[1] = "y"S; args[2] = NULL;
  check_string (g_build_filenamev (args), "x"S"y"S);
  args[0] = S"x"S; args[1] = S"y"S; args[2] = NULL;
  check_string (g_build_filenamev (args), S"x"S"y"S);
  args[0] = S"x"S S; args[1] = S S"y"S; args[2] = NULL;
  check_string (g_build_filenamev (args), S"x"S"y"S);
  args[0] = "x"; args[1] = ""; args[2] = "y"; args[3] = NULL;
  check_string (g_build_filenamev (args), "x"S"y");
  args[0] = "x"; args[1] = S; args[2] = "y"; args[3] = NULL;
  check_string (g_build_filenamev (args), "x"S"y");
  args[0] = "x"; args[1] = S S; args[2] = "y"; args[3] = NULL;
  check_string (g_build_filenamev (args), "x"S"y");
  args[0] = "x"; args[1] = "y"; args[2] = "z"; args[3] = NULL;
  check_string (g_build_filenamev (args), "x"S"y"S"z");
  args[0] = S"x"S; args[1] = S"y"S; args[2] = S"z"S; args[3] = NULL;
  check_string (g_build_filenamev (args), S"x"S"y"S"z"S);
  args[0] = S S"x"S S; args[1] = S S"y"S S; args[2] = S S"z"S S; args[3] = NULL;
  check_string (g_build_filenamev (args), S S"x"S"y"S"z"S S);

#ifdef G_OS_WIN32

  /* Test also using the slash as file name separator */
#define U "/"
  args[0] = NULL;
  check_string (g_build_filenamev (args), "");
  args[0] = U; args[1] = NULL;
  check_string (g_build_filenamev (args), U);
  args[0] = U"x"; args[1] = NULL;
  check_string (g_build_filenamev (args), U"x");
  args[0] = "x"U; args[1] = NULL;
  check_string (g_build_filenamev (args), "x"U);
  args[0] = ""; args[1] = U"x"; args[2] = NULL;
  check_string (g_build_filenamev (args), U"x");
  args[0] = ""; args[1] = U"x"; args[2] = NULL;
  check_string (g_build_filenamev (args), U"x");
  args[0] = U; args[1] = "x"; args[2] = NULL;
  check_string (g_build_filenamev (args), U"x");
  args[0] = U U; args[1] = "x"; args[2] = NULL;
  check_string (g_build_filenamev (args), U U"x");
  args[0] = U S; args[1] = "x"; args[2] = NULL;
  check_string (g_build_filenamev (args), U S"x");
  args[0] = "x"U; args[1] = ""; args[2] = NULL;
  check_string (g_build_filenamev (args), "x"U);
  args[0] = "x"S"y"; args[1] = "z"U"a"; args[2] = NULL;
  check_string (g_build_filenamev (args), "x"S"y"S"z"U"a");
  args[0] = "x"; args[1] = U; args[2] = NULL;
  check_string (g_build_filenamev (args), "x"U);
  args[0] = "x"; args[1] = U U; args[2] = NULL;
  check_string (g_build_filenamev (args), "x"U U);
  args[0] = "x"; args[1] = S U; args[2] = NULL;
  check_string (g_build_filenamev (args), "x"S U);
  args[0] = U"x"; args[1] = "y"; args[2] = NULL;
  check_string (g_build_filenamev (args), U"x"U"y");
  args[0] = "x"; args[1] = "y"U; args[2] = NULL;
  check_string (g_build_filenamev (args), "x"U"y"U);
  args[0] = U"x"U; args[1] = U"y"U; args[2] = NULL;
  check_string (g_build_filenamev (args), U"x"U"y"U);
  args[0] = U"x"U U; args[1] = U U"y"U; args[2] = NULL;
  check_string (g_build_filenamev (args), U"x"U"y"U);
  args[0] = "x"; args[1] = U; args[2] = "y", args[3] = NULL;
  check_string (g_build_filenamev (args), "x"U"y");
  args[0] = "x"; args[1] = U U; args[2] = "y", args[3] = NULL;
  check_string (g_build_filenamev (args), "x"U"y");
  args[0] = "x"; args[1] = U S; args[2] = "y", args[3] = NULL;
  check_string (g_build_filenamev (args), "x"S"y");
  args[0] = "x"; args[1] = S U; args[2] = "y", args[3] = NULL;
  check_string (g_build_filenamev (args), "x"U"y");
  args[0] = "x"; args[1] = U "y"; args[2] = "z", args[3] = NULL;
  check_string (g_build_filenamev (args), "x"U"y"U"z");
  args[0] = "x"; args[1] = S "y"; args[2] = "z", args[3] = NULL;
  check_string (g_build_filenamev (args), "x"S"y"S"z");
  args[0] = "x"; args[1] = S "y"; args[2] = "z", args[3] = U;
  args[4] = "a"; args[5] = "b"; args[6] = NULL;
  check_string (g_build_filenamev (args), "x"S"y"S"z"U"a"U"b");
  args[0] = U"x"U; args[1] = U"y"U; args[2] = U"z"U, args[3] = NULL;
  check_string (g_build_filenamev (args), U"x"U"y"U"z"U);
  args[0] = U U"x"U U; args[1] = U U"y"U U; args[2] = U U"z"U U, args[3] = NULL;
  check_string (g_build_filenamev (args), U U"x"U"y"U"z"U U);

#undef U

#endif /* G_OS_WIN32 */
}

#undef S

static void
test_mkdir_with_parents_1 (const gchar *base)
{
  char *p0 = g_build_filename (base, "fum", NULL);
  char *p1 = g_build_filename (p0, "tem", NULL);
  char *p2 = g_build_filename (p1, "zap", NULL);
  FILE *f;

  g_remove (p2);
  g_remove (p1);
  g_remove (p0);

  if (g_file_test (p0, G_FILE_TEST_EXISTS))
    g_error ("failed, %s exists, cannot test g_mkdir_with_parents\n", p0);

  if (g_file_test (p1, G_FILE_TEST_EXISTS))
    g_error ("failed, %s exists, cannot test g_mkdir_with_parents\n", p1);

  if (g_file_test (p2, G_FILE_TEST_EXISTS))
    g_error ("failed, %s exists, cannot test g_mkdir_with_parents\n", p2);

  if (g_mkdir_with_parents (p2, 0777) == -1)
    g_error ("failed, g_mkdir_with_parents(%s) failed: %s\n", p2, g_strerror (errno));

  if (!g_file_test (p2, G_FILE_TEST_IS_DIR))
    g_error ("failed, g_mkdir_with_parents(%s) succeeded, but %s is not a directory\n", p2, p2);

  if (!g_file_test (p1, G_FILE_TEST_IS_DIR))
    g_error ("failed, g_mkdir_with_parents(%s) succeeded, but %s is not a directory\n", p2, p1);

  if (!g_file_test (p0, G_FILE_TEST_IS_DIR))
    g_error ("failed, g_mkdir_with_parents(%s) succeeded, but %s is not a directory\n", p2, p0);

  g_rmdir (p2);
  if (g_file_test (p2, G_FILE_TEST_EXISTS))
    g_error ("failed, did g_rmdir(%s), but %s is still there\n", p2, p2);

  g_rmdir (p1);
  if (g_file_test (p1, G_FILE_TEST_EXISTS))
    g_error ("failed, did g_rmdir(%s), but %s is still there\n", p1, p1);

  f = g_fopen (p1, "w");
  if (f == NULL)
    g_error ("failed, couldn't create file %s\n", p1);
  fclose (f);

  if (g_mkdir_with_parents (p1, 0666) == 0)
    g_error ("failed, g_mkdir_with_parents(%s) succeeded, even if %s is a file\n", p1, p1);

  if (g_mkdir_with_parents (p2, 0666) == 0)
    g_error("failed, g_mkdir_with_parents(%s) succeeded, even if %s is a file\n", p2, p1);

  g_remove (p2);
  g_remove (p1);
  g_remove (p0);
}

static void
test_mkdir_with_parents (void)
{
  gchar *cwd;
  if (g_test_verbose())
    g_print ("checking g_mkdir_with_parents() in subdir ./hum/");
  test_mkdir_with_parents_1 ("hum");
  g_remove ("hum");
  if (g_test_verbose())
    g_print ("checking g_mkdir_with_parents() in subdir ./hii///haa/hee/");
  test_mkdir_with_parents_1 ("hii///haa/hee");
  g_remove ("hii/haa/hee");
  g_remove ("hii/haa");
  g_remove ("hii");
  cwd = g_get_current_dir ();
  if (g_test_verbose())
    g_print ("checking g_mkdir_with_parents() in cwd: %s", cwd);
  test_mkdir_with_parents_1 (cwd);
  g_free (cwd);
}

static void
test_format_size_for_display (void)
{
  check_string (g_format_size_for_display (0), "0 bytes");
  check_string (g_format_size_for_display (1), "1 byte");
  check_string (g_format_size_for_display (2), "2 bytes");
  check_string (g_format_size_for_display (1024), "1.0 KB");
  check_string (g_format_size_for_display (1024 * 1024), "1.0 MB");
  check_string (g_format_size_for_display (1024 * 1024 * 1024), "1.0 GB");
}

int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/fileutils/build-path", test_build_path);
  g_test_add_func ("/fileutils/build-pathv", test_build_pathv);
  g_test_add_func ("/fileutils/build-filename", test_build_filename);
  g_test_add_func ("/fileutils/build-filenamev", test_build_filenamev);
  g_test_add_func ("/fileutils/mkdir-with-parents", test_mkdir_with_parents);
  g_test_add_func ("/fileutils/format-size-for-display", test_format_size_for_display);

  return g_test_run();
}
