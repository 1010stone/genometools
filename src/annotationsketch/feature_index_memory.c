/*
  Copyright (c) 2007-2008 Sascha Steinbiss <ssteinbiss@stud.zbh.uni-hamburg.de>
  Copyright (c)      2008 Gordon Gremme <gremme@zbh.uni-hamburg.de>
  Copyright (c) 2007      Malte Mader <mmader@stud.zbh.uni-hamburg.de>
  Copyright (c) 2007      Chr. Schaerfer <cschaerfer@stud.zbh.uni-hamburg.de>
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

#include <string.h>
#include "annotationsketch/feature_index_memory.h"
#include "annotationsketch/feature_index_rep.h"
#include "annotationsketch/feature_index.h"
#include "annotationsketch/feature_stream.h"
#include "annotationsketch/feature_visitor.h"
#include "core/ensure.h"
#include "core/hashmap.h"
#include "core/interval_tree.h"
#include "core/ma.h"
#include "core/minmax.h"
#include "core/queue.h"
#include "core/range.h"
#include "core/undef.h"
#include "core/unused_api.h"
#include "extended/genome_node.h"
#include "extended/gff3_in_stream.h"

struct GtFeatureIndexMemory {
  const GtFeatureIndex parent_instance;
  Hashmap *regions;
  char *firstseqid;
  unsigned int nof_region_nodes,
               reference_count;
};

#define gt_feature_index_memory_cast(FI)\
        gt_feature_index_cast(gt_feature_index_memory_class(), FI)

typedef struct {
  GtIntervalTree *features;
  GtRegionNode *region;
  GtRange dyn_range;
} RegionInfo;

static void region_info_delete(RegionInfo *info)
{
  gt_interval_tree_delete(info->features);
  if (info->region)
    gt_genome_node_delete((GtGenomeNode*)info->region);
  gt_free(info);
}

void gt_feature_index_memory_add_region_node(GtFeatureIndex *gfi,
                                             GtRegionNode *rn)
{
  char *seqid;
  GtFeatureIndexMemory *fi;
  fi = gt_feature_index_memory_cast(gfi);
  RegionInfo *info;
  assert(fi && rn);
  seqid = gt_str_get(gt_genome_node_get_seqid((GtGenomeNode*) rn));
  if (!hashmap_get(fi->regions, seqid)) {
    info = gt_malloc(sizeof (RegionInfo));
    info->region = (GtRegionNode*) gt_genome_node_ref((GtGenomeNode*) rn);
    info->features = gt_interval_tree_new((GtFree)
                                          gt_genome_node_rec_delete);
    info->dyn_range.start = ~0UL;
    info->dyn_range.end   = 0;
    hashmap_add(fi->regions, seqid, info);
    if (fi->nof_region_nodes++ == 0)
      fi->firstseqid = seqid;
  }
}

void gt_feature_index_memory_add_feature_node(GtFeatureIndex *gfi,
                                              GtFeatureNode *gf)
{
  GtGenomeNode *gn;
  char* seqid;
  GtFeatureIndexMemory *fi;
  GtRange node_range;
  RegionInfo *info;

  assert(gfi && gf);

  fi = gt_feature_index_memory_cast(gfi);
  gn = gt_genome_node_rec_ref((GtGenomeNode*) gf);
  /* get information about seqid and range */
  node_range = gt_genome_node_get_range(gn);
  seqid = gt_str_get(gt_genome_node_get_seqid(gn));
  info = (RegionInfo*) hashmap_get(fi->regions, seqid);

  /* If the seqid was encountered for the first time, no sequence
     region nodes have been visited before. We therefore must create a new
     index entry and maintain our own GtRange. */
  if (!info)
  {
    info = gt_calloc(1, sizeof (RegionInfo));
    info->region = NULL;
    info->features = gt_interval_tree_new((GtFree)
                                          gt_genome_node_rec_delete);
    info->dyn_range.start = ~0UL;
    info->dyn_range.end   = 0;
    hashmap_add(fi->regions, seqid, info);
    if (fi->nof_region_nodes++ == 0)
      fi->firstseqid = seqid;
  }

  /* add node to the appropriate array in the hashtable */
  GtIntervalTreeNode *new_node = gt_interval_tree_node_new(gn,
                                                            node_range.start,
                                                            node_range.end);
  gt_interval_tree_insert(info->features, new_node);
  /* update dynamic range */
  info->dyn_range.start = MIN(info->dyn_range.start, node_range.start);
  info->dyn_range.end = MAX(info->dyn_range.end, node_range.end);
}

int gt_feature_index_memory_add_gff3file(GtFeatureIndex *gfi,
                                         const char *gff3file, GtError *err)
{
  GtNodeStream *gff3_in_stream;
  GtGenomeNode *gn;
  GtFeatureIndexMemory *fi;
  GtQueue *queue;
  int had_err = 0;
  gt_error_check(err);
  assert(gfi && gff3file);
  fi = gt_feature_index_memory_cast(gfi);
  queue = gt_queue_new();
  gff3_in_stream = gff3_in_stream_new_unsorted(1, &gff3file, false, false);
  while (!(had_err = gt_node_stream_next(gff3_in_stream, &gn, err)) && gn)
    gt_queue_add(queue, gn);
  if (!had_err) {
    GtNodeVisitor  *feature_visitor = feature_visitor_new(gfi);
    while (gt_queue_size(queue)) {
      gn = gt_queue_get(queue);
      had_err = gt_genome_node_accept(gn, feature_visitor, NULL);
      assert(!had_err); /* cannot happen */
    }
    gt_node_visitor_delete(feature_visitor);
  }
  gt_node_stream_delete(gff3_in_stream);
  while (gt_queue_size(queue))
    gt_genome_node_rec_delete(gt_queue_get(queue));
  gt_queue_delete(queue);
  return had_err;
}

static int collect_features_from_itree(GtIntervalTreeNode *node, void *data)
{
  GtArray *a = (GtArray*) data;
  GtGenomeNode *gn = (GtGenomeNode*) gt_interval_tree_node_get_data(node);
  gt_array_add(a, gn);
  return 0;
}

GtArray* gt_feature_index_memory_get_features_for_seqid(GtFeatureIndex *gfi,
                                                        const char *seqid)
{
  RegionInfo *ri;
  int had_err = 0;
  GtArray *a;
  GtFeatureIndexMemory *fi;
  assert(gfi && seqid);
  fi = gt_feature_index_memory_cast(gfi);
  a = gt_array_new(sizeof (GtFeatureNode*));
  ri = (RegionInfo*) hashmap_get(fi->regions, seqid);
  if (ri)
    had_err = gt_interval_tree_traverse(ri->features,
                                     collect_features_from_itree,
                                     a);
  assert(!had_err);   /* collect_features_from_itree() is sane */
  return a;
}

static int gt_genome_node_cmp_range_start(const void *v1, const void *v2)
{
  GtGenomeNode *n1, *n2;
  n1 = *(GtGenomeNode**) v1;
  n2 = *(GtGenomeNode**) v2;
  return gt_genome_node_compare(&n1, &n2);
}

int gt_feature_index_memory_get_features_for_range(GtFeatureIndex *gfi,
                                                   GtArray *results,
                                                   const char *seqid,
                                                   GtRange qry_range,
                                                   GtError *err)
{
  RegionInfo *ri;
  GtFeatureIndexMemory *fi;
  gt_error_check(err);
  assert(gfi && results);

  fi = gt_feature_index_memory_cast(gfi);
  ri = (RegionInfo*) hashmap_get(fi->regions, seqid);
  if (!ri) {
    gt_error_set(err, "feature index does not contain the given sequence id");
    return -1;
  }
  gt_interval_tree_find_all_overlapping(ri->features, qry_range.start,
                                     qry_range.end, results);
  gt_array_sort(results, gt_genome_node_cmp_range_start);
  return 0;
}

const char* gt_feature_index_memory_get_first_seqid(const GtFeatureIndex *gfi)
{
  GtFeatureIndexMemory *fi;
  assert(gfi);

  fi = gt_feature_index_memory_cast((GtFeatureIndex*) gfi);
  return fi->firstseqid;
}

static int store_seqid(void *key, GT_UNUSED void *value, void *data,
                       GT_UNUSED GtError *err)
{
  GtStrArray *seqids = (GtStrArray*) data;
  const char *seqid = (const char*) key;
  assert(seqids && seqid);
  gt_strarray_add_cstr(seqids, seqid);
  return 0;
}

GtStrArray* gt_feature_index_memory_get_seqids(const GtFeatureIndex *gfi)
{
  GtStrArray* seqids;
  int rval;
  GtFeatureIndexMemory *fi;
  assert(gfi);

  fi = gt_feature_index_memory_cast((GtFeatureIndex*) gfi);
  seqids = gt_strarray_new();
  rval = hashmap_foreach_in_key_order(fi->regions, store_seqid, seqids, NULL);
  assert(!rval); /* store_seqid() is sane */
  return seqids;
}

void gt_feature_index_memory_get_range_for_seqid(GtFeatureIndex *gfi,
                                                 GtRange *range,
                                                 const char *seqid)
{
  RegionInfo *info;
  GtFeatureIndexMemory *fi;
  assert(gfi && range && seqid);
  fi = gt_feature_index_memory_cast(gfi);
  info = (RegionInfo*) hashmap_get(fi->regions, seqid);
  assert(info);

  if (info->dyn_range.start != ~0UL && info->dyn_range.end != 0) {
    range->start = info->dyn_range.start;
    range->end = info->dyn_range.end;
  }
  else if (info->region)
    *range = gt_genome_node_get_range((GtGenomeNode*) info->region);
}

bool gt_feature_index_memory_has_seqid(const GtFeatureIndex *gfi,
                                       const char *seqid)
{
  GtFeatureIndexMemory *fi;
  assert(gfi);

  fi = gt_feature_index_memory_cast((GtFeatureIndex*) gfi);
  return (hashmap_get(fi->regions, seqid));
}

void gt_feature_index_memory_delete(GtFeatureIndex *gfi)
{
  if (!gfi) return;
  GtFeatureIndexMemory *fi;
  fi = gt_feature_index_memory_cast(gfi);
  hashmap_delete(fi->regions);
}

const GtFeatureIndexClass* gt_feature_index_memory_class(void)
{
  static const GtFeatureIndexClass gt_feature_index_class =
    { sizeof (GtFeatureIndexMemory),
      gt_feature_index_memory_add_region_node,
      gt_feature_index_memory_add_feature_node,
      gt_feature_index_memory_add_gff3file,
      gt_feature_index_memory_get_features_for_seqid,
      gt_feature_index_memory_get_features_for_range,
      gt_feature_index_memory_get_first_seqid,
      gt_feature_index_memory_get_seqids,
      gt_feature_index_memory_get_range_for_seqid,
      gt_feature_index_memory_has_seqid,
      gt_feature_index_memory_delete };
  return &gt_feature_index_class;
}

GtFeatureIndex* gt_feature_index_memory_new(void)
{
  GtFeatureIndexMemory *fim;
  GtFeatureIndex *fi;
  fi = gt_feature_index_create(gt_feature_index_memory_class());
  fim = gt_feature_index_memory_cast(fi);
  fim->regions = hashmap_new(HASH_STRING, NULL,
                             (GtFree) region_info_delete);
  return fi;
}

int gt_feature_index_memory_unit_test(GtError *err)
{
  GtGenomeNode *gn1, *gn2, *ex1, *ex2, *ex3, *cds1;
  GtFeatureIndex *fi;
  GtRange check_range, rs;
  GtStr *seqid1, *seqid2;
  GtStrArray *seqids = NULL;
  GtRegionNode *rn1, *rn2;
  GtArray *features = NULL;
  int had_err = 0;
  gt_error_check(err);

  /* generating some ranges */
  rs.start=100UL; rs.end=1200UL;

  /* generating sequnce ids as C-strings */
  seqid1 = gt_str_new_cstr("test1");
  seqid2 = gt_str_new_cstr("test2");

  rn1 = (GtRegionNode*) gt_region_node_new(seqid1, rs.start, rs.end);
  rn2 = (GtRegionNode*) gt_region_node_new(seqid2, rs.start, rs.end);

  /* generate a new genome feature */
  gn1 = gt_feature_node_new(seqid1, gft_gene, 100, 1000, GT_STRAND_UNKNOWN);
  gn2 = gt_feature_node_new(seqid2, gft_gene, 600, 1200, GT_STRAND_UNKNOWN);
  ex1 = gt_feature_node_new(seqid1, gft_exon, 100, 300, GT_STRAND_UNKNOWN);
  ex2 = gt_feature_node_new(seqid1, gft_exon, 500, 1000, GT_STRAND_UNKNOWN);
  ex3 = gt_feature_node_new(seqid2, gft_exon, 600, 1200 , GT_STRAND_UNKNOWN);
  cds1 = gt_feature_node_new(seqid2, gft_CDS, 600, 1200, GT_STRAND_UNKNOWN);

  /* Determine the structure of our feature tree */
  gt_genome_node_add_child(gn1, ex1);
  gt_genome_node_add_child(gn1, ex2);
  gt_genome_node_add_child(gn2, ex3);
  gt_genome_node_add_child(gn2, cds1);

  /* create a new feature index on which we can perform some tests */
  fi = gt_feature_index_memory_new();

  ensure(had_err, fi);
  ensure(had_err, !gt_feature_index_has_seqid(fi, "test1"));
  ensure(had_err, !gt_feature_index_has_seqid(fi, "test2"));

  /* add a sequence region directly and check if it has been added */
  gt_feature_index_add_region_node(fi, rn1);
  ensure(had_err, gt_feature_index_has_seqid(fi, "test1"));
  ensure(had_err, !gt_feature_index_has_seqid(fi, "test2"));

  gt_feature_index_get_range_for_seqid(fi, &check_range, "test1");
  ensure(had_err, check_range.start == 100UL && check_range.end == 1200UL);

  /* tests if we get a empty data structure for every added sequence region*/
  if (!had_err)
    features = gt_feature_index_get_features_for_seqid(fi, "test1");
  ensure(had_err, features);
  ensure(had_err, gt_array_size(features) == 0);
  gt_array_delete(features);
  features = NULL;

  if (!had_err)
    features = gt_feature_index_get_features_for_seqid(fi, "test2");
  ensure(had_err, features);
  ensure(had_err, gt_array_size(features) == 0);
  gt_array_delete(features);
  features = NULL;

  /* add features to every sequence region and test if the according
     datastructures are not empty anymore. As we have added one genome_feature
     to every sequence region the size has to be one. */
  if (!had_err) {
    gt_feature_index_add_feature_node(fi, (GtFeatureNode*) gn1);
    features = gt_feature_index_get_features_for_seqid(fi, "test1");
  }
  ensure(had_err, gt_array_size(features) == 1UL);
  gt_array_delete(features);
  features = NULL;

  if (!had_err) {
    gt_feature_index_add_feature_node(fi, (GtFeatureNode*) gn2);
    features = gt_feature_index_get_features_for_seqid(fi, "test2");
  }
  ensure(had_err, gt_array_size(features) == 1UL);
  gt_array_delete(features);
  features = NULL;

  /* test gt_feature_index_get_first_seqid() */
  ensure(had_err, gt_feature_index_get_first_seqid(fi));
  ensure(had_err, strcmp("test1", gt_feature_index_get_first_seqid(fi)) == 0);

  if (!had_err) {
    seqids = gt_feature_index_get_seqids(fi);
    ensure(had_err, gt_strarray_size(seqids) == 2);
    ensure(had_err, !strcmp(gt_strarray_get(seqids, 0), "test1"));
    ensure(had_err, !strcmp(gt_strarray_get(seqids, 1), "test2"));
  }

  gt_feature_index_get_range_for_seqid(fi, &check_range, "test1");
  ensure(had_err, check_range.start == 100UL && check_range.end == 1000UL);

  gt_feature_index_get_range_for_seqid(fi, &check_range, "test2");
  ensure(had_err, check_range.start == 600UL && check_range.end == 1200UL);

  if (!had_err)
    features = gt_feature_index_get_features_for_seqid(fi, "test1");
  ensure(had_err, features);
  gt_array_delete(features);

  /* delete all generated objects */
  gt_strarray_delete(seqids);
  gt_feature_index_delete(fi);
  gt_genome_node_rec_delete(gn1);
  gt_genome_node_rec_delete(gn2);
  gt_genome_node_rec_delete((GtGenomeNode*) rn1);
  gt_genome_node_rec_delete((GtGenomeNode*) rn2);
  gt_str_delete(seqid1);
  gt_str_delete(seqid2);
  return had_err;
}
