/*
  Copyright (c) 2007-2008 Gordon Gremme <gremme@zbh.uni-hamburg.de>
  Copyright (c) 2007-2008 Center for Bioinforfatics, University of Hamburg

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

#ifndef FA_H
#define FA_H

#include <bzlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>
#include "core/error.h"
#include "core/str.h"

/* the file allocator module */

/* functions for normal file pointer */
#define gt_fopen(path, mode, err)\
        gt_fopen_func(path, mode, __FILE__, __LINE__, err)
FILE*   gt_fopen_func(const char *path, const char *mode, const char*, int,
                      GT_Error*);
#define gt_xfopen(path, mode)\
        gt_xfopen_func(path, mode, __FILE__, __LINE__)
FILE*   gt_xfopen_func(const char *path, const char *mode, const char*, int);
void    gt_fclose(FILE *stream);
void    gt_xfclose(FILE *stream);

/* functions for gzip file pointer */
#define gt_gzopen(path, mode, err)\
        gt_gzopen_func(path, mode, __FILE__, __LINE__, err)
gzFile  gt_gzopen_func(const char *path, const char *mode, const char*, int,
                       GT_Error*);
#define gt_xgzopen(path, mode)\
        gt_xgzopen_func(path, mode, __FILE__, __LINE__)
gzFile  gt_xgzopen_func(const char *path, const char *mode, const char*, int);
void    gt_gzclose(gzFile stream);
void    gt_xgzclose(gzFile stream);

/* functions for bzip2 file pointer */
#define gt_bzopen(path, mode, err)\
        gt_bzopen_func(path, mode, __FILE__, __LINE__, err)
BZFILE* gt_bzopen_func(const char *path, const char *mode, const char*, int,
                       GT_Error*);
#define gt_xbzopen(path, mode)\
        gt_xbzopen_func(path, mode, __FILE__, __LINE__)
BZFILE* gt_xbzopen_func(const char *path, const char *mode, const char*, int);
void    gt_bzclose(BZFILE *stream);
void    gt_xbzclose(BZFILE *stream);

/* create a tmp file optionally using template analogous to mkstemp(3) */
enum tmpfp_flags
{
  TMPFP_AUTOREMOVE    = 1 << 0, /**< otherwise template holds a valid
                                 * path to (re-)open the temporary
                                 * file created */
  TMPFP_USETEMPLATE   = 1 << 1, /**< if set use string template
                                 * given, otherwise the value of
                                 * template is overwritten with an
                                 * interval default */
  TMPFP_OPENBINARY    = 1 << 2, /**< use stdio mode "w+b", "w+" otherwise */
  TMPFP_DEFAULT_FLAGS = 0,
};
#define gt_xtmpfp_generic(template, flags) \
        gt_xtmpfp_generic_func(template, TMPFP_DEFAULT_FLAGS, \
                               __FILE__, __LINE__)
FILE*   gt_xtmpfp_generic_func(GT_Str *template, int flags, const char*, int);
#define gt_xtmpfp(template)\
        gt_xtmpfp_generic(template, TMPFP_DEFAULT_FLAGS)

/* memory map functions */
#define gt_mmap_read(path, len)\
        gt_mmap_read_func(path, len, __FILE__, __LINE__)
void*   gt_mmap_read_func(const char *path, size_t *len, const char*, int);
#define gt_mmap_write(path, len)\
        gt_mmap_write_func(path, len, __FILE__, __LINE__)
void*   gt_mmap_write_func(const char *path, size_t *len, const char*, int);
#define gt_xmmap_read(path, len)\
        gt_xmmap_read_func(path, len, __FILE__, __LINE__)
void*   gt_xmmap_read_func(const char *path, size_t *len, const char*, int);
#define gt_xmmap_write(path, len)\
        gt_xmmap_write_func(path, len, __FILE__, __LINE__)
void*   gt_xmmap_write_func(const char *path, size_t *len, const char*, int);
void    gt_xmunmap(void *addr);
#define gt_mmap_generic_fd(fd, len, offset, mapwritable, hard_fail) \
  gt_mmap_generic_fd_func(fd, len, offset, mapwritable, hard_fail,  \
                          __FILE__, __LINE__)
void*   gt_mmap_generic_fd_func(int fd, size_t len, size_t offset,
                                bool mapwritable, bool hard_fail,
                                const char *filename, int line);
/* check if all allocated file pointer have been released, prints to stderr */
int     gt_fa_check_fptr_leak(void);
/* check if all allocated memory maps have been freed, prints to stderr */
int     gt_fa_check_mmap_leak(void);
void    gt_fa_show_space_peak(FILE*);
void    gt_fa_clean(void);

#endif
