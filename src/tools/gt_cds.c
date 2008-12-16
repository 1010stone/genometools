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

#include "core/ma.h"
#include "core/option.h"
#include "core/unused_api.h"
#include "extended/cds_stream.h"
#include "extended/genome_node.h"
#include "extended/gff3_in_stream.h"
#include "extended/gff3_out_stream.h"
#include "extended/gtdatahelp.h"
#include "extended/seqid2file.h"
#include "tools/gt_cds.h"

#define GT_CDS_SOURCE_TAG "gt cds"

typedef struct {
  bool verbose;
  GtStr *seqfile,
      *regionmapping;
} CDSArguments;

static void *gt_cds_arguments_new(void)
{
  CDSArguments *arguments = gt_calloc(1, sizeof *arguments);
  arguments->seqfile = gt_str_new();
  arguments->regionmapping = gt_str_new();
  return arguments;
}

static void gt_cds_arguments_delete(void *tool_arguments)
{
  CDSArguments *arguments = tool_arguments;
  if (!arguments) return;
  gt_str_delete(arguments->regionmapping);
  gt_str_delete(arguments->seqfile);
  gt_free(arguments);
}

static GtOptionParser* gt_cds_option_parser_new(void *tool_arguments)
{
  CDSArguments *arguments = tool_arguments;
  GtOptionParser *op;
  GtOption *option;
  gt_assert(arguments);

  op = gt_option_parser_new("[option ...] GFF3_file",
                            "Add CDS features to exon "
                            "features given in GFF3_file.");

  /* -seqfile and -regionmapping */
  gt_seqid2file_options(op, arguments->seqfile, arguments->regionmapping);

  /* -v */
  option = gt_option_new_verbose(&arguments->verbose);
  gt_option_parser_add_option(op, option);

  gt_option_parser_set_comment_func(op, gt_gtdata_show_help, NULL);
  gt_option_parser_set_min_max_args(op, 1, 1);

  return op;
}

static int gt_cds_runner(GT_UNUSED int argc, const char **argv, int parsed_args,
                         void *tool_arguments, GtError *err)
{
  GtNodeStream *gff3_in_stream, *cds_stream = NULL, *gff3_out_stream = NULL;
  GtGenomeNode *gn;
  CDSArguments *arguments = tool_arguments;
  GtRegionMapping *regionmapping;
  int had_err = 0;

  gt_error_check(err);
  gt_assert(arguments);

  /* create gff3 input stream */
  gff3_in_stream = gt_gff3_in_stream_new_sorted(argv[parsed_args]);
  if (arguments->verbose)
    gt_gff3_in_stream_show_progress_bar((GtGFF3InStream*) gff3_in_stream);

  /* create region mapping */
  regionmapping = gt_seqid2file_regionmapping_new(arguments->seqfile,
                                               arguments->regionmapping, err);
  if (!regionmapping)
    had_err = -1;

  /* create CDS stream */
  if (!had_err) {
    cds_stream = gt_cds_stream_new(gff3_in_stream, regionmapping,
                                   GT_CDS_SOURCE_TAG);
    if (!cds_stream)
      had_err = -1;
  }

  /* create gff3 output stream */
  /* XXX: replace NULL with proper outfile */
  if (!had_err)
    gff3_out_stream = gt_gff3_out_stream_new(cds_stream, NULL);

  /* pull the features through the stream and free them afterwards */
  while (!(had_err = gt_node_stream_next(gff3_out_stream, &gn, err)) &&
         gn) {
    gt_genome_node_delete(gn);
  }

  /* free */
  gt_node_stream_delete(gff3_out_stream);
  gt_node_stream_delete(cds_stream);
  gt_node_stream_delete(gff3_in_stream);

  return had_err;
}

GtTool *gt_cds(void)
{
  return gt_tool_new(gt_cds_arguments_new,
                  gt_cds_arguments_delete,
                  gt_cds_option_parser_new,
                  NULL,
                  gt_cds_runner);
}
