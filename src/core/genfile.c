/*
  Copyright (c) 2005-2008 Gordon Gremme <gremme@zbh.uni-hamburg.de>
  Copyright (c) 2005-2008 Center for Bioinformatics, University of Hamburg

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

#include <stdio.h>
#include <string.h>
#include "core/assert_api.h"
#include "core/cstr.h"
#include "core/fa.h"
#include "core/genfile.h"
#include "core/ma.h"
#include "core/xansi.h"
#include "core/xbzlib.h"
#include "core/xzlib.h"

struct GtGenFile {
  GtGenFileMode mode;
  union {
    FILE *file;
    gzFile gzfile;
    BZFILE *bzfile;
  } fileptr;
  char *orig_path,
       *orig_mode,
       unget_char;
  bool is_stdin,
       unget_used;
};

GtGenFileMode gt_genfilemode_determine(const char *path)
{
  size_t path_length;
  if (!path)
    return GFM_UNCOMPRESSED;
  path_length = strlen(path);
  if (path_length >= 4 && strcmp(".gz", path + path_length - 3) == 0)
    return GFM_GZIP;
  if (path_length >= 5 && strcmp(".bz2", path + path_length - 4) == 0)
    return GFM_BZIP2;
  return GFM_UNCOMPRESSED;
}

const char* gt_genfilemode_suffix(GtGenFileMode mode)
{
  switch (mode) {
    case GFM_UNCOMPRESSED:
      return "";
    case GFM_GZIP:
      return ".gz";
    case GFM_BZIP2:
      return ".bz2";
    default:
      gt_assert(0);
      return "";
  }
  /* due do warning on solaris:
     warning: control reaches end of non-void function */
  return "";
}

size_t gt_genfile_basename_length(const char *path)
{
  size_t path_length;

  gt_assert(path);
  path_length = strlen(path);

  if (path_length >= 4 && strcmp(".gz", path + path_length - 3) == 0)
    return path_length - 3;
  if (path_length >= 5 && strcmp(".bz2", path + path_length - 4) == 0)
    return path_length - 4;
  return path_length;
}

GtGenFile* gt_genfile_open(GtGenFileMode genfilemode, const char *path,
                      const char *mode, GtError *err)
{
  GtGenFile *genfile;
  gt_error_check(err);
  gt_assert(mode);
  genfile = gt_calloc(1, sizeof (GtGenFile));
  genfile->mode = genfilemode;
  if (path) {
    switch (genfilemode) {
      case GFM_UNCOMPRESSED:
        genfile->fileptr.file = gt_fa_fopen(path, mode, err);
        if (!genfile->fileptr.file) {
          gt_genfile_delete(genfile);
          return NULL;
        }
        break;
      case GFM_GZIP:
        genfile->fileptr.gzfile = gt_fa_gzopen(path, mode, err);
        if (!genfile->fileptr.gzfile) {
          gt_genfile_delete(genfile);
          return NULL;
        }
        break;
      case GFM_BZIP2:
        genfile->fileptr.bzfile = gt_fa_bzopen(path, mode, err);
        if (!genfile->fileptr.bzfile) {
          gt_genfile_delete(genfile);
          return NULL;
        }
        genfile->orig_path = gt_cstr_dup(path);
        genfile->orig_mode = gt_cstr_dup(path);
        break;
      default: gt_assert(0);
    }
  }
  else {
    gt_assert(genfilemode == GFM_UNCOMPRESSED);
    genfile->fileptr.file = stdin;
    genfile->is_stdin = true;
  }
  return genfile;
}

GtGenFile* gt_genfile_xopen_w_gfmode(GtGenFileMode genfilemode,
                                      const char *path, const char *mode)
{
  GtGenFile *genfile;
  gt_assert(mode);
  genfile = gt_calloc(1, sizeof (GtGenFile));
  genfile->mode = genfilemode;
  if (path) {
    switch (genfilemode) {
      case GFM_UNCOMPRESSED:
        genfile->fileptr.file = gt_fa_xfopen(path, mode);
        break;
      case GFM_GZIP:
        genfile->fileptr.gzfile = gt_fa_xgzopen(path, mode);
        break;
      case GFM_BZIP2:
        genfile->fileptr.bzfile = gt_fa_xbzopen(path, mode);
        genfile->orig_path = gt_cstr_dup(path);
        genfile->orig_mode = gt_cstr_dup(path);
        break;
      default: gt_assert(0);
    }
  }
  else {
    gt_assert(genfilemode == GFM_UNCOMPRESSED);
    genfile->fileptr.file = stdin;
    genfile->is_stdin = true;
  }
  return genfile;
}

GtGenFile* gt_genfile_xopen(const char *path, const char *mode)
{
  gt_assert(mode);
  return gt_genfile_xopen_w_gfmode(gt_genfilemode_determine(path), path, mode);
}

GtGenFile* gt_genfile_new(FILE *fp)
{
  GtGenFile *genfile;
  gt_assert(fp);
  genfile = gt_calloc(1, sizeof (GtGenFile));
  genfile->mode = GFM_UNCOMPRESSED;
  genfile->fileptr.file = fp;
  return genfile;
}

GtGenFileMode gt_genfile_mode(GtGenFile *genfile)
{
  gt_assert(genfile);
  return genfile->mode;
}

int gt_genfile_xfgetc(GtGenFile *genfile)
{
  int c = -1;
  if (genfile) {
    if (genfile->unget_used) {
      c = genfile->unget_char;
      genfile->unget_used = false;
    }
    else {
      switch (genfile->mode) {
        case GFM_UNCOMPRESSED:
          c = gt_xfgetc(genfile->fileptr.file);
          break;
        case GFM_GZIP:
          c = gt_xgzfgetc(genfile->fileptr.gzfile);
          break;
        case GFM_BZIP2:
          c = gt_xbzfgetc(genfile->fileptr.bzfile);
          break;
        default: gt_assert(0);
      }
    }
  }
  else
    c = gt_xfgetc(stdin);
  return c;
}

void gt_genfile_unget_char(GtGenFile *genfile, char c)
{
  gt_assert(genfile);
  gt_assert(!genfile->unget_used); /* only one char can be unget at a time */
  genfile->unget_char = c;
  genfile->unget_used = true;
}

static int vgzprintf(gzFile file, const char *format, va_list va)
{
  char buf[BUFSIZ];
  int len;
  len = vsnprintf(buf, sizeof (buf), format, va);
  gt_assert(len <= BUFSIZ);
  return gzwrite(file, buf, (unsigned) len);
}

static int vbzprintf(BZFILE *file, const char *format, va_list va)
{
  char buf[BUFSIZ];
  int len;
  len = vsnprintf(buf, sizeof (buf), format, va);
  gt_assert(len <= BUFSIZ);
  return BZ2_bzwrite(file, buf, len);
}

static int xvprintf(GtGenFile *genfile, const char *format, va_list va)
{
  int rval = -1;

  if (!genfile) /* implies stdout */
    rval = vfprintf(stdout, format, va);
  else {
    switch (genfile->mode) {
      case GFM_UNCOMPRESSED:
        rval = vfprintf(genfile->fileptr.file, format, va);
        break;
      case GFM_GZIP:
        rval = vgzprintf(genfile->fileptr.gzfile, format, va);
        break;
      case GFM_BZIP2:
        rval = vbzprintf(genfile->fileptr.bzfile, format, va);
        break;
      default: gt_assert(0);
    }
  }
  return rval;
}

void gt_genfile_xprintf(GtGenFile *genfile, const char *format, ...)
{
  va_list va;
  va_start(va, format);
  if (xvprintf(genfile, format, va) < 0) {
    fprintf(stderr,
            "gt_genfile_xprintf(): xvprintf() returned negative value\n");
    exit(EXIT_FAILURE);
  }
  va_end(va);
}

void gt_genfile_xfputc(int c, GtGenFile *genfile)
{
  if (!genfile)
    return gt_xfputc(c, stdout);
  switch (genfile->mode) {
    case GFM_UNCOMPRESSED:
      gt_xfputc(c, genfile->fileptr.file);
      break;
    case GFM_GZIP:
      gt_xgzfputc(c, genfile->fileptr.gzfile);
      break;
    case GFM_BZIP2:
      gt_xbzfputc(c, genfile->fileptr.bzfile);
      break;
    default: gt_assert(0);
  }
}

void gt_genfile_xfputs(const char *str, GtGenFile *genfile)
{
  if (!genfile)
    return gt_xfputs(str, stdout);
  switch (genfile->mode) {
    case GFM_UNCOMPRESSED:
      gt_xfputs(str, genfile->fileptr.file);
      break;
    case GFM_GZIP:
      gt_xgzfputs(str, genfile->fileptr.gzfile);
      break;
    case GFM_BZIP2:
      gt_xbzfputs(str, genfile->fileptr.bzfile);
      break;
    default: gt_assert(0);
  }
}

int gt_genfile_xread(GtGenFile *genfile, void *buf, size_t nbytes)
{
  int rval = -1;
  if (genfile) {
    switch (genfile->mode) {
      case GFM_UNCOMPRESSED:
        rval = gt_xfread(buf, 1, nbytes, genfile->fileptr.file);
        break;
      case GFM_GZIP:
        rval = gt_xgzread(genfile->fileptr.gzfile, buf, nbytes);
        break;
      case GFM_BZIP2:
        rval = gt_xbzread(genfile->fileptr.bzfile, buf, nbytes);
        break;
      default: gt_assert(0);
    }
  }
  else
    rval = gt_xfread(buf, 1, nbytes, stdin);
  return rval;
}

void gt_genfile_xwrite(GtGenFile *genfile, void *buf, size_t nbytes)
{
  if (!genfile) {
    gt_xfwrite(buf, 1, nbytes, stdout);
    return;
  }
  switch (genfile->mode) {
    case GFM_UNCOMPRESSED:
      gt_xfwrite(buf, 1, nbytes, genfile->fileptr.file);
      break;
    case GFM_GZIP:
      gt_xgzwrite(genfile->fileptr.gzfile, buf, nbytes);
      break;
    case GFM_BZIP2:
      gt_xbzwrite(genfile->fileptr.bzfile, buf, nbytes);
      break;
    default: gt_assert(0);
  }
}

void gt_genfile_xrewind(GtGenFile *genfile)
{
  gt_assert(genfile);
  switch (genfile->mode) {
    case GFM_UNCOMPRESSED:
      rewind(genfile->fileptr.file);
      break;
    case GFM_GZIP:
      gt_xgzrewind(genfile->fileptr.gzfile);
      break;
    case GFM_BZIP2:
      gt_xbzrewind(&genfile->fileptr.bzfile, genfile->orig_path,
                genfile->orig_mode);
      break;
    default: gt_assert(0);
  }
}

void gt_genfile_delete(GtGenFile *genfile)
{
  if (!genfile) return;
  gt_free(genfile->orig_path);
  gt_free(genfile->orig_mode);
  gt_free(genfile);
}

void gt_genfile_close(GtGenFile *genfile)
{
  if (!genfile) return;
  switch (genfile->mode) {
    case GFM_UNCOMPRESSED:
        if (!genfile->is_stdin)
          gt_fa_fclose(genfile->fileptr.file);
      break;
    case GFM_GZIP:
        gt_fa_gzclose(genfile->fileptr.gzfile);
      break;
    case GFM_BZIP2:
        gt_fa_bzclose(genfile->fileptr.bzfile);
      break;
    default: gt_assert(0);
  }
  gt_genfile_delete(genfile);
}
