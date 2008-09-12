/*
  Copyright (c) 2007-2008 Gordon Gremme <gremme@zbh.uni-hamburg.de>
  Copyright (c) 2007-2008 Center for Bioinformatics, University of Hamburg

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
#include "core/undef.h"
#include "core/unused_api.h"
#include "extended/add_introns_visitor.h"
#include "extended/genome_visitor_rep.h"

struct AddIntronsVisitor {
  const GenomeVisitor parent_instance;
  GtFeatureNode *parent_feature,
                *previous_exon_feature;
};

#define add_introns_visitor_cast(GV)\
        genome_visitor_cast(add_introns_visitor_class(), GV)

static int add_introns_in_children(GtGenomeNode *gn, void *data,
                                   GT_UNUSED GtError *err)
{
  AddIntronsVisitor *v = (AddIntronsVisitor*) data;
  GtFeatureNode *current_feature;
  GtGenomeNode *intron_node;
  GtRange previous_range, current_range, intron_range;
  GtStrand previous_strand, current_strand, intron_strand;
  GtStr *parent_seqid;
  gt_error_check(err);
  current_feature = gt_genome_node_cast(gt_feature_node_class(), gn);
  assert(current_feature);
  if (gt_feature_node_has_type(current_feature, gft_exon)) {
    if (v->previous_exon_feature) {
      /* determine intron range */
      previous_range = gt_genome_node_get_range((GtGenomeNode*)
                                             v->previous_exon_feature);
      current_range = gt_genome_node_get_range(gn);
      assert(previous_range.end < current_range.start);
      intron_range.start = previous_range.end + 1;
      intron_range.end = current_range.start - 1;

      /* determine intron strand */
      previous_strand = gt_feature_node_get_strand(v->previous_exon_feature);
      current_strand = gt_feature_node_get_strand(current_feature);
      assert(previous_strand == current_strand);
      intron_strand = previous_strand;

      /* determine sequence id */
      parent_seqid =
        gt_genome_node_get_seqid((GtGenomeNode*) v->parent_feature);
      assert(!gt_str_cmp(parent_seqid,
             gt_genome_node_get_seqid((GtGenomeNode*)
                                      v->previous_exon_feature)));
      assert(!gt_str_cmp(parent_seqid,
             gt_genome_node_get_seqid((GtGenomeNode*) current_feature)));

      /* create intron */
      intron_node = gt_feature_node_new(parent_seqid, gft_intron,
                                          intron_range.start, intron_range.end,
                                          intron_strand);
      gt_genome_node_add_child((GtGenomeNode*) v->parent_feature, intron_node);
    }
    v->previous_exon_feature = current_feature;
  }
  return 0;
}

static int add_introns_if_necessary(GtGenomeNode *gn, void *data,
                                    GtError *err)
{
  AddIntronsVisitor *v = (AddIntronsVisitor*) data;
  GtFeatureNode *gf;
  gt_error_check(err);
  gf = gt_genome_node_cast(gt_feature_node_class(), gn);
  assert(gf);
  v->parent_feature = gf;
  v->previous_exon_feature = NULL;
  return gt_genome_node_traverse_direct_children(gn, v, add_introns_in_children,
                                              err);
}

static int add_introns_visitor_genome_feature(GenomeVisitor *gv,
                                              GtFeatureNode *gf,
                                              GtError *err)
{
  AddIntronsVisitor *v;
  gt_error_check(err);
  v = add_introns_visitor_cast(gv);
  return gt_genome_node_traverse_children((GtGenomeNode*) gf, v,
                                       add_introns_if_necessary, false, err);
}

const GenomeVisitorClass* add_introns_visitor_class()
{
  static const GenomeVisitorClass gvc = { sizeof (AddIntronsVisitor),
                                          NULL,
                                          NULL,
                                          add_introns_visitor_genome_feature,
                                          NULL,
                                          NULL };
  return &gvc;
}

GenomeVisitor* add_introns_visitor_new(void)
{
  return genome_visitor_create(add_introns_visitor_class());
}
