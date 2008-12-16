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
#include "core/versionfunc.h"
#include "extended/genome_node.h"
#include "extended/gff3_out_stream.h"
#include "extended/gtf_in_stream.h"
#include "tools/gt_gtf_to_gff3.h"

typedef struct {
  bool be_tolerant;
} GTFToGFF3Arguments;

static void* gt_gtf_to_gff3_arguments_new(void)
{
  return gt_calloc(1, sizeof (GTFToGFF3Arguments));
}

static void gt_gtf_to_gff3_arguments_delete(void *tool_arguments)
{
  GTFToGFF3Arguments *arguments = tool_arguments;
  if (!arguments) return;
  gt_free(arguments);
}

static GtOptionParser* gt_gtf_to_gff3_option_parser_new(void *tool_arguments)
{
  GTFToGFF3Arguments *arguments = tool_arguments;
  GtOptionParser *op;
  GtOption *option;
  gt_assert(arguments);
  op = gt_option_parser_new("[gtf_file]",
                            "Parse GTF2.2 file and show it as GFF3.");
  /* -tolerant */
  option = gt_option_new_bool("tolerant",
                              "be tolerant when parsing the GTF file",
                              &arguments->be_tolerant, false);
  gt_option_parser_add_option(op, option);
  gt_option_parser_set_max_args(op, 1);
  return op;
}

static int gt_gtf_to_gff3_runner(GT_UNUSED int argc, const char **argv,
                                 int parsed_args,
                                 void *tool_arguments, GtError *err)
{
  GTFToGFF3Arguments *arguments = tool_arguments;
  GtNodeStream *gtf_in_stream = NULL, *gff3_out_stream = NULL;
  GtGenomeNode *gn;
  int had_err;

  gt_error_check(err);
  gt_assert(arguments);

  /* create a GTF input stream */
  gtf_in_stream = gt_gtf_in_stream_new(argv[parsed_args]);
  if (arguments->be_tolerant)
    gt_gtf_in_stream_enable_tidy_mode(gtf_in_stream);

  /* create a GFF3 output stream */
  /* XXX: use proper genfile */
  gff3_out_stream = gt_gff3_out_stream_new(gtf_in_stream, NULL);

  /* pull the features through the stream and free them afterwards */
  while (!(had_err = gt_node_stream_next(gff3_out_stream, &gn, err)) && gn)
    gt_genome_node_delete(gn);

  /* free */
  gt_node_stream_delete(gff3_out_stream);
  gt_node_stream_delete(gtf_in_stream);

  return had_err;
}

GtTool* gt_gtf_to_gff3(void)
{
  return gt_tool_new(gt_gtf_to_gff3_arguments_new,
                     gt_gtf_to_gff3_arguments_delete,
                     gt_gtf_to_gff3_option_parser_new,
                     NULL,
                     gt_gtf_to_gff3_runner);
}
