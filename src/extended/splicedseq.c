/*
  Copyright (c) 2006-2007 Gordon Gremme <gremme@zbh.uni-hamburg.de>
  Copyright (c) 2006-2007 Center for Bioinformatics, University of Hamburg

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

#include <string.h>
#include "core/array.h"
#include "core/ensure.h"
#include "core/ma.h"
#include "core/str.h"
#include "extended/reverse.h"
#include "extended/splicedseq.h"

struct Splicedseq {
  GtStr *splicedseq;
  GtArray *positionmapping;
  bool forward;
};

Splicedseq* splicedseq_new(void)
{
  Splicedseq *ss = gt_malloc(sizeof (Splicedseq));
  ss->splicedseq = gt_str_new();
  ss->positionmapping = gt_array_new(sizeof (unsigned long));
  ss->forward = true;
  return ss;
}

void splicedseq_add(Splicedseq *ss, unsigned long start, unsigned long end,
                    const char *original_sequence)
{
  unsigned long i;
  assert(ss && start <= end && original_sequence);
  gt_str_append_cstr_nt(ss->splicedseq, original_sequence + start,
                        end - start + 1);
  /* make sure elemnts are added in ascending order */
  assert(!gt_array_size(ss->positionmapping) ||
         start > *(unsigned long*) gt_array_get_last(ss->positionmapping));
  for (i = start; i <= end; i++)
    gt_array_add(ss->positionmapping, i);
}

char* splicedseq_get(const Splicedseq *ss)
{
  return gt_str_get(ss->splicedseq);
}

bool splicedseq_pos_is_border(const Splicedseq *ss, unsigned long pos)
{
  assert(ss &&
         gt_str_length(ss->splicedseq) == gt_array_size(ss->positionmapping));
  assert(pos < gt_str_length(ss->splicedseq)); /* legal position */
  if (ss->forward && pos + 1 < gt_array_size(ss->positionmapping) &&
      *(unsigned long*) gt_array_get(ss->positionmapping, pos) + 1 !=
      *(unsigned long*) gt_array_get(ss->positionmapping, pos+1)) {
    return true;
  }
  if (!ss->forward && pos &&
      *(unsigned long*) gt_array_get(ss->positionmapping, pos-1) - 1 !=
      *(unsigned long*) gt_array_get(ss->positionmapping, pos)) {
    return true;
  }
  return false;
}

unsigned long splicedseq_map(const Splicedseq *ss, unsigned long pos)
{
  assert(ss &&
         gt_str_length(ss->splicedseq) == gt_array_size(ss->positionmapping));
  assert(pos < gt_str_length(ss->splicedseq)); /* legal position */
  return *(unsigned long*) gt_array_get(ss->positionmapping, pos);
}

unsigned long splicedseq_length(const Splicedseq *ss)
{
  assert(ss);
  return gt_str_length(ss->splicedseq);
}

int splicedseq_reverse(Splicedseq *ss, GT_Error *err)
{
  int had_err;
  gt_error_check(err);
  assert(ss);
  had_err = reverse_complement(gt_str_get(ss->splicedseq),
                               gt_str_length(ss->splicedseq), err);
  if (!had_err) {
    gt_array_reverse(ss->positionmapping);
    ss->forward = !ss->forward;
  }
  return had_err;
}

void splicedseq_reset(Splicedseq *ss)
{
  assert(ss);
  gt_str_reset(ss->splicedseq);
  gt_array_reset(ss->positionmapping);
  ss->forward = true;
}

static int check_splicedseq(Splicedseq *ss, GT_Error *err)
{                       /*0123456789*/
  static char *origseq = "aaccaagtga", *splicedseq = "ccgtg";
  int had_err = 0;
  gt_error_check(err);
  splicedseq_add(ss, 2, 3, origseq);
  splicedseq_add(ss, 6, 8, origseq);
  ensure(had_err, strcmp(splicedseq_get(ss), splicedseq) == 0);
  ensure(had_err, !splicedseq_pos_is_border(ss, 0));
  ensure(had_err,  splicedseq_pos_is_border(ss, 1));
  ensure(had_err, !splicedseq_pos_is_border(ss, 2));
  ensure(had_err, !splicedseq_pos_is_border(ss, 3));
  ensure(had_err, !splicedseq_pos_is_border(ss, 4));
  return had_err;
}

int splicedseq_unit_test(GT_Error *err)
{
  Splicedseq *ss;
  int had_err = 0;
  gt_error_check(err);
  ss = splicedseq_new();
  had_err = check_splicedseq(ss, err);
  if (!had_err) {
    splicedseq_reset(ss);
    had_err = check_splicedseq(ss, err);
  }
  splicedseq_delete(ss);
  return had_err;
}

void splicedseq_delete(Splicedseq *ss)
{
  if (!ss) return;
  gt_str_delete(ss->splicedseq);
  gt_array_delete(ss->positionmapping);
  gt_free(ss);
}
