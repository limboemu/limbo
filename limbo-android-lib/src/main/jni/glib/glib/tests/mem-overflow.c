/* Unit tests for g
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
 */

/* We test for errors in optimize-only definitions in gmem.h */

#pragma GCC optimize (1)

#include "glib.h"
#include <stdlib.h>

static void
mem_overflow (void)
{
  gsize a = G_MAXSIZE / 10 + 10;
  gsize b = 10;
  gpointer p, q;
  typedef char X[10];

#define CHECK_PASS(P)	p = (P); g_assert (p == NULL);
#define CHECK_FAIL(P)	p = (P); g_assert (p != NULL);

  CHECK_PASS (g_try_malloc_n (a, a));
  CHECK_PASS (g_try_malloc_n (a, b));
  CHECK_PASS (g_try_malloc_n (b, a));
  CHECK_FAIL (g_try_malloc_n (b, b));

  CHECK_PASS (g_try_malloc0_n (a, a));
  CHECK_PASS (g_try_malloc0_n (a, b));
  CHECK_PASS (g_try_malloc0_n (b, a));
  CHECK_FAIL (g_try_malloc0_n (b, b));

  q = g_malloc (1);
  CHECK_PASS (g_try_realloc_n (q, a, a));
  CHECK_PASS (g_try_realloc_n (q, a, b));
  CHECK_PASS (g_try_realloc_n (q, b, a));
  CHECK_FAIL (g_try_realloc_n (q, b, b));
  free (p);

  CHECK_PASS (g_try_new (X, a));
  CHECK_FAIL (g_try_new (X, b));

  CHECK_PASS (g_try_new0 (X, a));
  CHECK_FAIL (g_try_new0 (X, b));

  q = g_try_malloc (1);
  CHECK_PASS (g_try_renew (X, q, a));
  CHECK_FAIL (g_try_renew (X, q, b));
  free (p);

#undef CHECK_FAIL
#undef CHECK_PASS

#define CHECK_FAIL(P)	if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDERR)) { p = (P); exit (0); } g_test_trap_assert_failed();
#define CHECK_PASS(P)	if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDERR)) { p = (P); exit (0); } g_test_trap_assert_passed();

  CHECK_FAIL (g_malloc_n (a, a));
  CHECK_FAIL (g_malloc_n (a, b));
  CHECK_FAIL (g_malloc_n (b, a));
  CHECK_PASS (g_malloc_n (b, b));

  CHECK_FAIL (g_malloc0_n (a, a));
  CHECK_FAIL (g_malloc0_n (a, b));
  CHECK_FAIL (g_malloc0_n (b, a));
  CHECK_PASS (g_malloc0_n (b, b));

  q = g_malloc (1);
  CHECK_FAIL (g_realloc_n (q, a, a));
  CHECK_FAIL (g_realloc_n (q, a, b));
  CHECK_FAIL (g_realloc_n (q, b, a));
  CHECK_PASS (g_realloc_n (q, b, b));
  free (q);

  CHECK_FAIL (g_new (X, a));
  CHECK_PASS (g_new (X, b));

  CHECK_FAIL (g_new0 (X, a));
  CHECK_PASS (g_new0 (X, b));

  q = g_malloc (1);
  CHECK_FAIL (g_renew (X, q, a));
  CHECK_PASS (g_renew (X, q, b));
  free (q);
}

typedef struct
{
} Empty;

static void
empty_alloc (void)
{
  g_test_bug ("615379");

  g_assert_cmpint (sizeof (Empty), ==, 0);

  if (g_test_trap_fork (0, 0))
    {
      Empty *empty;

      empty = g_new0 (Empty, 1);
      g_assert (empty == NULL);
      exit (0);
    }
  g_test_trap_assert_passed ();
}

int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_bug_base ("http://bugzilla.gnome.org/");

  g_test_add_func ("/mem/overflow", mem_overflow);
  g_test_add_func ("/mem/empty-alloc", empty_alloc);

  return g_test_run();
}
