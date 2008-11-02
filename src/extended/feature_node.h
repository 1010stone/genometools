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

#ifndef FEATURE_NODE_H
#define FEATURE_NODE_H

#include "core/bittab.h"
#include "core/range.h"
#include "core/phase.h"
#include "core/strand_api.h"
#include "core/str_array.h"
#include "extended/feature_node_api.h"
#include "extended/feature_type.h"
#include "extended/transcript_feature_type.h"

typedef void (*AttributeIterFunc)(const char *attr_name, const char *attr_value,
                                  void *data);
typedef int (*GtGenomeNodeTraverseFunc)(GtGenomeNode*, void*, GtError*);

const GtGenomeNodeClass* gt_feature_node_class(void);
GtGenomeNode*  gt_feature_node_new_pseudo(GtFeatureNode*);
/* Return the ``standard gene'' (mainly for testing purposes). */
GtGenomeNode*  gt_feature_node_new_standard_gene(void);
const char*    gt_feature_node_get_source(GtFeatureNode*);
const char*    gt_feature_node_get_attribute(GtFeatureNode *feature_node,
                                             const char *attr_name);
/* Return a GtStrArray containing the used attribute names. */
GtStrArray*    gt_feature_node_get_attribute_list(GtFeatureNode*);
const char*    gt_feature_node_get_type(GtFeatureNode*);
bool           gt_feature_node_has_type(GtFeatureNode*, const char*);
bool           gt_feature_node_score_is_defined(const GtFeatureNode*);
bool           gt_feature_node_is_multi(const GtFeatureNode*);
bool           gt_feature_node_is_pseudo(const GtFeatureNode*);
void           gt_feature_node_make_multi_representative(GtFeatureNode*);
void           gt_feature_node_set_multi_representative(GtFeatureNode*,
                                                        GtFeatureNode*);
GtFeatureNode* gt_feature_node_get_multi_representative(GtFeatureNode*);
float                 gt_feature_node_get_score(GtFeatureNode*);
GtStrand       gt_feature_node_get_strand(GtFeatureNode*);
Phase          gt_feature_node_get_phase(GtFeatureNode*);
void           gt_feature_node_get_exons(GtFeatureNode*,
                                         GtArray *exon_features);
void           gt_feature_node_determine_transcripttypes(GtFeatureNode*);
GtTranscriptFeatureType
               gt_feature_node_get_transcriptfeaturetype(GtFeatureNode*);
void           gt_feature_node_set_source(GtFeatureNode*, GtStr *source);
void           gt_feature_node_set_phase(GtGenomeNode*, Phase);
void           gt_feature_node_set_end(GtFeatureNode*, unsigned long);
void           gt_feature_node_set_score(GtFeatureNode*, float);
void           gt_feature_node_unset_score(GtFeatureNode*);
void           gt_feature_node_add_attribute(GtFeatureNode*,
                                             const char *attr_name,
                                             const char *attr_value);
void           gt_feature_node_foreach_attribute(GtFeatureNode*,
                                                 AttributeIterFunc, void *data);
bool           gt_feature_node_has_CDS(const GtFeatureNode*);
bool           gt_feature_node_has_splice_site(const GtFeatureNode*);
double         gt_feature_node_average_splice_site_prob(const GtFeatureNode*);
/* Returns true, if the given features have the same seqid, feature type, range,
   strand, and phase. */
bool           gt_genome_features_are_similar(GtFeatureNode*, GtFeatureNode*);
int            gt_feature_node_unit_test(GtError*);

GtGenomeNode*  gt_genome_node_rec_ref(GtGenomeNode*);

/* perform depth first traversal of the given genome node */
int            gt_genome_node_traverse_children(GtGenomeNode*, void*,
                                                GtGenomeNodeTraverseFunc,
                                                bool traverse_only_once,
                                                GtError*);
/* perform breadth first traversal of the given genome node  */
int            gt_genome_node_traverse_children_breadth(GtGenomeNode*, void*,
                                                       GtGenomeNodeTraverseFunc,
                                                        bool traverse_only_once,
                                                        GtError*);
int            gt_genome_node_traverse_direct_children(GtGenomeNode*, void*,
                                                       GtGenomeNodeTraverseFunc,
                                                       GtError*);
unsigned long  gt_genome_node_number_of_children(const GtGenomeNode*);
/* does not free the leaf, do not use during traversal! */
void           gt_genome_node_remove_leaf(GtGenomeNode *tree,
                                          GtGenomeNode *leafn);
void           gt_genome_node_mark(GtGenomeNode*);
/* returns true if the (top-level) node is marked */
bool           gt_genome_node_is_marked(const GtGenomeNode*);

/* returns true if the given node graph contains a marked node */
bool           gt_genome_node_contains_marked(GtGenomeNode*);
bool           gt_genome_node_has_children(GtGenomeNode*);
bool           gt_genome_node_direct_children_do_not_overlap(GtGenomeNode*);
/* returns true if all direct childred of <parent> with the same type (s.t.) as
   <child> do not overlap */
bool           gt_genome_node_direct_children_do_not_overlap_st(GtGenomeNode
                                                                *parent,
                                                                GtGenomeNode
                                                                *child);
bool           gt_genome_node_is_tree(GtGenomeNode*);
/* returns true if the genome node overlaps at least one of the nodes given in
   the array. O(gt_array_size) */
bool           gt_genome_node_overlaps_nodes(GtGenomeNode*, GtArray*);
/* similar interface to gt_genome_node_overlaps_nodes(). Aditionally, if a
   bittab is given (which must have the same size as the array), the bits
   corresponding to overlapped nodes are marked (i.e., set) */
bool           gt_genome_node_overlaps_nodes_mark(GtGenomeNode*, GtArray*,
                                                  GtBittab*);

#define gt_feature_node_cast(genome_node) \
        gt_genome_node_cast(gt_feature_node_class(), genome_node)

#define gt_feature_node_try_cast(genome_node) \
        gt_genome_node_try_cast(gt_feature_node_class(), genome_node)

#endif
