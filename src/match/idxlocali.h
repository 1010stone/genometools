/*
 * Copyright (c) 2008 Z. Tang <tangzhihao0117@hotmail.com> Copyright (c)
 * 2008 Center for Bioinformatics, University of Hamburg

 * Permission to use, copy, modify, and distribute this software for any
 *  with or without fee is hereby granted, provided that the above copyright
 * notice and this permission notice appear in all copies.

 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *  REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * TY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL,
 * DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
 * CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef IDXLOCALI_H
#define IDXLOCALI_H

#include <stdbool.h>
#include "core/str_array.h"
#include "core/str.h"
#include "core/option.h"
#include "core/error_api.h"

typedef struct
{
  GtStrArray *queryfiles;
  GtStr *indexname;
  GtOption *refoptionesaindex,
           *refoptionpckindex;
  bool withesa,
       dosort;
  unsigned long threshold;
  long matchscore,
       mismatchscore,
       gapstart,
       gapextend;
} IdxlocaliOptions;

int runidxlocali(const IdxlocaliOptions *arguments,GtError *err);

#endif
