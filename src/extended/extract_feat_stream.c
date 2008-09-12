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
#include "extended/extract_feat_stream.h"
#include "extended/extract_feat_visitor.h"
#include "extended/node_stream_rep.h"

struct ExtractFeatStream
{
  const GtNodeStream parent_instance;
  GtNodeStream *in_stream;
  GtNodeVisitor *extract_feat_visitor;
};

#define extract_feat_stream_cast(GS)\
        gt_node_stream_cast(extract_feat_stream_class(), GS)

static int extract_feat_stream_next_tree(GtNodeStream *gs, GtGenomeNode **gn,
                                         GtError *err)
{
  ExtractFeatStream *efs;
  int had_err;
  gt_error_check(err);
  efs = extract_feat_stream_cast(gs);
  had_err = gt_node_stream_next(efs->in_stream, gn, err);
  if (!had_err) {
    assert(efs->extract_feat_visitor);
    if (*gn) {
      had_err = gt_genome_node_accept(*gn, efs->extract_feat_visitor, err);
      if (had_err) {
        /* we own the node -> delete it */
        gt_genome_node_rec_delete(*gn);
        *gn = NULL;
      }
    }
  }
  return had_err;
}

static void extract_feat_stream_free(GtNodeStream *gs)
{
  ExtractFeatStream *extract_feat_stream = extract_feat_stream_cast(gs);
  gt_node_visitor_delete(extract_feat_stream->extract_feat_visitor);
  gt_node_stream_delete(extract_feat_stream->in_stream);
}

const GtNodeStreamClass* extract_feat_stream_class(void)
{
  static const GtNodeStreamClass gsc = { sizeof (ExtractFeatStream),
                                         extract_feat_stream_next_tree,
                                         extract_feat_stream_free };
  return &gsc;
}

GtNodeStream* extract_feat_stream_new(GtNodeStream *in_stream,
                                      RegionMapping *rm, const char *type,
                                      bool join, bool translate)
{
  GtNodeStream *gs = gt_node_stream_create(extract_feat_stream_class(), true);
  ExtractFeatStream *efs = extract_feat_stream_cast(gs);
  efs->in_stream = gt_node_stream_ref(in_stream);
  efs->extract_feat_visitor = extract_feat_visitor_new(rm, type, join,
                                                       translate);
  return gs;
}
