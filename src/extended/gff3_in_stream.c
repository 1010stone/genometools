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
#include <string.h>
#include "core/cstr_table.h"
#include "core/fileutils.h"
#include "core/progressbar.h"
#include "core/strarray.h"
#include "extended/gff3_in_stream.h"
#include "extended/gff3_parser.h"
#include "extended/node_stream_rep.h"

struct GFF3InStream
{
  const GtNodeStream parent_instance;
  unsigned long next_file;
  GtStrArray *files;
  GtStr *stdinstr;
  bool ensure_sorting,
       stdin_argument,
       file_is_open,
       be_verbose,
       checkids;
  GtGenFile *fpin;
  unsigned long long line_number;
  GtQueue *genome_node_buffer;
  GT_GFF3Parser *gff3_parser;
  GtCstrTable *used_types;
};

#define gff3_in_stream_cast(GS)\
        gt_node_stream_cast(gff3_in_stream_class(), GS)

static int buffer_is_sorted(void **elem, void *info, GtError *err)
{
  GtGenomeNode *current_node, **last_node;

  gt_error_check(err);
  assert(elem && info);

  current_node = *(GtGenomeNode**) elem,
  last_node = info;

  if (*last_node && gt_genome_node_compare(last_node, &current_node) > 0) {
    assert(*last_node);
    gt_error_set(err, "the file %s is not sorted (example: line %u and %u)",
              gt_genome_node_get_filename(*last_node),
              gt_genome_node_get_line_number(*last_node),
              gt_genome_node_get_line_number(current_node));
    return -1;
  }
  else
    *last_node = current_node;
  return 0;
}

static int gff3_in_stream_next(GtNodeStream *gs, GtGenomeNode **gn,
                               GtError *err)
{
  GFF3InStream *is = gff3_in_stream_cast(gs);
  GtStr *filenamestr;
  int had_err = 0, status_code;

  gt_error_check(err);

  if (gt_queue_size(is->genome_node_buffer) > 1) {
    /* we still have at least two nodes in the buffer -> serve from there */
    *gn = gt_queue_get(is->genome_node_buffer);
    return 0;
  }

  /* the buffer is empty or has one element */
  assert(gt_queue_size(is->genome_node_buffer) <= 1);

  for (;;) {
    /* open file if necessary */
    if (!is->file_is_open) {
      if (gt_strarray_size(is->files) &&
          is->next_file == gt_strarray_size(is->files)) {
        break;
      }
      if (gt_strarray_size(is->files)) {
        if (strcmp(gt_strarray_get(is->files, is->next_file), "-") == 0) {
          if (is->stdin_argument) {
            gt_error_set(err, "multiple specification of argument file \"-\"");
            had_err = -1;
            break;
          }
          is->fpin = gt_genfile_xopen(NULL, "r");
          is->file_is_open = true;
          is->stdin_argument = true;
        }
        else {
          is->fpin = gt_genfile_xopen(gt_strarray_get(is->files, is->next_file),
                                   "r");
          is->file_is_open = true;
        }
        is->next_file++;
      }
      else {
        is->fpin = NULL;
        is->file_is_open = true;
      }
      is->line_number = 0;

      if (!had_err && is->be_verbose) {
        printf("processing file \"%s\"\n", gt_strarray_size(is->files)
               ? gt_strarray_get(is->files, is->next_file-1) : "stdin");
      }
      if (!had_err && is->fpin && is->be_verbose) {
        gt_progressbar_start(&is->line_number,
                            gt_file_number_of_lines(gt_strarray_get(is->files,
                                                             is->next_file-1)));
      }
    }

    assert(is->file_is_open);

    filenamestr = gt_strarray_size(is->files)
                  ? gt_strarray_get_str(is->files, is->next_file-1)
                  : is->stdinstr;
    /* read two nodes */
    had_err = gt_gff3_parser_parse_genome_nodes(is->gff3_parser, &status_code,
                                                is->genome_node_buffer,
                                                is->used_types, filenamestr,
                                                &is->line_number, is->fpin,
                                                err);
    if (had_err)
      break;
    if (status_code != EOF) {
      had_err = gt_gff3_parser_parse_genome_nodes(is->gff3_parser, &status_code,
                                                  is->genome_node_buffer,
                                                  is->used_types, filenamestr,
                                                  &is->line_number, is->fpin,
                                                  err);
      if (had_err)
        break;
    }

    if (status_code == EOF) {
      /* end of current file */
      if (is->be_verbose) gt_progressbar_stop();
      gt_genfile_close(is->fpin);
      is->fpin = NULL;
      is->file_is_open = false;
      gt_gff3_parser_reset(is->gff3_parser);
      if (!gt_strarray_size(is->files))
        break;
      continue;
    }

    assert(gt_queue_size(is->genome_node_buffer));

    /* make sure the parsed nodes are sorted */
    if (is->ensure_sorting && gt_queue_size(is->genome_node_buffer) > 1) {
      GtGenomeNode *last_node = NULL;
      /* a sorted stream can have at most one input file */
      assert(gt_strarray_size(is->files) == 0 ||
             gt_strarray_size(is->files) == 1);
      had_err = gt_queue_iterate(is->genome_node_buffer, buffer_is_sorted,
                              &last_node, err);
    }
    if (!had_err) {
      *gn = gt_queue_get(is->genome_node_buffer);
    }
    return had_err;
  }
  assert(!gt_queue_size(is->genome_node_buffer));
  *gn = NULL;
  return had_err;
}

static void gff3_in_stream_free(GtNodeStream *gs)
{
  GFF3InStream *gff3_in_stream = gff3_in_stream_cast(gs);
  gt_strarray_delete(gff3_in_stream->files);
  gt_str_delete(gff3_in_stream->stdinstr);
  while (gt_queue_size(gff3_in_stream->genome_node_buffer))
    gt_genome_node_rec_delete(gt_queue_get(gff3_in_stream->genome_node_buffer));
  gt_queue_delete(gff3_in_stream->genome_node_buffer);
  gt_gff3_parser_delete(gff3_in_stream->gff3_parser);
  gt_cstr_table_delete(gff3_in_stream->used_types);
  gt_genfile_close(gff3_in_stream->fpin);
}

const GtNodeStreamClass* gff3_in_stream_class(void)
{
  static const GtNodeStreamClass *nsc = NULL;
  if (!nsc) {
    nsc = gt_node_stream_class_new(sizeof (GFF3InStream),
                                   gff3_in_stream_free,
                                   gff3_in_stream_next);
  }
  return nsc;
}

/* takes ownership of <files> */
static GtNodeStream* gff3_in_stream_new(GtStrArray *files,
                                        bool ensure_sorting, bool be_verbose,
                                        bool checkids)
{
  GtNodeStream *gs = gt_node_stream_create(gff3_in_stream_class(),
                                          ensure_sorting);
  GFF3InStream *gff3_in_stream       = gff3_in_stream_cast(gs);
  gff3_in_stream->next_file          = 0;
  gff3_in_stream->files              = files;
  gff3_in_stream->stdinstr           = gt_str_new_cstr("stdin");
  gff3_in_stream->ensure_sorting     = ensure_sorting;
  gff3_in_stream->stdin_argument     = false;
  gff3_in_stream->file_is_open       = false;
  gff3_in_stream->fpin               = NULL;
  gff3_in_stream->line_number        = 0;
  gff3_in_stream->genome_node_buffer = gt_queue_new();
  gff3_in_stream->checkids           = checkids;
  gff3_in_stream->gff3_parser        = gt_gff3_parser_new(checkids, NULL);
  gff3_in_stream->used_types         = gt_cstr_table_new();
  gff3_in_stream->be_verbose         = be_verbose;
  return gs;
}

void gff3_in_stream_set_type_checker(GtNodeStream *gs,
                                     GT_TypeChecker *type_checker)
{
  GFF3InStream *is = gff3_in_stream_cast(gs);
  assert(is);
  gt_gff3_parser_delete(is->gff3_parser);
  is->gff3_parser = gt_gff3_parser_new(is->checkids, type_checker);
}

GtStrArray* gff3_in_stream_get_used_types(GtNodeStream *gs)
{
  GFF3InStream *is = gff3_in_stream_cast(gs);
  assert(is);
  return gt_cstr_table_get_all(is->used_types);
}

void gff3_in_stream_set_offset(GtNodeStream *gs, long offset)
{
  GFF3InStream *is = gff3_in_stream_cast(gs);
  assert(is);
  gt_gff3_parser_set_offset(is->gff3_parser, offset);
}

int gff3_in_stream_set_offsetfile(GtNodeStream *gs, GtStr *offsetfile,
                                  GtError *err)
{
  GFF3InStream *is = gff3_in_stream_cast(gs);
  assert(is);
  return gt_gff3_parser_set_offsetfile(is->gff3_parser, offsetfile, err);
}

void gff3_in_stream_enable_tidy_mode(GtNodeStream *gs)
{
  GFF3InStream *is = gff3_in_stream_cast(gs);
  assert(is);
  gt_gff3_parser_enable_tidy_mode(is->gff3_parser);
}

GtNodeStream* gff3_in_stream_new_unsorted(int num_of_files,
                                          const char **filenames,
                                          bool be_verbose, bool checkids)
{
  int i;
  GtStrArray *files = gt_strarray_new();
  for (i = 0; i < num_of_files; i++)
    gt_strarray_add_cstr(files, filenames[i]);
  return gff3_in_stream_new(files, false, be_verbose, checkids);
}

GtNodeStream* gff3_in_stream_new_sorted(const char *filename, bool be_verbose)
{
  GtStrArray *files = gt_strarray_new();
  if (filename)
    gt_strarray_add_cstr(files, filename);
  return gff3_in_stream_new(files, true, be_verbose, false);
}
