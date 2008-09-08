/*
  Copyright (c) 2007 Stefan Kurtz <kurtz@zbh.uni-hamburg.de>
  Copyright (c) 2007 Center for Bioinformatics, University of Hamburg

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

#include <ctype.h>
#include "core/chardef.h"
#include "core/cstr.h"
#include "core/error.h"
#include "core/strarray.h"
#include "core/fastabuffer.h"

#define FASTASEPARATOR    '>'
#define NEWLINESYMBOL     '\n'

FastaBuffer* fastabuffer_new(const GT_StrArray *filenametab,
                             const Uchar *symbolmap,
                             bool plainformat,
                             Filelengthvalues **filelengthtab,
                             Queue *descptr,
                             unsigned long *characterdistribution)
{
  FastaBuffer *fb;
  fb = gt_calloc(1, sizeof (FastaBuffer));
  fb->plainformat = plainformat;
  fb->filenum = 0;
  fb->firstoverallseq = true;
  fb->firstseqinfile = true;
  fb->nextfile = true;
  fb->nextread = fb->nextfree = 0;
  fb->filenametab = filenametab;
  fb->symbolmap = symbolmap;
  fb->complete = false;
  fb->lastspeciallength = 0;
  fb->descptr = descptr;
  if (filelengthtab) {
    *filelengthtab = gt_calloc(gt_strarray_size(filenametab),
                               sizeof (Filelengthvalues));
    fb->filelengthtab = *filelengthtab;
  }
  else
    fb->filelengthtab = NULL;
  fb->characterdistribution = characterdistribution;
  INITARRAY(&fb->headerbuffer, char);
  return fb;
}

static inline int ownbuffergenfile_getc(FastaBuffer *fb,GenFile *inputstream)
{
  if (fb->currentinpos >= fb->currentfillpos)
  {
    fb->currentfillpos = genfile_xread(inputstream,
                                       fb->inputbuffer,
                                       (size_t) INPUTFILEBUFFERSIZE);
    if (fb->currentfillpos == 0)
    {
       return EOF;
    }
    fb->currentinpos = 0;
  }
  return fb->inputbuffer[fb->currentinpos++];
}

static int advancefastabufferstate(FastaBuffer *fb, GT_Error *err)
{
  int currentchar;
  unsigned long currentoutpos = 0, currentfileadd = 0, currentfileread = 0;
  Uchar charcode;

  gt_error_check(err);
  while (true)
  {
    if (currentoutpos >= (unsigned long) OUTPUTFILEBUFFERSIZE)
    {
      if (fb->filelengthtab != NULL)
      {
        fb->filelengthtab[fb->filenum].length
          += (uint64_t) currentfileread;
        fb->filelengthtab[fb->filenum].effectivelength
          += (uint64_t) currentfileadd;
      }
      break;
    }
    if (fb->nextfile)
    {
      if (fb->filelengthtab != NULL)
      {
        fb->filelengthtab[fb->filenum].length = 0;
        fb->filelengthtab[fb->filenum].effectivelength = 0;
      }
      fb->nextfile = false;
      fb->indesc = false;
      fb->firstseqinfile = true;
      currentfileadd = 0;
      currentfileread = 0;
      fb->linenum = (uint64_t) 1;
      fb->inputstream = genfile_xopen(gt_strarray_get(fb->filenametab,
                                                  (unsigned long) fb->filenum),
                                       "rb");
      fb->currentinpos = 0;
      fb->currentfillpos = 0;
    } else
    {
      currentchar = ownbuffergenfile_getc(fb,fb->inputstream);
      if (currentchar == EOF)
      {
        genfile_close(fb->inputstream);
        fb->inputstream = NULL;
        if (fb->filelengthtab != NULL)
        {
          fb->filelengthtab[fb->filenum].length += currentfileread;
          fb->filelengthtab[fb->filenum].effectivelength += currentfileadd;
        }
        if ((unsigned long) fb->filenum == gt_strarray_size(fb->filenametab)-1)
        {
          fb->complete = true;
          break;
        }
        fb->filenum++;
        fb->nextfile = true;
      } else
      {
        currentfileread++;
        if (fb->indesc)
        {
          if (currentchar == NEWLINESYMBOL)
          {
            fb->linenum++;
            fb->indesc = false;
          }
          if (fb->descptr != NULL)
          {
            if (currentchar == NEWLINESYMBOL)
            {
              STOREINARRAY(&fb->headerbuffer, char, 128, '\0');
              queue_add(fb->descptr, gt_cstr_dup(fb->headerbuffer.spacechar));
              fb->headerbuffer.nextfreechar = 0;
            } else
            {
              STOREINARRAY(&fb->headerbuffer, char, 128, currentchar);
            }
          }
        } else
        {
          if (!isspace((int) currentchar))
          {
            if (currentchar == FASTASEPARATOR)
            {
              if (fb->firstoverallseq)
              {
                fb->firstoverallseq = false;
                fb->firstseqinfile = false;
              } else
              {
                if (fb->firstseqinfile)
                {
                  fb->firstseqinfile = false;
                } else
                {
                  currentfileadd++;
                }
                fb->outputbuffer[currentoutpos++] = (Uchar) SEPARATOR;
                fb->lastspeciallength++;
              }
              fb->indesc = true;
            } else
            {
              if (fb->symbolmap == NULL)
              {
                fb->outputbuffer[currentoutpos++] = (Uchar) currentchar;
              } else
              {
                charcode = fb->symbolmap[(unsigned int) currentchar];
                if (charcode == (Uchar) UNDEFCHAR)
                {
                  gt_error_set(err,
                            "illegal character '%c': file \"%s\", line %llu",
                            currentchar,
                            gt_strarray_get(fb->filenametab, fb->filenum),
                            (unsigned long long) fb->linenum);
                  return -1;
                }
                if (ISSPECIAL(charcode))
                {
                  fb->lastspeciallength++;
                } else
                {
                  if (fb->lastspeciallength > 0)
                  {
                    fb->lastspeciallength = 0;
                  }
                  if (fb->characterdistribution != NULL)
                  {
                    fb->characterdistribution[charcode]++;
                  }
                }
                fb->outputbuffer[currentoutpos++] = charcode;
              }
              currentfileadd++;
            }
          }
        }
      }
    }
  }
  if (fb->firstoverallseq)
  {
    gt_error_set(err,"no sequences in multiple fasta file(s) %s ...",
              gt_strarray_get(fb->filenametab,0));
    return -2;
  }
  fb->nextfree = currentoutpos;
  return 0;
}

static int advancePlainbufferstate(FastaBuffer *fb, GT_Error *err)
{
  int currentchar;
  unsigned long currentoutpos = 0, currentfileread = 0;

  gt_error_check(err);
  if (fb->descptr != NULL)
  {
    gt_error_set(err, "no headers in plain sequence file");
    return -1;
  }
  while (true)
  {
    if (currentoutpos >= (unsigned long) OUTPUTFILEBUFFERSIZE)
    {
      if (fb->filelengthtab != NULL)
      {
        fb->filelengthtab[fb->filenum].length
           += (uint64_t) currentfileread;
        fb->filelengthtab[fb->filenum].effectivelength
           += (uint64_t) currentfileread;
      }
      break;
    }
    if (fb->nextfile)
    {
      if (fb->filelengthtab != NULL)
      {
        fb->filelengthtab[fb->filenum].length = 0;
        fb->filelengthtab[fb->filenum].effectivelength = 0;
      }
      fb->nextfile = false;
      fb->firstseqinfile = true;
      currentfileread = 0;
      fb->inputstream = genfile_xopen(gt_strarray_get(fb->filenametab,
                                                  (unsigned long) fb->filenum),
                                      "rb");
      fb->currentinpos = 0;
      fb->currentfillpos = 0;
    } else
    {
      currentchar = ownbuffergenfile_getc(fb,fb->inputstream);
      if (currentchar == EOF)
      {
        genfile_close(fb->inputstream);
        fb->inputstream = NULL;
        if (fb->filelengthtab != NULL)
        {
          fb->filelengthtab[fb->filenum].length
            += (uint64_t) currentfileread;
          fb->filelengthtab[fb->filenum].effectivelength
            += (uint64_t) currentfileread;
        }
        if ((unsigned long) fb->filenum == gt_strarray_size(fb->filenametab)-1)
        {
          fb->complete = true;
          break;
        }
        fb->filenum++;
        fb->nextfile = true;
      } else
      {
        currentfileread++;
        fb->outputbuffer[currentoutpos++] = (Uchar) currentchar;
      }
    }
  }
  if (currentoutpos == 0)
  {
    gt_error_set(err, "no characters in plain file(s) %s ...",
              gt_strarray_get(fb->filenametab,0));
    return -2;
  }
  fb->nextfree = currentoutpos;
  return 0;
}

int advanceformatbufferstate(FastaBuffer *fb, GT_Error *err)
{
  gt_error_check(err);
  if (fb->plainformat)
  {
    return advancePlainbufferstate(fb, err);
  }
  return advancefastabufferstate(fb, err);
}

void fastabuffer_delete(FastaBuffer *fb)
{
  if (!fb) return;
  genfile_close(fb->inputstream);
  FREEARRAY(&fb->headerbuffer, char);
  gt_free(fb);
}
