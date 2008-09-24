/*
  Copyright (c) 2007-2008 Gordon Gremme <gremme@zbh.uni-hamburg.de>
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

#include "core/bioseq_iterator.h"
#include "core/fasta.h"
#include "core/grep.h"
#include "core/ma.h"
#include "core/option.h"
#include "core/outputfile.h"
#include "core/unused_api.h"
#include "match/giextract.pr"
#include "extended/gtdatahelp.h"
#include "tools/gt_extractseq.h"

#define FROMPOS_OPTION_STR  "frompos"
#define TOPOS_OPTION_STR    "topos"

typedef struct {
  GtStr *pattern,
      *ginum;
  unsigned long frompos,
                topos,
                width;
  GtOutputFileInfo *ofi;
  GtGenFile *outfp;
} ExtractSeqArguments;

static void* gt_extractseq_arguments_new(void)
{
  ExtractSeqArguments *arguments = gt_calloc(1, sizeof *arguments);
  arguments->pattern = gt_str_new();
  arguments->ginum = gt_str_new();
  arguments->ofi = gt_outputfileinfo_new();
  return arguments;
}

static void gt_extractseq_arguments_delete(void *tool_arguments)
{
  ExtractSeqArguments *arguments = tool_arguments;
  if (!arguments) return;
  gt_genfile_close(arguments->outfp);
  gt_outputfileinfo_delete(arguments->ofi);
  gt_str_delete(arguments->ginum);
  gt_str_delete(arguments->pattern);
  gt_free(arguments);
}

static GtOptionParser* gt_extractseq_option_parser_new(void *tool_arguments)
{
  ExtractSeqArguments *arguments = tool_arguments;
  GtOptionParser *op;
  GtOption *frompos_option, *topos_option, *match_option, *width_option,
         *ginum_option;
  assert(arguments);

  /* init */
  op = gt_option_parser_new("[option ...] [sequence_file ...]",
                         "Extract sequences from given sequence file(s).");

  /* -frompos */
  frompos_option = gt_option_new_ulong_min(FROMPOS_OPTION_STR,
                                        "extract sequence from this position\n"
                                        "counting from 1 on",
                                        &arguments->frompos, 0, 1);
  gt_option_parser_add_option(op, frompos_option);

  /* -topos */
  topos_option = gt_option_new_ulong_min(TOPOS_OPTION_STR,
                                      "extract sequence up to this position\n"
                                      "counting from 1 on",
                                      &arguments->topos, 0, 1);
  gt_option_parser_add_option(op, topos_option);

  /* -match */
  match_option = gt_option_new_string("match", "extract all sequences whose "
                                   "description matches the given pattern.\n"
                                   "The given pattern must be a valid extended "
                                   "regular expression.", arguments->pattern,
                                   NULL);
  gt_option_parser_add_option(op, match_option);

  /* -ginum */
  ginum_option = gt_option_new_filename("ginum", "extract substrings for gi "
                                     "numbers in specified file",
                                     arguments->ginum);
  gt_option_parser_add_option(op, ginum_option);

  /* -width */
  width_option = gt_option_new_ulong("width", "set output width for showing of "
                                  "sequences (0 disables formatting)",
                                  &arguments->width, 0);
  gt_option_parser_add_option(op, width_option);

  /* output file options */
  gt_outputfile_register_options(op, &arguments->outfp, arguments->ofi);

  /* option implications */
  gt_option_imply(frompos_option, topos_option);
  gt_option_imply(topos_option, frompos_option);

  /* option exclusions */
  gt_option_exclude(frompos_option, match_option);
  gt_option_exclude(topos_option, match_option);
  gt_option_exclude(frompos_option, ginum_option);
  gt_option_exclude(match_option, ginum_option);

  gt_option_parser_set_comment_func(op, gt_gtdata_show_help, NULL);
  return op;
}

static int gt_extractseq_arguments_check(GT_UNUSED int argc,
                                         void *tool_arguments, GtError *err)
{
  ExtractSeqArguments *arguments = tool_arguments;
  gt_error_check(err);
  assert(arguments);
  if (arguments->frompos > arguments->topos) {
    gt_error_set(err,
              "argument to option '-%s' must be <= argument to option '-%s'",
              FROMPOS_OPTION_STR, TOPOS_OPTION_STR);
    return -1;
  }
  return 0;
}

static int extractseq_pos(GtGenFile *outfp, GtBioseq *bs,
                          unsigned long frompos, unsigned long topos,
                          unsigned long width, GtError *err)
{
  int had_err = 0;
  gt_error_check(err);
  assert(bs);
  if (topos > gt_bioseq_get_raw_sequence_length(bs)) {
    gt_error_set(err,
              "argument %lu to option '-%s' is larger than sequence length %lu",
              topos, TOPOS_OPTION_STR, gt_bioseq_get_raw_sequence_length(bs));
    had_err = -1;
  }
  if (!had_err) {
    gt_fasta_show_entry_generic(NULL,
                                gt_bioseq_get_raw_sequence(bs) + frompos - 1,
                                topos - frompos + 1, width, outfp);
  }
  return had_err;
}

static int extractseq_match(GtGenFile *outfp, GtBioseq *bs,
                            const char *pattern, unsigned long width,
                            GtError *err)
{
  const char *desc;
  unsigned long i;
  bool match;
  int had_err = 0;

  gt_error_check(err);
  assert(bs && pattern);

  for (i = 0; !had_err && i < gt_bioseq_number_of_sequences(bs); i++) {
    desc = gt_bioseq_get_description(bs, i);
    assert(desc);
    had_err = gt_grep(&match, pattern, desc, err);
    if (!had_err && match) {
      gt_fasta_show_entry_generic(desc, gt_bioseq_get_sequence(bs, i),
                                  gt_bioseq_get_sequence_length(bs, i), width,
                                  outfp);
    }
  }

  return had_err;
}

static int process_ginum(GtStr *ginum, int argc, const char **argv,
                         unsigned long width, GtGenFile *outfp, GtError *err)
{
  int had_err = 0;
  gt_error_check(err);
  assert(gt_str_length(ginum));

  if (argc == 0) {
    gt_error_set(err,"option -ginum requires at least one file argument");
    had_err = -1;
  }

  if (!had_err) {
    GtStrArray *referencefiletab;
    int i;
    referencefiletab = gt_str_array_new();
    for (i = 0; i < argc; i++)
      gt_str_array_add_cstr(referencefiletab, argv[i]);
    if (extractginumbers(true, outfp, width, ginum, referencefiletab, err) != 1)
      had_err = -1;
    gt_str_array_delete(referencefiletab);
  }

  return had_err;
}

static int gt_extractseq_runner(int argc, const char **argv, int parsed_args,
                                void *tool_arguments, GtError *err)
{
  ExtractSeqArguments *arguments = tool_arguments;
  int had_err = 0;

  gt_error_check(err);
  assert(arguments);
  if (gt_str_length(arguments->ginum)) {
    had_err = process_ginum(arguments->ginum, argc - parsed_args,
                            argv + parsed_args, arguments->width,
                            arguments->outfp, err);
  }
  else {
    GtBioseqIterator *bsi;
    GtBioseq *bs;
    bsi = gt_bioseq_iterator_new(argc - parsed_args, argv + parsed_args);
    while (!had_err &&
           !(had_err = gt_bioseq_iterator_next(bsi, &bs, err)) && bs) {
      if (arguments->frompos) {
        had_err = extractseq_pos(arguments->outfp, bs, arguments->frompos,
                                 arguments->topos, arguments->width, err);
      }
      else {
        had_err = extractseq_match(arguments->outfp, bs,
                                   gt_str_get(arguments->pattern),
                                   arguments->width, err);
      }
      gt_bioseq_delete(bs);
    }
    gt_bioseq_iterator_delete(bsi);
  }
  return had_err;
}

GtTool* gt_extractseq(void)
{
  return gt_tool_new(gt_extractseq_arguments_new,
                  gt_extractseq_arguments_delete,
                  gt_extractseq_option_parser_new,
                  gt_extractseq_arguments_check,
                  gt_extractseq_runner);
}
