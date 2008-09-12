/*
  Copyright (c) 2005-2008 Gordon Gremme <gremme@zbh.uni-hamburg.de>
  Copyright (c) 2005-2008 Center for Bioinformatics, University of Hamburg

  Permission to use, copy, modify, and distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include <assert.h>
#include <limits.h>
#include <string.h>
#include "core/array.h"
#include "core/dynalloc.h"
#include "core/ensure.h"
#include "core/ma.h"
#include "core/mathsupport.h"
#include "core/range.h"
#include "core/unused_api.h"
#include "core/xansi.h"

#define NUM_OF_TESTS    100
#define MAX_SIZE        1024

struct GtArray {
  void *space;
  unsigned long next_free;
  size_t allocated,
         size_of_elem;
  unsigned int reference_count;
};

GtArray* gt_array_new(size_t size_of_elem)
{
  GtArray *a = gt_calloc(1, sizeof *a);
  assert(size_of_elem);
  a->size_of_elem = size_of_elem;
  return a;
}

GtArray* gt_array_clone(const GtArray *a)
{
  GtArray *a_copy;
  assert(a);
  a_copy = gt_malloc(sizeof (GtArray));
  a_copy->space = gt_malloc(a->next_free * a->size_of_elem);
  memcpy(a_copy->space, a->space, a->next_free * a->size_of_elem);
  a_copy->next_free = a_copy->allocated = a->next_free;
  a_copy->size_of_elem = a->size_of_elem;
  a_copy->reference_count = 0;
  return a_copy;
}

GtArray* gt_array_ref(GtArray *a)
{
  if (!a) return NULL;
  a->reference_count++;
  return a;
}

void* gt_array_get(const GtArray *a, unsigned long idx)
{
  assert(a && idx < a->next_free);
  return (char*) a->space + idx * a->size_of_elem;
}

void* gt_array_get_first(const GtArray *a)
{
  return gt_array_get(a, 0);
}

void* gt_array_get_last(const GtArray *a)
{
  assert(a->next_free);
  return gt_array_get(a, a->next_free-1);
}

void* gt_array_pop(GtArray *a)
{
  assert(a && a->next_free);
  a->next_free--;
  return (char*) a->space + a->next_free * a->size_of_elem;
}

void gt_array_rem(GtArray *a, unsigned long idx)
{
  unsigned long i;
  assert(a && idx < a->next_free);
  /* move elements */
  for (i = idx+1; i < a->next_free; i++) {
    memcpy((char*) a->space + (i-1) * a->size_of_elem,
           (char*) a->space + i * a->size_of_elem,
           a->size_of_elem);
  }
  /* remove last (now duplicated) element */
  a->next_free--;
}

void gt_array_reverse(GtArray *a)
{
  char *front, *back, *tmp;
  assert(a);
  tmp = gt_malloc(a->size_of_elem);
  for (front = a->space,
       back = (char*) a->space + (a->next_free-1) * a->size_of_elem;
       front < back;
       front += a->size_of_elem, back -= a->size_of_elem) {
    memcpy(tmp, front, a->size_of_elem);
    memcpy(front, back, a->size_of_elem);
    memcpy(back, tmp, a->size_of_elem);
  }
  gt_free(tmp);
}

void* gt_array_get_space(const GtArray *a)
{
  assert(a);
  return a->space;
}

void gt_array_add_elem(GtArray *a, void *elem, size_t size_of_elem)
{
  assert(a && elem);
  assert(a->size_of_elem == size_of_elem);
  assert(a->next_free <= a->allocated);
  /* make sure we have enough space */
  if ((a->next_free + 1) * size_of_elem > a->allocated) {
    a->space = gt_dynalloc(a->space, &a->allocated,
                           (a->next_free + 1) * size_of_elem);
  }
  /* add */
  memcpy((char*) a->space + a->next_free * size_of_elem, elem, size_of_elem);
  a->next_free++;
}

void gt_array_add_array(GtArray *dest, const GtArray *src)
{
  unsigned long i;
  assert(dest && src && dest->size_of_elem == src->size_of_elem);
  for (i = 0; i < gt_array_size(src); i++)
    gt_array_add_elem(dest, gt_array_get(src, i), src->size_of_elem);
}

size_t gt_array_elem_size(const GtArray *a)
{
  assert(a);
  return a->size_of_elem;
}

unsigned long gt_array_size(const GtArray *a)
{
  return a ? a->next_free : 0;
}

void gt_array_set_size(GtArray *a, unsigned long size)
{
  assert(a);
  assert(size <= a->next_free);
  a->next_free = size;
}

void gt_array_reset(GtArray *a)
{
  assert(a);
  a->next_free = 0;
}

void gt_array_sort(GtArray *a, GT_Compare compar)
{
  assert(a && compar);
  qsort(a->space, a->next_free, a->size_of_elem, compar);
}

int gt_array_cmp(const GtArray *array_a, const GtArray *array_b)
{
  assert(gt_array_size(array_a) == gt_array_size(array_b));
  assert(gt_array_elem_size(array_a) == gt_array_elem_size(array_b));
  return memcmp(array_a->space, array_b->space,
                array_a->size_of_elem * array_a->next_free);
}

int gt_array_iterate(GtArray *a, GtArrayProcessor array_processor, void *info,
                  GtError *err)
{
  unsigned long idx;
  int rval;
  gt_error_check(err);
  assert(a && array_processor);
  for (idx = 0; idx < gt_array_size(a); idx++) {
    if ((rval = array_processor(gt_array_get(a, idx), info, err)))
      return rval;
  }
  return 0;
}

int gt_array_iterate_reverse(GtArray *a, GtArrayProcessor array_processor,
                             void *info, GtError *err)
{
  unsigned long idx;
  int rval;
  gt_error_check(err);
  assert(a && array_processor);
  for (idx = gt_array_size(a); idx > 0; idx--) {
    if ((rval = array_processor(gt_array_get(a, idx-1), info, err)))
      return rval;
  }
  return 0;
}

static int iterate_test_func(void *value, void *info, GT_UNUSED GtError *err)
{
  unsigned long *i;
  GtRange range;
  int had_err = 0;
  gt_error_check(err);
  i = (unsigned long*) info;
  range = *(GtRange*) value;
  ensure(had_err, range.start == *i + 1);
  ensure(had_err, range.end == *i + 101);
  (*i)++;
  return had_err;
}

static int iterate_fail_func(GT_UNUSED void *value, GT_UNUSED void *info,
                             GT_UNUSED GtError *err)
{
  return -1;
}

int gt_array_example(GT_UNUSED GtError *err)
{
  unsigned long i;
  GtArray *a;

  gt_error_check(err);

  /* an example array use case */

  a = gt_array_new(sizeof (unsigned long));
  for (i = 0; i < 100; i++) {
    gt_array_add(a, i);
    assert(i == *(unsigned long*) gt_array_get(a, i));
  }
  assert(gt_array_size(a) == 100);
  assert(*(unsigned long*) gt_array_pop(a) == 99);
  assert(gt_array_size(a) == 99);
  gt_array_delete(a);

  return 0;
}

int gt_array_unit_test(GtError *err)
{
  GtArray *char_array, *int_array, *a = NULL, *aref, *aclone;
  char cc, *char_array_test;
  int ci, *int_array_test;
  unsigned long i, j, size;
  GtRange range;
  int had_err = 0;
  gt_error_check(err);

  /* testing an empty array */
  char_array = gt_array_new(sizeof (char));
  gt_array_delete(char_array);
  int_array = gt_array_new(sizeof (int));
  gt_array_delete(int_array);

  char_array = gt_array_new(sizeof (char));
  int_array = gt_array_new(sizeof (int));
  char_array_test = gt_malloc((MAX_SIZE + 1) * sizeof (char));
  int_array_test = gt_malloc(MAX_SIZE * sizeof (int));

  for (i = 0; !had_err && i < NUM_OF_TESTS; i++) {
    size = gt_rand_max(MAX_SIZE);

    gt_array_reset(char_array);
    gt_array_set_size(int_array, 0);

    ensure(had_err, gt_array_size(char_array) == 0);
    ensure(had_err, gt_array_size(int_array) == 0);

    for (i = 0; !had_err && i < size; i++) {
      cc = gt_rand_max(CHAR_MAX);
      ci = gt_rand_max(INT_MAX);

      gt_array_add(char_array, cc);
      gt_array_add(int_array, ci);

      ensure(had_err, gt_array_size(char_array) == i+1);
      ensure(had_err, gt_array_size(int_array) == i+1);
      ensure(had_err, *((char*) gt_array_get(char_array, i)) == cc);
      ensure(had_err, *((int*) gt_array_get(int_array, i)) == ci);

      gt_array_add_elem(char_array, &cc, sizeof (char));
      gt_array_add_elem(int_array, &ci, sizeof (int));

      ensure(had_err, gt_array_size(char_array) == i+2);
      ensure(had_err, gt_array_size(int_array) == i+2);
      ensure(had_err, *((char*) gt_array_get(char_array, i+1)) == cc);
      ensure(had_err, *((int*) gt_array_get(int_array, i+1)) == ci);
      ensure(had_err, *((char*) gt_array_pop(char_array)) == cc);
      ensure(had_err, *((int*) gt_array_pop(int_array)) == ci);
      ensure(had_err, gt_array_size(char_array) == i+1);
      ensure(had_err, gt_array_size(int_array) == i+1);
      ensure(had_err, *((char*) gt_array_get(char_array, i)) == cc);
      ensure(had_err, *((int*) gt_array_get(int_array, i)) == ci);

      char_array_test[i] = cc;
      char_array_test[i+1]= '\0';
      int_array_test[i] = ci;

      ensure(had_err, strncmp(gt_array_get_space(char_array), char_array_test,
                              strlen(char_array_test)) == 0);

      for (j = 0; j <= i && !had_err; j++)
        ensure(had_err,
               *(int*) gt_array_get(int_array, j) == int_array_test[j]);
    }
  }

  /* test gt_array_reverse(), gt_array_iterate(), gt_array_rem(), and
     gt_array_ref()
   */
  if (!had_err) {
    a = gt_array_new(sizeof (GtRange));
    for (i = 0; i < 24; i++) {
      range.start = i + 1;
      range.end   = i + 101;
      gt_array_add(a, range);
    }
    gt_array_reverse(a);
    for (i = 0; !had_err && i < 24; i++) {
      range.start = i + 1;
      range.end   = i + 101;
      ensure(had_err,
             !gt_range_compare(range, *(GtRange*) gt_array_get(a, 23 - i)));
    }
    aref = gt_array_ref(a);
    gt_array_delete(aref);
  }
  if (!had_err) {
    i = 0;
    had_err = gt_array_iterate_reverse(a, iterate_test_func, &i, err);
    ensure(had_err, gt_array_iterate_reverse(a, iterate_fail_func, NULL, err));
  }
  if (!had_err) {
    gt_array_reverse(a);
    i = 0;
    had_err = gt_array_iterate(a, iterate_test_func, &i, err);
    ensure(had_err, gt_array_iterate(a, iterate_fail_func, NULL, err));
  }
  if (!had_err) {
    aclone = gt_array_clone(a);
    for (i = 0;!had_err && i < gt_array_size(a); i++) {
      ensure(had_err, !gt_range_compare_ptr(gt_array_get(a, i),
                                         gt_array_get(aclone, i)));
    }
    gt_array_delete(aclone);
  }
  if (!had_err) {
    gt_array_rem(a, 13);
    gt_array_rem(a, 12);
    gt_array_rem(a, 11);
    gt_array_rem(a, 10);
    ensure(had_err, gt_array_size(a) == 20);
    for (i = 0; !had_err && i < 10; i++) {
      range.start = i + 1;
      range.end   = i + 101;
      ensure(had_err,
             !gt_range_compare(range, *(GtRange*) gt_array_get(a, i)));
    }
    for (i = 10; !had_err && i < 20; i++) {
      range.start = 4 + i + 1;
      range.end   = 4 + i + 101;
      ensure(had_err,
             !gt_range_compare(range, *(GtRange*) gt_array_get(a, i)));
    }
  }
  gt_array_delete(a);

  gt_array_delete(char_array);
  gt_array_delete(int_array);
  gt_free(char_array_test);
  gt_free(int_array_test);

  return had_err;
}

void gt_array_delete(GtArray *a)
{
  if (!a) return;
  if (a->reference_count) {
    a->reference_count--;
    return;
  }
  gt_free(a->space);
  gt_free(a);
}
