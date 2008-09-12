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

#include <assert.h>
#include "core/hashmap.h"
#include "core/undef.h"
#include "core/unused_api.h"
#include "extended/mergefeat_visitor.h"
#include "extended/node_visitor_rep.h"

struct MergefeatVisitor {
  const GtNodeVisitor parent_instance;
  GtGenomeNode *current_tree;
  Hashmap *hm; /* type -> previous node */
  GtArray *nodes_to_remove;
};

#define mergefeat_visitor_cast(GV)\
        gt_node_visitor_cast(mergefeat_visitor_class(), GV)

static void mergefeat_visitor_free(GtNodeVisitor *gv)
{
  MergefeatVisitor *mergefeat_visitor = mergefeat_visitor_cast(gv);
  assert(mergefeat_visitor);
  hashmap_delete(mergefeat_visitor->hm);
  gt_array_delete(mergefeat_visitor->nodes_to_remove);
}

static int mergefeat_in_children(GtGenomeNode *gn, void *data,
                                 GT_UNUSED GtError *err)
{
  MergefeatVisitor *v = (MergefeatVisitor*) data;
  GtFeatureNode *previous_feature, *current_feature;
  GtRange previous_range, current_range;
  gt_error_check(err);
  current_feature = gt_genome_node_cast(gt_feature_node_class(), gn);
  assert(current_feature);
  if ((previous_feature =
        hashmap_get(v->hm, gt_feature_node_get_type(current_feature)))) {
    /* previous feature found -> check if merging is necessary */
    assert(gt_feature_node_get_type(previous_feature) ==
           gt_feature_node_get_type(current_feature));
    previous_range = gt_genome_node_get_range((GtGenomeNode*)
                                              previous_feature);
    current_range = gt_genome_node_get_range((GtGenomeNode*) current_feature);
    assert(gt_range_compare(previous_range, current_range) <= 0); /* sorted */
    if (previous_range.end + 1 == current_range.start) {
      /* merge nodes */
      gt_feature_node_set_end(previous_feature, current_range.end);
      /* XXX: compute average score ? */
      gt_feature_node_unset_score(previous_feature);
      assert(!gt_genome_node_number_of_children((GtGenomeNode*)
                                                current_feature));
      gt_array_add(v->nodes_to_remove, current_feature);
    }
    /* remove previous feature */
    hashmap_remove(v->hm, gt_feature_node_get_type(previous_feature));
  }
  /* add current feature */
  hashmap_add(v->hm, (char*) gt_feature_node_get_type(current_feature),
              current_feature);
  return 0;
}

static int mergefeat_if_necessary(GtGenomeNode *gn, void *data, GtError *err)
{
  MergefeatVisitor *v = (MergefeatVisitor*) data;
  GtFeatureNode *gf;
  gt_error_check(err);
  gf = gt_genome_node_cast(gt_feature_node_class(), gn);
  assert(gf);
  v->current_tree = gn;
  hashmap_reset(v->hm);
  return gt_genome_node_traverse_direct_children(gn, v, mergefeat_in_children,
                                              err);
}

static int mergefeat_visitor_genome_feature(GtNodeVisitor *gv,
                                            GtFeatureNode *gf, GtError *err)
{
  MergefeatVisitor *v;
  GtGenomeNode *leaf;
  unsigned long i;
  int had_err = 0;
  gt_error_check(err);
  v = mergefeat_visitor_cast(gv);
  gt_array_reset(v->nodes_to_remove);
  had_err = gt_genome_node_traverse_children((GtGenomeNode*) gf, v,
                                          mergefeat_if_necessary, false, err);
  if (!had_err) {
    for (i = 0; i < gt_array_size(v->nodes_to_remove); i++) {
      leaf = *(GtGenomeNode**) gt_array_get(v->nodes_to_remove, i);
      gt_genome_node_remove_leaf((GtGenomeNode*) gf, leaf);
      gt_genome_node_delete(leaf);
    }
  }
  return had_err;
}

const GtNodeVisitorClass* mergefeat_visitor_class()
{
  static const GtNodeVisitorClass gvc = { sizeof (MergefeatVisitor),
                                          mergefeat_visitor_free,
                                          NULL,
                                          mergefeat_visitor_genome_feature,
                                          NULL,
                                          NULL };
  return &gvc;
}

GtNodeVisitor* mergefeat_visitor_new(void)
{
  GtNodeVisitor *gv = gt_node_visitor_create(mergefeat_visitor_class());
  MergefeatVisitor *mergefeat_visitor = mergefeat_visitor_cast(gv);
  mergefeat_visitor->hm = hashmap_new(HASH_STRING, NULL, NULL);
  mergefeat_visitor->nodes_to_remove = gt_array_new(sizeof (GtGenomeNode*));
  return gv;
}
