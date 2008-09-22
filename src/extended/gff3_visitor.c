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

#include <stdlib.h>
#include <string.h>
#include "core/assert.h"
#include "core/fasta.h"
#include "core/hashmap.h"
#include "core/ma.h"
#include "core/unused_api.h"
#include "core/string_distri.h"
#include "extended/genome_node.h"
#include "extended/gff3_output.h"
#include "extended/gff3_parser.h"
#include "extended/gff3_visitor.h"
#include "extended/node_visitor_rep.h"

struct GFF3Visitor {
  const GtNodeVisitor parent_instance;
  bool version_string_shown,
       fasta_directive_shown;
  StringDistri *id_counter;
  GtHashmap *gt_feature_node_to_id_array,
            *gt_feature_node_to_unique_id_str;
  unsigned long fasta_width;
  GtGenFile *outfp;
};

typedef struct {
  GtHashmap *gt_feature_node_to_id_array;
  const char *id;
} Add_id_info;

typedef struct {
  bool *attribute_shown;
  GtGenFile *outfp;
} ShowAttributeInfo;

#define gff3_visitor_cast(GV)\
        gt_node_visitor_cast(gff3_visitor_class(), GV)

static void gff3_version_string(GtNodeVisitor *gv)
{
  GFF3Visitor *gff3_visitor = gff3_visitor_cast(gv);
  assert(gff3_visitor);
  if (!gff3_visitor->version_string_shown) {
    gt_genfile_xprintf(gff3_visitor->outfp, "%s   %u\n", GFF_VERSION_PREFIX,
                    GFF_VERSION);
    gff3_visitor->version_string_shown = true;
  }
}

static void gff3_visitor_free(GtNodeVisitor *gv)
{
  GFF3Visitor *gff3_visitor = gff3_visitor_cast(gv);
  assert(gff3_visitor);
  string_distri_delete(gff3_visitor->id_counter);
  gt_hashmap_delete(gff3_visitor->gt_feature_node_to_id_array);
  gt_hashmap_delete(gff3_visitor->gt_feature_node_to_unique_id_str);
}

static int gff3_visitor_comment_node(GtNodeVisitor *gv, GtCommentNode *cn,
                                     GT_UNUSED GtError *err)
{
  GFF3Visitor *gff3_visitor;
  gt_error_check(err);
  gff3_visitor = gff3_visitor_cast(gv);
  assert(gv && cn);
  gff3_version_string(gv);
  gt_genfile_xprintf(gff3_visitor->outfp, "#%s\n",
                     gt_comment_node_get_comment(cn));
  return 0;
}

static int add_id(GtGenomeNode *gn, void *data, GT_UNUSED GtError *err)
{
  Add_id_info *info = (Add_id_info*) data;
  GtArray *parent_features = NULL;
  gt_error_check(err);
  assert(gn && info && info->gt_feature_node_to_id_array && info->id);
  parent_features = gt_hashmap_get(info->gt_feature_node_to_id_array, gn);
  if (!parent_features) {
    parent_features = gt_array_new(sizeof (char*));
    gt_hashmap_add(info->gt_feature_node_to_id_array, gn, parent_features);
  }
  gt_array_add(parent_features, info->id);
  return 0;
}

static void show_attribute(const char *attr_name, const char *attr_value,
                           void *data)
{
  ShowAttributeInfo *info = (ShowAttributeInfo*) data;
  assert(attr_name && attr_value && info);
  if (strcmp(attr_name, ID_STRING) && strcmp(attr_name, PARENT_STRING)) {
    if (*info->attribute_shown)
      gt_genfile_xfputc(';', info->outfp);
    else
      *info->attribute_shown = true;
    gt_genfile_xprintf(info->outfp, "%s=%s", attr_name, attr_value);
  }
}

static int gff3_show_genome_feature(GtGenomeNode *gn, void *data,
                                    GT_UNUSED GtError *err)
{
  bool part_shown = false;
  GFF3Visitor *gff3_visitor = (GFF3Visitor*) data;
  GtFeatureNode *gf = (GtFeatureNode*) gn;
  GtArray *parent_features = NULL;
  ShowAttributeInfo info;
  unsigned long i;
  GtStr *id;

  gt_error_check(err);
  assert(gn && gf && gff3_visitor);

  /* output leading part */
  gt_gff3_output_leading(gf, gff3_visitor->outfp);

  /* show unique id part of attributes */
  if ((id = gt_hashmap_get(gff3_visitor->gt_feature_node_to_unique_id_str,
                        gn))) {
    gt_genfile_xprintf(gff3_visitor->outfp, "%s=%s", ID_STRING, gt_str_get(id));
    part_shown = true;
  }

  /* show parent part of attributes */
  parent_features = gt_hashmap_get(gff3_visitor->gt_feature_node_to_id_array,
                                  gn);
  if (gt_array_size(parent_features)) {
    if (part_shown)
      gt_genfile_xfputc(';', gff3_visitor->outfp);
    gt_genfile_xprintf(gff3_visitor->outfp, "%s=", PARENT_STRING);
    for (i = 0; i < gt_array_size(parent_features); i++) {
      if (i)
        gt_genfile_xfputc(',', gff3_visitor->outfp);
      gt_genfile_xprintf(gff3_visitor->outfp, "%s",
                      *(char**) gt_array_get(parent_features, i));
    }
    part_shown = true;
  }

  /* show missing part of attributes */
  info.attribute_shown = &part_shown;
  info.outfp = gff3_visitor->outfp;
  gt_feature_node_foreach_attribute(gf, show_attribute, &info);

  /* show dot if no attributes have been shown */
  if (!part_shown)
    gt_genfile_xfputc('.', gff3_visitor->outfp);

  /* show terminal newline */
  gt_genfile_xfputc('\n', gff3_visitor->outfp);

  return 0;
}

static GtStr* create_unique_id(GFF3Visitor *gff3_visitor, GtFeatureNode *gf)
{
  const char *type;
  GtStr *id;
  assert(gff3_visitor && gf);
  type = gt_feature_node_get_type(gf);

  /* increase id counter */
  string_distri_add(gff3_visitor->id_counter, type);

  /* build id string */
  id = gt_str_new_cstr(type);
  gt_str_append_ulong(id, string_distri_get(gff3_visitor->id_counter, type));

  /* store (unique) id */
  gt_hashmap_add(gff3_visitor->gt_feature_node_to_unique_id_str, gf, id);

  return id;
}

static int store_ids(GtGenomeNode *gn, void *data, GtError *err)
{
  GFF3Visitor *gff3_visitor = (GFF3Visitor*) data;
  GtFeatureNode *gf = (GtFeatureNode*) gn;
  Add_id_info add_id_info;
  int had_err = 0;
  GtStr *id;

  gt_error_check(err);
  assert(gn && gf && gff3_visitor);

  if (gt_genome_node_has_children(gn) || gt_feature_node_is_multi(gf)) {
    if (gt_feature_node_is_multi(gf)) {
      id = gt_hashmap_get(gff3_visitor->gt_feature_node_to_unique_id_str,
                       gt_feature_node_get_multi_representative(gf));
      if (!id) { /* the representative does not have its own id */
        id = create_unique_id(gff3_visitor,
                              gt_feature_node_get_multi_representative(gf));
      }
      if (gt_feature_node_get_multi_representative(gf) != gf) {
        gt_hashmap_add(gff3_visitor->gt_feature_node_to_unique_id_str, gf,
                    gt_str_ref(id));
      }
    }
    else
      id = create_unique_id(gff3_visitor, gf);

    /* for each child -> store the parent feature in the hash map */
    add_id_info.gt_feature_node_to_id_array =
      gff3_visitor->gt_feature_node_to_id_array,
    add_id_info.id = gt_str_get(id);
    had_err = gt_genome_node_traverse_direct_children(gn, &add_id_info, add_id,
                                                   err);
  }
  return had_err;
}

static int gff3_visitor_genome_feature(GtNodeVisitor *gv, GtFeatureNode *gf,
                                       GtError *err)
{
  GFF3Visitor *gff3_visitor;
  int had_err;
  gt_error_check(err);
  gff3_visitor = gff3_visitor_cast(gv);

  gff3_version_string(gv);

  had_err = gt_genome_node_traverse_children((GtGenomeNode*) gf, gff3_visitor,
                                          store_ids, true, err);
  if (!had_err) {
    if (gt_genome_node_is_tree((GtGenomeNode*) gf)) {
      had_err = gt_genome_node_traverse_children((GtGenomeNode*) gf,
                                                 gff3_visitor,
                                                 gff3_show_genome_feature, true,
                                                 err);
    }
    else {
      /* got a DAG -> traverse bin breadth first fashion to make sure that the
         'Parent' attributes are shown in correct order */
      had_err = gt_genome_node_traverse_children_breadth((GtGenomeNode*) gf,
                                                      gff3_visitor,
                                                      gff3_show_genome_feature,
                                                      true, err);
    }
  }

  /* reset hashmaps */
  gt_hashmap_reset(gff3_visitor->gt_feature_node_to_id_array);
  gt_hashmap_reset(gff3_visitor->gt_feature_node_to_unique_id_str);

  /* show terminator, if the feature has children (otherwise it is clear that
     the feature is complete, because no ID attribute has been shown) */
  if (gt_genome_node_has_children((GtGenomeNode*) gf))
    gt_genfile_xprintf(gff3_visitor->outfp, "%s\n", GFF_TERMINATOR);

  return had_err;
}

static int gff3_visitor_region_node(GtNodeVisitor *gv, GtRegionNode *rn,
                                    GT_UNUSED GtError *err)
{
  GFF3Visitor *gff3_visitor;
  gt_error_check(err);
  gff3_visitor = gff3_visitor_cast(gv);
  gt_assert(gv && rn);
  gff3_version_string(gv);
  gt_genfile_xprintf(gff3_visitor->outfp, "%s   %s %lu %lu\n",
                     GFF_SEQUENCE_REGION,
                     gt_str_get(gt_genome_node_get_seqid((GtGenomeNode*) rn)),
                     gt_genome_node_get_start((GtGenomeNode*) rn),
                     gt_genome_node_get_end((GtGenomeNode*) rn));
  return 0;
}

static int gff3_visitor_sequence_node(GtNodeVisitor *gv, GtSequenceNode *sn,
                                      GT_UNUSED GtError *err)
{
  GFF3Visitor *gff3_visitor;
  gt_error_check(err);
  gff3_visitor = gff3_visitor_cast(gv);
  assert(gv && sn);
  if (!gff3_visitor->fasta_directive_shown) {
    gt_genfile_xprintf(gff3_visitor->outfp, "%s\n", GFF_FASTA_DIRECTIVE);
    gff3_visitor->fasta_directive_shown = true;
  }
  gt_fasta_show_entry_generic(gt_sequence_node_get_description(sn),
                              gt_sequence_node_get_sequence(sn),
                              gt_sequence_node_get_sequence_length(sn),
                              gff3_visitor->fasta_width, gff3_visitor->outfp);
  return 0;
}

const GtNodeVisitorClass* gff3_visitor_class()
{
  static const GtNodeVisitorClass *gvc = NULL;
  if (!gvc) {
    gvc = gt_node_visitor_class_new(sizeof (GFF3Visitor),
                                    gff3_visitor_free,
                                    gff3_visitor_comment_node,
                                    gff3_visitor_genome_feature,
                                    gff3_visitor_region_node,
                                    gff3_visitor_sequence_node);
  }
  return gvc;
}

GtNodeVisitor* gff3_visitor_new(GtGenFile *outfp)
{
  GtNodeVisitor *gv = gt_node_visitor_create(gff3_visitor_class());
  GFF3Visitor *gff3_visitor = gff3_visitor_cast(gv);
  gff3_visitor->version_string_shown = false;
  gff3_visitor->fasta_directive_shown = false;
  gff3_visitor->id_counter = string_distri_new();
  gff3_visitor->gt_feature_node_to_id_array = gt_hashmap_new(
    HASH_DIRECT, NULL, (GtFree) gt_array_delete);
  gff3_visitor->gt_feature_node_to_unique_id_str = gt_hashmap_new(
    HASH_DIRECT, NULL, (GtFree) gt_str_delete);
  gff3_visitor->fasta_width = 0;
  gff3_visitor->outfp = outfp;
  return gv;
}

void gff3_visitor_set_fasta_width(GtNodeVisitor *gv, unsigned long fasta_width)
{
  GFF3Visitor *gff3_visitor = gff3_visitor_cast(gv);
  assert(gv);
  gff3_visitor->fasta_width = fasta_width;
}
