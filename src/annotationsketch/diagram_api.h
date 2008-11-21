/*
  Copyright (c) 2007      Malte Mader <mmader@zbh.uni-hamburg.de>
  Copyright (c) 2007-2008 Sascha Steinbiss <steinbiss@zbh.uni-hamburg.de>
  Copyright (c) 2007      Christin Schaerfer <cschaerfer@zbh.uni-hamburg.de>
  Copyright (c) 2007      Center for Bioinformatics, University of Hamburg

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

#ifndef DIAGRAM_API_H
#define DIAGRAM_API_H

/* The <GtDiagram> class acts as a representation of a sequence annotation
   diagram independent of any output format. It contains code to arrange
   feature nodes into blocks, lines and tracks. A individual graphical
   representation of the <GtDiagram> contents is created by calling
   <gt_diagram_sketch()> with an appropriate <GtCanvas> object. */
typedef struct GtDiagram GtDiagram;

#include "annotationsketch/feature_index_api.h"
#include "annotationsketch/style_api.h"
#include "annotationsketch/block.h"

/* A <GtTrackSelectorFunc> is a callback function which returns a string
   which can be used as a track identifier for assignment of a <GtBlock>
   to a given track. */
typedef char* (*GtTrackSelectorFunc)(GtBlock*, void*);

/* Create a new <GtDiagram> object representing the feature nodes in
   <feature_index> in region <seqid> overlapping with <range>. The <GtStyle>
   object <style> will be used to determine collapsing options during the
   layout process. */
GtDiagram* gt_diagram_new(GtFeatureIndex *feature_index, const char *seqid,
                          const GtRange *range, GtStyle *style);
/* Create a new <GtDiagram> object representing the feature nodes in
   <features>. The features must overlap with <range>. The <GtStyle>
   object <style> will be used to determine collapsing options during the
   layout process.*/
GtDiagram* gt_diagram_new_from_array(GtArray *features, const GtRange *range,
                                     GtStyle *style);
/* Returns the sequence position range represented by the <diagram>. */
GtRange    gt_diagram_get_range(const GtDiagram *diagram);
/* Assigns a GtTrackSelectorFunc to use to assign blocks to tracks.
   If none is set, or set to NULL, then track types are used as track keys
   (default behaviour). */
void       gt_diagram_set_track_selector_func(GtDiagram*, GtTrackSelectorFunc);

/* Delete the <diagram> and all its components. */
void       gt_diagram_delete(GtDiagram*);

#endif
