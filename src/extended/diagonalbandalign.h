/*
  Copyright (c) 2015 Annika <annika.seidel@studium.uni-hamburg.de>
  Copyright (c) 2015 Center for Bioinformatics, University of Hamburg

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

#ifndef DIAGONALBANDALIGN_H
#define DIAGONALBANDALIGN_H
#include "core/error.h"
#include "core/types_api.h"
#include "core/unused_api.h"
#include "extended/alignment.h"

typedef struct {
  GtUword lastcpoint, currentrowindex;
  int edge;
} Diagentry;

void gt_checkdiagonalbandalign(GT_UNUSED bool forward,
                                const GtUchar *useq,
                                GtUword ulen,
                                const GtUchar *vseq,
                                GtUword vlen);

/* creating alignment with diagonalband in linear space O(n) */
void gt_computediagonalbandalign(GtAlignment *align,
                                 const GtUchar *useq,
                                 GtUword ustart, GtUword ulen,
                                 const GtUchar *vseq,
                                 GtUword vstart, GtUword vlen,
                                 GtWord left_dist,
                                 GtWord right_dist,
                                 GtUword matchcost,
                                 GtUword mismatchcost,
                                 GtUword gapcost);

/* creating alignment with diagonalband in square space O(n²) */
GtUword diagonalbandalignment_in_square_space(GtAlignment *align,
                                              const GtUchar *useq,
                                              GtUword ustart,
                                              GtUword ulen,
                                              const GtUchar *vseq,
                                              GtUword vstart,
                                              GtUword vlen,
                                              GtWord left_dist,
                                              GtWord right_dist,
                                              GtUword matchcost,
                                              GtUword mismatchcost,
                                              GtUword gapcost);

#endif
