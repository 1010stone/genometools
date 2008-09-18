/*
  Copyright (c) 2006-2008 Gordon Gremme <gremme@zbh.uni-hamburg.de>
  Copyright (c)      2007 Stefan Kurtz <kurtz@zbh.uni-hamburg.de>
  Copyright (c)      2008 Thomas Jahns <Thomas.Jahns@gmx.net>
  Copyright (c) 2006-2008 Center for Bioinformatics, University of Hamburg

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
#include <stdio.h>
#include "core/disc_distri.h"
#include "core/ensure.h"
#include "core/hashmap-generic.h"
#include "core/ma.h"
#include "core/unused_api.h"

struct GtDiscDistri {
  GtHashtable *hashdist;
  unsigned long long num_of_occurrences;
};

GtDiscDistri* gt_disc_distri_new(void)
{
  return gt_calloc(1, sizeof (GtDiscDistri));
}

void gt_disc_distri_add(GtDiscDistri *d, unsigned long key)
{
  gt_disc_distri_add_multi(d, key, 1);
}

DECLARE_HASHMAP(unsigned long, ul, unsigned long long, ull, static, inline)
DEFINE_HASHMAP(unsigned long, ul, unsigned long long, ull, ht_ul_elem_hash,
               ht_ul_elem_cmp, NULL_DESTRUCTOR, NULL_DESTRUCTOR, static,
               inline)

void gt_disc_distri_add_multi(GtDiscDistri *d, unsigned long key,
                           unsigned long long occurrences)
{
  unsigned long long *valueptr;
  assert(d);

  if (!d->hashdist)
    d->hashdist = ul_ull_gt_hashmap_new();

  valueptr = ul_ull_gt_hashmap_get(d->hashdist, key);
  if (!valueptr) {
    ul_ull_gt_hashmap_add(d->hashdist, key, occurrences);
  }
  else
    (*valueptr) += occurrences;

  d->num_of_occurrences += occurrences;
}

unsigned long long gt_disc_distri_get(const GtDiscDistri *d, unsigned long key)
{
  unsigned long long *valueptr;
  assert(d);
  if (!d->hashdist || !(valueptr = ul_ull_gt_hashmap_get(d->hashdist, key)))
    return 0;
  return *valueptr;
}

void gt_disc_distri_show(const GtDiscDistri *d)
{
  assert(d);
  gt_disc_distri_show_generic(d, NULL);
}

typedef struct {
  double cumulative_probability;
  unsigned long long num_of_occurrences;
  GtGenFile *genfile;
} ShowValueInfo;

static enum iterator_op
showvalue(unsigned long key, unsigned long long occurrences,
          void *data, GT_UNUSED GtError *err)
{
  double probability;
  ShowValueInfo *info;

  gt_error_check(err);
  assert(data && occurrences);
  info = (ShowValueInfo*) data;

  probability = (double) occurrences / info->num_of_occurrences;
  info->cumulative_probability += probability;
  gt_genfile_xprintf(info->genfile, "%lu: %llu (prob=%.4f,cumulative=%.4f)\n",
                  key, occurrences, probability, info->cumulative_probability);
  return CONTINUE_ITERATION;
}

void gt_disc_distri_show_generic(const GtDiscDistri *d, GtGenFile *genfile)
{
  ShowValueInfo showvalueinfo;
  int rval;

  assert(d);

  if (d->hashdist) {
    showvalueinfo.cumulative_probability = 0.0;
    showvalueinfo.num_of_occurrences = d->num_of_occurrences;
    showvalueinfo.genfile = genfile;
    rval = ul_ull_gt_hashmap_foreach_in_default_order(d->hashdist, showvalue,
                                                   &showvalueinfo, NULL);
    assert(!rval); /* showvalue() is sane */
  }
}

typedef struct {
  GtDiscDistriIterFunc func;
  void *data;
} ForeachInfo;

static enum iterator_op
foreach_iterfunc(unsigned long key, unsigned long long occurrences, void *data,
                 GT_UNUSED GtError *err)
{
  ForeachInfo *info;
  gt_error_check(err);
  assert(data);
  info = (ForeachInfo*) data;
  info->func(key, occurrences, info->data);
  return CONTINUE_ITERATION;
}

void gt_disc_distri_foreach(const GtDiscDistri *d, GtDiscDistriIterFunc func,
                        void *data)
{
  ForeachInfo info;
  int rval;
  assert(d);
  if (d->hashdist) {
    info.func = func;
    info.data = data;
    rval = ul_ull_gt_hashmap_foreach_in_default_order(d->hashdist,
                                                   foreach_iterfunc, &info,
                                                   NULL);
    assert(!rval); /* foreach_iterfunc() is sane */
  }
}

int gt_disc_distri_unit_test(GtError *err)
{
  GtDiscDistri *d;
  int had_err = 0;

  gt_error_check(err);

  d = gt_disc_distri_new();

  ensure(had_err, gt_disc_distri_get(d, 0) == 0);
  ensure(had_err, gt_disc_distri_get(d, 100) == 0);
  if (!had_err) {
    gt_disc_distri_add(d, 0);
    gt_disc_distri_add_multi(d, 100, 256);
  }
  ensure(had_err, gt_disc_distri_get(d, 0) == 1);
  ensure(had_err, gt_disc_distri_get(d, 100) == 256);

  gt_disc_distri_delete(d);

  return had_err;
}

void gt_disc_distri_delete(GtDiscDistri *d)
{
  if (!d) return;
  gt_hashtable_delete(d->hashdist);
  gt_free(d);
}
