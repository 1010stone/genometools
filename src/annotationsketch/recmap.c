/*
  Copyright (c) 2008 Sascha Steinbiss <steinbiss@zbh.uni-hamburg.de>
  Copyright (c) 2008 Center for Bioinformatics, University of Hamburg

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

#include "core/ma.h"
#include "extended/genome_node.h"
#include "annotationsketch/recmap.h"

GtRecMap* gt_recmap_new(double nw_x, double nw_y, double se_x, double se_y,
                         GtFeatureNode *gf)
{
  GtRecMap *rm = gt_malloc(sizeof *rm);
  rm->nw_x = nw_x;
  rm->nw_y = nw_y;
  rm->se_x = se_x;
  rm->se_y = se_y;
  rm->gf = (GtFeatureNode*) gt_genome_node_ref((GtGenomeNode*) gf);
  rm->has_omitted_children = false;
  return rm;
}

double gt_recmap_get_northwest_x(const GtRecMap *rm)
{
  assert(rm);
  return rm->nw_x;
}

double gt_recmap_get_northwest_y(const GtRecMap *rm)
{
  assert(rm);
  return rm->nw_y;
}

double gt_recmap_get_southeast_x(const GtRecMap *rm)
{
  assert(rm);
  return rm->se_x;
}

double gt_recmap_get_southeast_y(const GtRecMap *rm)
{
  assert(rm);
  return rm->se_y;
}

const GtFeatureNode* gt_recmap_get_genome_feature(const GtRecMap *rm)
{
  assert(rm);
  return rm->gf;
}

bool gt_recmap_has_omitted_children(const GtRecMap *rm)
{
  assert(rm);
  return rm->has_omitted_children;
}

int gt_recmap_format_html_imagemap_coords(const GtRecMap *rm, char *buf,
                                          size_t n)
{
  assert(rm && buf);
  return snprintf(buf, n, "%.0f,%.0f,%.0f,%.0f", rm->nw_x, rm->nw_y,
                                                 rm->se_x, rm->se_y);
}

void gt_recmap_delete(GtRecMap *rm)
{
  if (!rm) return;
  gt_genome_node_delete((GtGenomeNode*) rm->gf);
  gt_free(rm);
}
