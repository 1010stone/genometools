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

#ifndef GENOME_NODE_API_H
#define GENOME_NODE_API_H

#include "core/range_api.h"
#include "core/str_api.h"

typedef struct GtGenomeNodeClass GtGenomeNodeClass;

/* The <GtGenomeNode> interface. */
typedef struct GtGenomeNode GtGenomeNode;

#if 0
/* Decrease the reference count for <genome_node> or delete it, if this was the
   last reference. */
void          gt_genome_node_delete(GtGenomeNode *genome_node);
#endif

/* Decrease the reference count for <genome_node> and recursively for all its
   children or delete it, if this was the last reference. */
void          gt_genome_node_rec_delete(GtGenomeNode *genome_node);

/* Return the sequence ID of <genome_node>.
   Corresponds to column 1 of regular GFF3 lines. */
GtStr*        gt_genome_node_get_seqid(GtGenomeNode *genome_node);

/* Return the genomic range of of <genome_node>.
   Corresponds to columns 4 and 5 of regular GFF3 lines. */
GtRange       gt_genome_node_get_range(GtGenomeNode *genome_node);

/* Return the start of <genome_node>.
   Corresponds to column 4 of regular GFF3 lines. */
unsigned long gt_genome_node_get_start(GtGenomeNode *genome_node);

/* Return the end of <genome_node>.
   Corresponds to column 5 of regular GFF3 lines. */
unsigned long gt_genome_node_get_end(GtGenomeNode *genome_Node);

/* Set the genomic range of <genome_node> to given <range>. */
void          gt_genome_node_set_range(GtGenomeNode *genome_node,
                                       const GtRange *range);

#endif
