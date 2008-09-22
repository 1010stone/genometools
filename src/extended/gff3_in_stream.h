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

#ifndef GFF3_IN_STREAM_H
#define GFF3_IN_STREAM_H

#include <stdio.h>
#include "extended/node_stream.h"
#include "extended/type_checker.h"

/* implements the ``genome_stream'' interface */
typedef struct GtGFF3InStream GtGFF3InStream;

const GtNodeStreamClass* gt_gff3_in_stream_class(void);
void                     gt_gff3_in_stream_set_type_checker(GtNodeStream*,
                                                            GtTypeChecker
                                                            *type_checker);
/* Returns a <GtStrArray*> which contains all type names in alphabetical order
   which have been parsed by <gff3_in_stream>.
   The caller is responsible to free it! */
GtStrArray*              gt_gff3_in_stream_get_used_types(GtNodeStream
                                                          *gff3_in_stream);
void                     gt_gff3_in_stream_set_offset(GtNodeStream*, long);
int                      gt_gff3_in_stream_set_offsetfile(GtNodeStream*, GtStr*,
                                                          GtError*);
void                     gt_gff3_in_stream_enable_tidy_mode(GtNodeStream*);
GtNodeStream*            gt_gff3_in_stream_new_unsorted(int num_of_files,
                                                        const char **filenames,
                                                        bool be_verbose,
                                                        bool checkids);
/* filename == NULL -> use stdin */
GtNodeStream*            gt_gff3_in_stream_new_sorted(const char *filename,
                                                      bool be_verbose);

#endif
