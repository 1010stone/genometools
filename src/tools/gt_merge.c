/*
  Copyright (c) 2006-2007 Gordon Gremme <gremme@zbh.uni-hamburg.de>
  Copyright (c) 2006-2007 Center for Bioinformatics, University of Hamburg

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

#include "core/option.h"
#include "core/outputfile.h"
#include "core/versionfunc.h"
#include "extended/gff3_in_stream.h"
#include "extended/gff3_out_stream.h"
#include "extended/merge_stream.h"
#include "tools/gt_merge.h"

static OPrval parse_options(int *parsed_args, GtGenFile **outfp, int argc,
                            const char **argv, GtError *err)
{
  GtOptionParser *op;
  GtOutputFileInfo *ofi;
  OPrval oprval;
  gt_error_check(err);
  op = gt_option_parser_new("[option ...] [GFF3_file ...]",
                         "Merge sorted GFF3 files in sorted fashion.");
  ofi = gt_outputfileinfo_new();
  gt_outputfile_register_options(op, outfp, ofi);
  oprval = gt_option_parser_parse(op, parsed_args, argc, argv, gt_versionfunc,
                                  err);
  gt_outputfileinfo_delete(ofi);
  gt_option_parser_delete(op);
  return oprval;
}

int gt_merge(int argc, const char **argv, GtError *err)
{
  GtNodeStream *gff3_in_stream,
                *merge_stream,
                *gff3_out_stream;
  GtArray *genome_streams;
  GtGenomeNode *gn;
  unsigned long i;
  int parsed_args, had_err;
  GtGenFile *outfp;

  /* option parsing */
  switch (parse_options(&parsed_args, &outfp, argc, argv, err)) {
    case OPTIONPARSER_OK: break;
    case OPTIONPARSER_ERROR: return -1;
    case OPTIONPARSER_REQUESTS_EXIT: return 0;
  }

  /* alloc */
  genome_streams = gt_array_new(sizeof (GtNodeStream*));

  /* XXX: check for multiple specification of '-' */

  /* create an gff3 input stream for each given file */
  if (parsed_args < argc) {
    /* we got files to open */
    for (i = parsed_args; i < argc; i++) {
      gff3_in_stream = gt_gff3_in_stream_new_sorted(argv[i], false);
      gt_array_add(genome_streams, gff3_in_stream);
    }
   }
   else {
     /* use stdin */
     gff3_in_stream = gt_gff3_in_stream_new_sorted(NULL, false);
     gt_array_add(genome_streams, gff3_in_stream);
   }

  /* create a merge stream */
  merge_stream = gt_merge_stream_new(genome_streams);

  /* create a gff3 output stream */
  gff3_out_stream = gt_gff3_out_stream_new(merge_stream, outfp);

  /* pull the features through the stream and free them afterwards */
  while (!(had_err = gt_node_stream_next(gff3_out_stream, &gn, err)) &&
         gn) {
    gt_genome_node_rec_delete(gn);
  }

  /* free */
  gt_node_stream_delete(gff3_out_stream);
  gt_node_stream_delete(merge_stream);
  for (i = 0; i < gt_array_size(genome_streams); i++)
    gt_node_stream_delete(*(GtNodeStream**) gt_array_get(genome_streams, i));
  gt_array_delete(genome_streams);
  gt_genfile_close(outfp);

  return had_err;
}
