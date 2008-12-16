/*
  Copyright (c) 2007      Christin Schaerfer <cschaerfer@zbh.uni-hamburg.de>
  Copyright (c)      2008 Sascha Steinbiss <steinbiss@zbh.uni-hamburg.de>
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

#include "core/ensure.h"
#include "core/hashtable.h"
#include "core/ma.h"
#include "core/undef.h"
#include "core/unused_api.h"
#include "annotationsketch/default_formats.h"
#include "annotationsketch/line.h"
#include "annotationsketch/line_breaker_bases.h"
#include "annotationsketch/style.h"
#include "annotationsketch/track.h"

struct GtTrack {
  GtStr *title;
  unsigned long max_num_lines, discarded_blocks;
  GtLineBreaker *lb;
  bool split;
  GtArray *lines;
};

GtTrack* gt_track_new(GtStr *title, unsigned long max_num_lines, bool split,
                 GtLineBreaker *lb)
{
  GtTrack *track;
  gt_assert(title && lb);
  track = gt_calloc(1, sizeof (GtTrack));
  gt_assert(track);
  track->title = gt_str_ref(title);
  track->lines = gt_array_new(sizeof (GtLine*));
  track->max_num_lines = max_num_lines;
  track->split = split;
  track->lb = lb;
  return track;
}

static GtLine* get_next_free_line(GtTrack *track, GtBlock *block)
{
  unsigned long i;
  GtLine* line;

  gt_assert(track);

  /* find unoccupied line -- may need optimisation */
  for (i = 0; i < gt_array_size(track->lines); i++) {
    line = *(GtLine**) gt_array_get(track->lines, i);
    if (!gt_line_breaker_gt_line_is_occupied(track->lb, line, block))
      return line;
  }
  /* if line limit is hit, do not create any more lines! */
  if (track->max_num_lines != UNDEF_ULONG
       && gt_array_size(track->lines) == track->max_num_lines)
  {
    track->discarded_blocks++;
    return NULL;
  }
  /* make sure there is only one line if 'split_lines' is set to false */
  if (!track->split)
  {
    if (gt_array_size(track->lines) < 1)
    {
      line = gt_line_new();
      gt_array_add(track->lines, line);
    }
    else
      line = *(GtLine**) gt_array_get(track->lines, 0);
    gt_assert(gt_array_size(track->lines) == 1);
  }
  else
  {
    line = gt_line_new();
    gt_array_add(track->lines, line);
  }
  gt_assert(line);
  return line;
}

unsigned long gt_track_get_number_of_discarded_blocks(GtTrack *track)
{
  gt_assert(track);
  return track->discarded_blocks;
}

void gt_track_insert_block(GtTrack *track, GtBlock *block)
{
  GtLine *line;

  gt_assert(track && block);
  line = get_next_free_line(track, block);
  block = gt_block_ref(block);
  if (line)
  {
    gt_line_insert_block(line, block);
    gt_line_breaker_register_block(track->lb, line, block);
  } else gt_block_delete(block);
}

GtStr* gt_track_get_title(const GtTrack *track)
{
  gt_assert(track && track->title);
  return track->title;
}

unsigned long gt_track_get_number_of_lines(const GtTrack *track)
{
  gt_assert(track);
  return gt_array_size(track->lines);
}

unsigned long gt_track_get_number_of_lines_with_captions(const GtTrack *track)
{
  unsigned long i = 0, nof_tracks = 0;
  gt_assert(track);
  for (i = 0; i < gt_array_size(track->lines); i++) {
    if (gt_line_has_captions(*(GtLine**) gt_array_get(track->lines, i)))
      nof_tracks++;
  }
  return nof_tracks;
}

int gt_track_sketch(GtTrack* track, GtCanvas *canvas, GtError *err)
{
  int i = 0, had_err = 0;
  gt_assert(track && canvas);
  had_err = gt_canvas_visit_track_pre(canvas, track, err);
  for (i = 0; i < gt_array_size(track->lines); i++)
  {
    had_err = gt_line_sketch(*(GtLine**) gt_array_get(track->lines, i),
                             canvas,
                             err);
    if (had_err)
      break;
  }
  if (!had_err)
    had_err = gt_canvas_visit_track_post(canvas, track, err);
  return had_err;
}

double gt_track_get_height(const GtTrack *track, const GtStyle *sty)
{
  unsigned long i;
  double track_height = 0;
  gt_assert(track && sty);
  for (i = 0; i < gt_array_size(track->lines); i++)
  {
    GtLine *line = *(GtLine**) gt_array_get(track->lines, i);
    track_height += gt_line_get_height(line, sty);
  }
  return track_height;
}

int gt_track_unit_test(GtError *err)
{
  int had_err = 0;
  GtBlock *b[4];
  GtRange r[4];
  GtTrack *track;
  GtGenomeNode *parent[4], *gn[4];
  GtStr *title;
  GtStyle *sty;
  unsigned long i;
  GtLineBreaker *lb;
  gt_error_check(err);

  title = gt_str_new_cstr("test");

  r[0].start=100UL;  r[0].end=1000UL;
  r[1].start=1001UL; r[1].end=1500UL;
  r[2].start=700UL;  r[2].end=1200UL;
  r[3].start=10UL;   r[3].end=200UL;

  for (i=0;i<4;i++)
  {
    parent[i] = gt_feature_node_new(title, gft_gene, r[i].start, r[i].end,
                                    GT_STRAND_FORWARD);
    gn[i] = gt_feature_node_new(title, gft_exon, r[i].start, r[i].end,
                                 GT_STRAND_FORWARD);

    gt_feature_node_add_child((GtFeatureNode*) parent[i],
                              (GtFeatureNode*) gn[i]);

    gt_feature_node_add_attribute((GtFeatureNode*) parent[i], "Name", "parent");
    gt_feature_node_add_attribute((GtFeatureNode*) gn[i], "Name", "child");
  }

  for (i=0;i<4;i++)
  {
    b[i] = gt_block_new();
    gt_block_set_range(b[i], r[i]);
    gt_block_insert_element(b[i], (GtFeatureNode*) parent[i]);
    gt_block_insert_element(b[i], (GtFeatureNode*) gn[i]);
  }

  lb = gt_line_breaker_bases_new();

  sty = gt_style_new(err);

  track = gt_track_new(title, UNDEF_ULONG, true, lb);
  ensure(had_err, track);
  ensure(had_err, gt_track_get_title(track) == title);

  ensure(had_err, gt_track_get_number_of_lines(track) == 0);
  ensure(had_err, gt_track_get_height(track, sty) == 0);

  gt_track_insert_block(track, b[0]);
  ensure(had_err, gt_track_get_number_of_lines(track) == 1);
  ensure(had_err, gt_track_get_height(track, sty) == BAR_HEIGHT_DEFAULT);

  gt_track_insert_block(track, b[1]);
  ensure(had_err, gt_track_get_number_of_lines(track) == 1);
  ensure(had_err, gt_track_get_height(track, sty) == BAR_HEIGHT_DEFAULT);

  gt_track_insert_block(track, b[2]);
  ensure(had_err, gt_track_get_number_of_lines(track) == 2);
  gt_track_insert_block(track, b[3]);
  ensure(had_err, gt_track_get_number_of_lines(track) == 2);
  ensure(had_err, gt_track_get_height(track, sty) == 2*BAR_HEIGHT_DEFAULT);

  gt_style_set_num(sty, "exon", "bar_height", 42);
  ensure(had_err, gt_track_get_height(track, sty) == 2*42);
  gt_style_set_num(sty, "gene", "bar_height", 23);
  ensure(had_err, gt_track_get_height(track, sty) == 2*42);
  gt_style_unset(sty, "exon", "bar_height");
  ensure(had_err, gt_track_get_height(track, sty) == 2*23);
  gt_style_unset(sty, "gene", "bar_height");
  gt_style_set_num(sty, "format", "bar_height", 99);
  ensure(had_err, gt_track_get_height(track, sty) == 2*99);

  ensure(had_err, gt_track_get_number_of_discarded_blocks(track) == 0);

  gt_track_delete(track);
  gt_str_delete(title);
  gt_style_delete(sty);
  for (i=0;i<4;i++)
  {
    gt_block_delete(b[i]);
    gt_genome_node_delete(parent[i]);
  }
  return had_err;
}

void gt_track_delete(GtTrack *track)
{
  unsigned long i;
  if (!track) return;
  if (track->lb)
    gt_line_breaker_delete(track->lb);
  for (i = 0; i < gt_array_size(track->lines); i++)
    gt_line_delete(*(GtLine**) gt_array_get(track->lines, i));
  gt_array_delete(track->lines);
  gt_str_delete(track->title);
  gt_free(track);
}
