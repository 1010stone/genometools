/*
  Copyright (c) 2006-2008 Gordon Gremme <gremme@zbh.uni-hamburg.de>
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

#include <stdarg.h>
#include "core/assert_api.h"
#include "core/hashtable.h"
#include "core/ma.h"
#include "core/msort.h"
#include "core/queue.h"
#include "core/unused_api.h"
#include "extended/genome_node_rep.h"
#include "extended/region_node_api.h"

static int compare_gt_genome_node_type(GtGenomeNode *gn_a, GtGenomeNode *gn_b)
{
  void *sr_a, *sr_b, *sn_a, *sn_b;

  /* region nodes first */
  sr_a = gt_region_node_try_cast(gn_a);
  sr_b = gt_region_node_try_cast(gn_b);

  if (sr_a && !sr_b)
    return -1;
  if (!sr_a && sr_b)
    return 1;

  /* sequence nodes last */
  sn_a = gt_sequence_node_try_cast(gn_a);
  sn_b = gt_sequence_node_try_cast(gn_b);

  if (sn_a && !sn_b)
    return 1;
  if (!sn_a && sn_b)
    return -1;

  return 0;
}

int gt_genome_node_cmp(GtGenomeNode *gn_a, GtGenomeNode *gn_b)
{
  GtRange range_a, range_b;
  int rval;
  gt_assert(gn_a && gn_b);
  /* ensure that region nodes come first and sequence nodes come last,
     otherwise we don't get a valid GFF3 stream */
  if ((rval = compare_gt_genome_node_type(gn_a, gn_b)))
    return rval;

  if ((rval = gt_str_cmp(gt_genome_node_get_idstr(gn_a),
                         gt_genome_node_get_idstr(gn_b)))) {
    return rval;
  }
  range_a = gt_genome_node_get_range(gn_a),
  range_b = gt_genome_node_get_range(gn_b);
  return gt_range_compare(&range_a, &range_b);
}

static int compare_genome_nodes_with_delta(GtGenomeNode *gn_a,
                                           GtGenomeNode *gn_b,
                                           unsigned long delta)
{
  GtRange range_a, range_b;
  int rval;
  gt_assert(gn_a && gn_b);
  /* ensure that sequence regions come first, otherwise we don't get a valid
     gff3 stream */
  if ((rval = compare_gt_genome_node_type(gn_a, gn_b)))
    return rval;

  if ((rval = gt_str_cmp(gt_genome_node_get_idstr(gn_a),
                      gt_genome_node_get_idstr(gn_b)))) {
    return rval;
  }
  range_a = gt_genome_node_get_range(gn_a);
  range_b = gt_genome_node_get_range(gn_b);
  return gt_range_compare_with_delta(&range_a, &range_b, delta);
}

GtGenomeNode* gt_genome_node_create(const GtGenomeNodeClass *gnc)
{
  GtGenomeNode *gn;
  gt_assert(gnc && gnc->size);
  gn                  = gt_malloc(gnc->size);
  gn->c_class         = gnc;
  gn->filename        = NULL; /* means the node is generated */
  gn->line_number     = 0;
  gn->reference_count = 0;
  return gn;
}

void gt_genome_node_set_origin(GtGenomeNode *gn,
                            GtStr *filename, unsigned int line_number)
{
  gt_assert(gn && filename && line_number);
  gt_str_delete(gn->filename);
  gn->filename = gt_str_ref(filename);
  gn->line_number =line_number;
}

void* gt_genome_node_cast(GT_UNUSED const GtGenomeNodeClass *gnc,
                          GtGenomeNode *gn)
{
  gt_assert(gnc && gn && gn->c_class == gnc);
  return gn;
}

void* gt_genome_node_try_cast(const GtGenomeNodeClass *gnc, GtGenomeNode *gn)
{
  gt_assert(gnc && gn);
  if (gn->c_class == gnc)
    return gn;
  return NULL;
}

GtGenomeNode* gt_genome_node_ref(GtGenomeNode *gn)
{
  gt_assert(gn);
  gn->reference_count++;
  return gn;
}

const char* gt_genome_node_get_filename(const GtGenomeNode *gn)
{
  gt_assert(gn);
  if (gn->filename)
    return gt_str_get(gn->filename);
  return "generated";
}

unsigned int gt_genome_node_get_line_number(const GtGenomeNode *gn)
{
  gt_assert(gn);
  return gn->line_number;
}

GtStr* gt_genome_node_get_seqid(GtGenomeNode *gn)
{
  gt_assert(gn && gn->c_class);
  if (gn->c_class->get_seqid)
    return gn->c_class->get_seqid(gn);
  return NULL;
}

GtStr* gt_genome_node_get_idstr(GtGenomeNode *gn)
{
  gt_assert(gn && gn->c_class && gn->c_class->get_idstr);
  return gn->c_class->get_idstr(gn);
}

unsigned long gt_genome_node_get_start(GtGenomeNode *gn)
{
  return gt_genome_node_get_range(gn).start;
}

unsigned long gt_genome_node_get_end(GtGenomeNode *gn)
{
  return gt_genome_node_get_range(gn).end;
}

GtRange gt_genome_node_get_range(GtGenomeNode *gn)
{
  gt_assert(gn && gn->c_class && gn->c_class->get_range);
  return gn->c_class->get_range(gn);
}

void gt_genome_node_set_range(GtGenomeNode *gn, const GtRange *range)
{
  gt_assert(gn && gn->c_class && gn->c_class->set_range);
  gt_assert(range->start <= range->end);
  gn->c_class->set_range(gn, range);
}

void gt_genome_node_change_seqid(GtGenomeNode *gn, GtStr *seqid)
{
  gt_assert(gn && gn->c_class && gn->c_class->change_seqid && seqid);
  gn->c_class->change_seqid(gn, seqid);
}

int gt_genome_node_accept(GtGenomeNode *gn, GtNodeVisitor *gv, GtError *err)
{
  gt_error_check(err);
  gt_assert(gn && gv && gn->c_class && gn->c_class->accept);
  return gn->c_class->accept(gn, gv, err);
}

int gt_genome_node_compare(GtGenomeNode **gn_a, GtGenomeNode **gn_b)
{
  return gt_genome_node_cmp(*gn_a, *gn_b);
}

int gt_genome_node_compare_with_data(GtGenomeNode **gn_a, GtGenomeNode **gn_b,
                                  GT_UNUSED void *unused)
{
  return gt_genome_node_cmp(*gn_a, *gn_b);
}

int gt_genome_node_compare_delta(GtGenomeNode **gn_a, GtGenomeNode **gn_b,
                              void *delta)
{
  unsigned long *deltaptr = delta;
  gt_assert(delta);
  return compare_genome_nodes_with_delta(*gn_a, *gn_b, *deltaptr);
}

void gt_genome_node_delete(GtGenomeNode *gn)
{
  if (!gn) return;
  if (gn->reference_count) {
    gn->reference_count--;
    return;
  }
  gt_assert(gn->c_class);
  if (gn->c_class->free)
    gn->c_class->free(gn);
  gt_str_delete(gn->filename);
  gt_free(gn);
}

void gt_genome_nodes_sort(GtArray *nodes)
{
  qsort(gt_array_get_space(nodes), gt_array_size(nodes),
        sizeof (GtGenomeNode*), (GtCompare) gt_genome_node_compare);
}

void gt_genome_nodes_sort_stable(GtArray *nodes)
{
  gt_msort(gt_array_get_space(nodes), gt_array_size(nodes),
           sizeof (GtGenomeNode*), (GtCompare) gt_genome_node_compare);
}

bool gt_genome_nodes_are_equal_region_nodes(GtGenomeNode *gn_a,
                                            GtGenomeNode *gn_b)
{
  void *sr_a, *sr_b;

  sr_a = gn_a ? gt_region_node_try_cast(gn_a) : NULL;
  sr_b = gn_b ? gt_region_node_try_cast(gn_b) : NULL;

  if (sr_a && sr_b && !gt_str_cmp(gt_genome_node_get_seqid(gn_a),
                                  gt_genome_node_get_seqid(gn_b))) {
    return true;
  }
  return false;
}

bool gt_genome_nodes_are_sorted(const GtArray *nodes)
{
  unsigned long i;
  gt_assert(nodes);
  for (i = 1; i < gt_array_size(nodes); i++) {
    if (gt_genome_node_compare(gt_array_get(nodes, i-1),
                               gt_array_get(nodes, i)) > 0) {
      return false;
    }
  }
  return true;
}
