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

#ifndef GENFILE_H
#define GENFILE_H

#include <stdio.h>
#include <stdlib.h>
#include "core/error.h"

/*
  This class defines generic files.  A generic file is is a file which either
  uncompressed or compressed (with gzip/zlib). All I/O functions like fprintf(),
  fopen(), fread(), and fwrite() have to be replaced by the appropriate generic
  functions like gt_genfile_printf(), gt_genfile_open(), gt_genfile_write(),...

  A NULL-pointer as generic file implies stdout.
*/
typedef struct GtGenFile GtGenFile;

typedef enum {
  GFM_UNCOMPRESSED,
  GFM_GZIP,
  GFM_BZIP2
} GtGenFileMode;

/* Returns GFM_GZIP if file with <path> ends with '.gz', GFM_BZIP2 if it ends
   with '.bz2', and GFM_UNCOMPRESSED otherwise. */
GtGenFileMode gt_genfilemode_determine(const char *path);

/* Returns ".gz" if <mode> is GFM_GZIP, ".bz2" if <mode> is GFM_BZIP2, and ""
   otherwise. */
const char* gt_genfilemode_suffix(GtGenFileMode mode);

/* Returns the length of the ``basename'' of <path>. That is, the length of path
   without '.gz' or '.bz2' suffixes. */
size_t      gt_genfile_basename_length(const char *path);

/* Create a new GtGenFile object and open the underlying file handle, returns
   NULL and sets <err> if the file <path> could not be opened. */
GtGenFile*    gt_genfile_open(GtGenFileMode, const char *path,
                               const char *mode, GtError*);

/* Create a new GtGenFile object and open the underlying file handle, abort if
   the file <path> does not exist. The GtGenFileMode has to be given
   explicitly. */
GtGenFile*    gt_genfile_xopen_w_gfmode(GtGenFileMode, const char *path,
                                         const char *mode);

/* Create a new GtGenFile object and open the underlying file handle. Aborts if
   the file <path> could not be opened. The GtGenFileMode is determined
   automatically via gt_genfilemode_determine(path). */
GtGenFile*    gt_genfile_xopen(const char *path, const char *mode);

/* Create a new GtGenFile object from a normal file pointer. */
GtGenFile*    gt_genfile_new(FILE*);

GtGenFileMode gt_genfile_mode(GtGenFile*);

/* Return next character from <genfile> of EOF, if end-of-file is reached. */
int         gt_genfile_xfgetc(GtGenFile *genfile);

/* Unget character <c> to <genfile> (which obviously cannot be NULL).
   Can only be used once at a time. */
void        gt_genfile_unget_char(GtGenFile *genfile, char c);

/* printf(3) for generic files */
void        gt_genfile_xprintf(GtGenFile*, const char *format, ...)
  __attribute__ ((format (printf, 2, 3)));

void        gt_genfile_xfputc(int c, GtGenFile*);
void        gt_genfile_xfputs(const char *str, GtGenFile*);

/* Read up to <nbytes> and store result in <buf>, returns bytes read. */
int         gt_genfile_xread(GtGenFile*, void *buf, size_t nbytes);

/* Write <nbytes> from <buf> to given generic file. */
void        gt_genfile_xwrite(GtGenFile*, void *buf, size_t nbytes);

/* Rewind the file. */
void        gt_genfile_xrewind(GtGenFile*);

/* Destroy the file handle object, but do not close the underlying handle. */
void        gt_genfile_delete(GtGenFile*);

/* Close the underlying file handle and destroy the object. */
void        gt_genfile_close(GtGenFile*);

#endif
