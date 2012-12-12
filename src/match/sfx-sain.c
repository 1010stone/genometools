/*
  Copyright (c) 2012 Stefan Kurtz <kurtz@zbh.uni-hamburg.de>
  Copyright (c) 2012 Center for Bioinformatics, University of Hamburg

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

#include <limits.h>
#include "core/minmax.h"
#include "core/unused_api.h"
#include "core/timer_api.h"
#include "core/mathsupport.h"
#include "sfx-linlcp.h"
#include "sfx-sain.h"

#define GT_SAIN_SHOWTIMER(DESC)\
        if (timer != NULL)\
        {\
          gt_timer_show_progress(timer,DESC,stdout);\
        }

typedef enum
{
  GT_SAIN_PLAINSEQ,
  GT_SAIN_INTSEQ,
  GT_SAIN_ENCSEQ
} GtSainSeqtype;

typedef struct
{
  unsigned long totallength,
                numofchars,
                startoccupied,
                currentround,
                *bucketsize,
                *bucketfillptr,
                *sstarfirstcharcount,
                *roundtable;
  union
  {
    const GtEncseq *encseq;
    const GtUchar *plainseq;
    const unsigned long *array;
  } seq;
  GtReadmode readmode; /* only relevant for encseq */
  GtSainSeqtype seqtype;
  bool bucketfillptrpoints2suftab,
       bucketsizepoints2suftab,
       roundtablepoints2suftab;
} GtSainseq;

static bool gt_sain_decideforfastmethod(unsigned long maxvalue,
                                        unsigned long len)
{
  return maxvalue < (unsigned long) GT_FIRSTTWOBITS && len > 1024UL
         ? true : false;
}

static GtSainseq *gt_sainseq_new_from_encseq(const GtEncseq *encseq,
                                             GtReadmode readmode)
{
  unsigned long idx;
  GtSainseq *sainseq = gt_malloc(sizeof *sainseq);

  sainseq->seqtype = GT_SAIN_ENCSEQ;
  sainseq->seq.encseq = encseq;
  sainseq->readmode = readmode;
  sainseq->totallength = gt_encseq_total_length(encseq);
  sainseq->numofchars = (unsigned long) gt_encseq_alphabetnumofchars(encseq);
  sainseq->bucketsize = gt_malloc(sizeof (*sainseq->bucketsize) *
                                  sainseq->numofchars);
  sainseq->bucketfillptr = gt_malloc(sizeof (*sainseq->bucketfillptr) *
                                     sainseq->numofchars);
  if (gt_sain_decideforfastmethod(sainseq->totallength+GT_COMPAREOFFSET,
                                  sainseq->totallength))
  {
    sainseq->roundtable = gt_malloc(sizeof (*sainseq->roundtable) *
                                    (size_t) GT_MULT2(sainseq->numofchars));
  } else
  {
    sainseq->roundtable = NULL;
  }
  sainseq->sstarfirstcharcount
    = gt_calloc((size_t) sainseq->numofchars,
                sizeof (*sainseq->sstarfirstcharcount));
  sainseq->bucketfillptrpoints2suftab = false;
  sainseq->bucketsizepoints2suftab = false;
  sainseq->roundtablepoints2suftab = false;
  for (idx = 0; idx<sainseq->numofchars; idx++)
  {
    if (GT_ISDIRCOMPLEMENT(readmode))
    {
      sainseq->bucketsize[idx]
        = gt_encseq_charcount(encseq,(GtUchar) GT_COMPLEMENTBASE(idx));
    } else
    {
      sainseq->bucketsize[idx] = gt_encseq_charcount(encseq,(GtUchar)idx);
    }
  }
  return sainseq;
}

static GtSainseq *gt_sainseq_new_from_plainseq(const GtUchar *plainseq,
                                               unsigned long len)
{
  unsigned long idx;
  GtSainseq *sainseq = gt_malloc(sizeof *sainseq);

  sainseq->seqtype = GT_SAIN_PLAINSEQ;
  sainseq->seq.plainseq = plainseq;
  sainseq->totallength = len;
  sainseq->numofchars = UCHAR_MAX+1;
  sainseq->bucketsize = gt_malloc(sizeof (*sainseq->bucketsize) *
                                  sainseq->numofchars);
  sainseq->bucketfillptr = gt_malloc(sizeof (*sainseq->bucketfillptr) *
                                     sainseq->numofchars);
  if (gt_sain_decideforfastmethod(len+1,len))
  {
    sainseq->roundtable = gt_malloc(sizeof (*sainseq->roundtable) *
                                    (size_t) GT_MULT2(sainseq->numofchars));
  } else
  {
    sainseq->roundtable = NULL;
  }
  sainseq->sstarfirstcharcount
    = gt_calloc((size_t) sainseq->numofchars,
                sizeof (*sainseq->sstarfirstcharcount));
  sainseq->bucketfillptrpoints2suftab = false;
  sainseq->bucketsizepoints2suftab = false;
  sainseq->roundtablepoints2suftab = false;
  for (idx = 0; idx<sainseq->numofchars; idx++)
  {
    sainseq->bucketsize[idx] = 0;
  }
  for (idx = 0; idx<sainseq->totallength; idx++)
  {
    sainseq->bucketsize[sainseq->seq.plainseq[idx]]++;
  }
  return sainseq;
}

static GtSainseq *gt_sainseq_new_from_array(unsigned long *arr,
                                            unsigned long len,
                                            unsigned long numofchars,
                                            unsigned long *suftab,
                                            unsigned long firstusable,
                                            unsigned long suftabentries)
{
  unsigned long charidx;
  GtSainseq *sainseq = gt_malloc(sizeof *sainseq);

  sainseq->seqtype = GT_SAIN_INTSEQ;
  sainseq->seq.array = arr;
  sainseq->totallength = len;
  sainseq->numofchars = numofchars;
  sainseq->startoccupied = suftabentries;
  gt_assert(firstusable < suftabentries);
  if (suftabentries - firstusable >= numofchars)
  {
    /*printf("bucketsize: reclaim suftab[%lu,%lu]\n",
            suftabentries - numofchars,suftabentries-1);*/
    sainseq->bucketsize = suftab + suftabentries - numofchars;
    sainseq->bucketsizepoints2suftab = true;
    sainseq->startoccupied = suftabentries - numofchars;
  } else
  {
    printf("bucketsize requires %lu entries and only %lu are left\n",
           numofchars,suftabentries - firstusable);
    sainseq->bucketsizepoints2suftab = false;
    sainseq->bucketsize = gt_malloc(sizeof (*sainseq->bucketsize) * numofchars);
  }
  if (suftabentries - firstusable >= GT_MULT2(numofchars))
  {
    /*printf("bucketfillptr: reclaim suftab[%lu,%lu]\n",
            suftabentries - GT_MULT2(numofchars),
            suftabentries - numofchars -1);*/
    sainseq->bucketfillptr = suftab + suftabentries - GT_MULT2(numofchars);
    sainseq->bucketfillptrpoints2suftab = true;
    sainseq->startoccupied = suftabentries - GT_MULT2(numofchars);
  } else
  {
    /*printf("bucketfillptr requires %lu entries and only %lu are left\n",
           numofchars,suftabentries - firstusable);*/
    sainseq->bucketfillptrpoints2suftab = false;
    sainseq->bucketfillptr = gt_malloc(sizeof (*sainseq->bucketfillptr) *
                                       numofchars);
  }
  if (gt_sain_decideforfastmethod(len+1,len))
  {
    if (suftabentries - firstusable >= GT_MULT4(numofchars))
    {
      /*printf("roundtable: reclaim suftab[%lu,%lu]\n",
              suftabentries - GT_MULT4(numofchars),
              suftabentries - GT_MULT2(numofchars) - 1);*/
      sainseq->roundtable = suftab + suftabentries - GT_MULT4(numofchars);
      sainseq->roundtablepoints2suftab = true;
      sainseq->startoccupied = suftabentries - GT_MULT4(numofchars);
    } else
    {
      sainseq->roundtablepoints2suftab = false;
      sainseq->roundtable = gt_malloc(sizeof (*sainseq->roundtable) *
                                      GT_MULT2(numofchars));
    }
  } else
  {
    sainseq->roundtablepoints2suftab = false;
    sainseq->roundtable = NULL;
  }
  sainseq->sstarfirstcharcount = NULL;
  for (charidx = 0; charidx < sainseq->numofchars; charidx++)
  {
    sainseq->bucketsize[charidx] = 0;
  }
  for (charidx = 0; charidx<sainseq->totallength; charidx++)
  {
    unsigned long cc = sainseq->seq.array[charidx];

    gt_assert(cc < sainseq->numofchars);
    sainseq->bucketsize[cc]++;
  }
  return sainseq;
}

static unsigned long gt_sainseq_delete(GtSainseq *sainseq)
{
  unsigned long ret = ULONG_MAX;

  if (sainseq != NULL)
  {
    if (!sainseq->bucketfillptrpoints2suftab)
    {
      gt_free(sainseq->bucketfillptr);
    }
    if (!sainseq->bucketsizepoints2suftab)
    {
      gt_free(sainseq->bucketsize);
    }
    if (!sainseq->roundtablepoints2suftab)
    {
      gt_free(sainseq->roundtable);
    }
    if (sainseq->bucketsizepoints2suftab ||
        sainseq->bucketsizepoints2suftab ||
        sainseq->roundtablepoints2suftab)
    {
      ret = sainseq->startoccupied;
    }
    if (sainseq->seqtype != GT_SAIN_INTSEQ)
    {
      gt_free(sainseq->sstarfirstcharcount);
    }
    gt_free(sainseq);
  }
  return ret;
}

static unsigned long gt_sain_countcharaccess = 0;

static unsigned long gt_sainseq_getchar(const GtSainseq *sainseq,
                                        unsigned long position)
{
  gt_assert(position < sainseq->totallength);
  gt_sain_countcharaccess++;
  switch (sainseq->seqtype)
  {
    case GT_SAIN_PLAINSEQ:
      return (unsigned long) sainseq->seq.plainseq[position];
    case GT_SAIN_INTSEQ:
      return sainseq->seq.array[position];
    case GT_SAIN_ENCSEQ:
      {
        GtUchar cc = gt_encseq_get_encoded_char(sainseq->seq.encseq,
                                                position,
                                                sainseq->readmode);
        return ISSPECIAL(cc) ? GT_UNIQUEINT(position) : (unsigned long) cc;
      }
  }
  /*@ignore@*/
  return 0;
  /*@end@*/
}

static void gt_sain_endbuckets(GtSainseq *sainseq)
{
  unsigned long charidx;

  sainseq->bucketfillptr[0] = sainseq->bucketsize[0];
  for (charidx = 1UL; charidx < sainseq->numofchars; charidx++)
  {
    sainseq->bucketfillptr[charidx] = sainseq->bucketfillptr[charidx-1] +
                                      sainseq->bucketsize[charidx];
  }
}

static void gt_sain_startbuckets(GtSainseq *sainseq)
{
  unsigned long charidx;

  sainseq->bucketfillptr[0] = 0;
  for (charidx = 1UL; charidx < sainseq->numofchars; charidx++)
  {
    sainseq->bucketfillptr[charidx] = sainseq->bucketfillptr[charidx-1] +
                                      sainseq->bucketsize[charidx-1];
  }
}

typedef struct
{
  unsigned long countStype,
                countSstartype,
                namecount;
  GtSainseq *sainseq;
} GtSaininfo;

typedef struct
{
  unsigned long buf_size, numofchars, cachesize, *values, *fillptr, *suftab;
  uint16_t *nextidx;
  int log_bufsize;
  size_t size;
} GtSainbuffer;

static GtSainbuffer *gt_sainbuffer_new(unsigned long *suftab,
                                       unsigned long *fillptr,
                                       unsigned long numofchars)
{
  if (numofchars <= UCHAR_MAX+1)
  {
    GtSainbuffer *buf = gt_malloc(sizeof (*buf));

    buf->size = sizeof (*buf);
    buf->fillptr = fillptr;
    buf->suftab = suftab;
    buf->log_bufsize = 18 - (sizeof (unsigned long) == (size_t) 4 ? 1 : 2) -
                             (int) gt_determinebitspervalue(numofchars);
    buf->buf_size = 1UL << buf->log_bufsize;
    buf->numofchars = numofchars;
    gt_assert(buf->buf_size <= UINT16_MAX);
    buf->cachesize = numofchars << buf->log_bufsize;
    buf->values = gt_malloc(sizeof (*buf->values) * buf->cachesize);
    buf->size += sizeof (*buf->values) * buf->cachesize;
    buf->nextidx = gt_calloc((size_t) numofchars,sizeof (*buf->nextidx));
    buf->size += sizeof (*buf->nextidx) * numofchars;
    printf("used buffer of %lu bytes (bufsize=%lu)\n",
               (unsigned long) buf->size,buf->buf_size);
    return buf;
  } else
  {
    return NULL;
  }
}

static void gt_sainbuffer_update(GtSainbuffer *buf,
                                 unsigned long charidx,
                                 unsigned long value)
{
  const unsigned long offset = charidx << buf->log_bufsize;

  buf->values[offset + (unsigned long) buf->nextidx[charidx]] = value;
  if ((unsigned long) buf->nextidx[charidx] < buf->buf_size - 1)
  {
    buf->nextidx[charidx]++;
  } else
  {
    unsigned long *writeptr = buf->suftab + buf->fillptr[charidx] - 1,
                  *readptr = buf->values + offset;
    const unsigned long *readend = readptr + buf->buf_size;

    while (readptr < readend)
    {
      *(writeptr--) = *(readptr++);
    }
    buf->nextidx[charidx] = 0;
    buf->fillptr[charidx] -= buf->buf_size;
  }
}

static void gt_sainbuffer_flushall(GtSainbuffer *buf)
{
  unsigned long charidx;

  if (buf == NULL)
  {
    return;
  }
  for (charidx = 0; charidx < buf->numofchars; charidx++)
  {
    const unsigned long bufleft = (unsigned long) buf->nextidx[charidx];

    if (bufleft > 0)
    {
      unsigned long *writeptr = buf->suftab + buf->fillptr[charidx] - 1,
                    *readptr = buf->values + (charidx << buf->log_bufsize);
      const unsigned long *readend = readptr + bufleft;

      while (readptr < readend)
      {
        *(writeptr--) = *(readptr++);
      }
      buf->nextidx[charidx] = 0;
      buf->fillptr[charidx] -= bufleft;
    }
  }
}

static void gt_sainbuffer_delete(GtSainbuffer *buf)
{
  if (buf != NULL)
  {
    gt_free(buf->values);
    gt_free(buf->nextidx);
    gt_free(buf);
  }
}

static GtSaininfo *gt_saininfo_new(GtSainseq *sainseq,unsigned long *suftab,
                                   GT_UNUSED unsigned long nonspecialentries)
{
  unsigned long position,
                nextcc,
                *fillptr = sainseq->bucketfillptr;
  bool nextisStype = true;
  GtSaininfo *saininfo;
  GtSainbuffer *sainbuffer = gt_sainbuffer_new(suftab,fillptr,
                                               sainseq->numofchars);

  saininfo = gt_malloc(sizeof *saininfo);
  saininfo->sainseq = sainseq;
  saininfo->countStype = 1UL;
  saininfo->countSstartype = 0;
  nextcc = GT_UNIQUEINT(saininfo->sainseq->totallength);
  gt_sain_endbuckets(saininfo->sainseq);
  for (position = saininfo->sainseq->totallength-1; /* Nothing */; position--)
  {
    bool currentisStype;
    unsigned long currentcc;

    currentcc = gt_sainseq_getchar(saininfo->sainseq,position);
    if (currentcc < nextcc || (currentcc == nextcc && nextisStype))
    {
      currentisStype = true;
      saininfo->countStype++;
    } else
    {
      currentisStype = false;
    }
    /*printf("position %lu, char=%lu is %c-type\n",position,currentcc,
                                           currentisStype ? 'S' : 'L');*/
    if (!currentisStype && nextisStype)
    {
      saininfo->countSstartype++;
      if (sainseq->sstarfirstcharcount != NULL)
      {
        sainseq->sstarfirstcharcount[nextcc]++;
      }
      if (sainbuffer != NULL)
      {
        gt_sainbuffer_update(sainbuffer,nextcc,position);
      } else
      {
        suftab[--fillptr[nextcc]] = position;
      }
#undef SAINSHOWSTATE
#ifdef SAINSHOWSTATE
      printf("Sstar.suftab[%lu]=%lu\n",fillptr[nextcc],position+1);
#endif
    }
    nextisStype = currentisStype;
    nextcc = currentcc;
    if (position == 0)
    {
      break;
    }
  }
  gt_sainbuffer_flushall(sainbuffer);
  gt_sainbuffer_delete(sainbuffer);
  gt_assert(GT_MULT2(saininfo->countSstartype) <=
            saininfo->sainseq->totallength);
  return saininfo;
}

static void gt_saininfo_delete(GtSaininfo *saininfo)
{
  if (saininfo != NULL)
  {
    gt_free(saininfo);
  }
}

static void gt_saininfo_show(const GtSaininfo *saininfo)
{
  printf("S-type: %lu (%.2f)\n",saininfo->countStype,
                (double) saininfo->countStype/saininfo->sainseq->totallength);
  printf("Sstar-type: %lu (%.2f)\n",saininfo->countSstartype,
              (double) saininfo->countSstartype/saininfo->sainseq->totallength);
#ifdef CRITICAL
  {
    unsigned long d;
    for (d=2UL; d<10UL; d++)
    {
      unsigned long critical = gt_sain_countDcriticalsubstrings (saininfo,d);
      printf("d=%lu,critical=%lu (%.2f)\n",d,critical,
                        (double) critical/saininfo->sainseq->totallength);
    }
  }
#endif
}

static void gt_sain_incrementfirstSstar(GtSainseq *sainseq,
                                        unsigned long *suftab)
{
  unsigned long charidx, sum = 0;

  for (charidx = 0; charidx < sainseq->numofchars; charidx++)
  {
    sum += sainseq->bucketsize[charidx];
    gt_assert(sainseq->bucketfillptr[charidx] <= sum);
    if (sainseq->bucketfillptr[charidx] < sum)
    {
      suftab[sainseq->bucketfillptr[charidx]] += sainseq->totallength;
    }
    sainseq->roundtable[charidx] = 0;
    sainseq->roundtable[charidx+sainseq->numofchars] = 0;
  }
}

#define GT_SAINUPDATEBUCKETPTR(CURRENTCC)\
        if (bucketptr != NULL)\
        {\
          if ((CURRENTCC) != lastupdatecc)\
          {\
            fillptr[lastupdatecc] = (unsigned long) (bucketptr - suftab);\
            bucketptr = suftab + fillptr[CURRENTCC];\
            lastupdatecc = CURRENTCC;\
          }\
        } else\
        {\
          bucketptr = suftab + fillptr[CURRENTCC];\
          lastupdatecc = CURRENTCC;\
        }

static void gt_sain_induceLtypesuffixes1(GtSainseq *sainseq,
                                         long *suftab,
                                         unsigned long nonspecialentries)
{
  unsigned long idx, lastupdatecc = 0, *fillptr;
  long *bucketptr = NULL;

  fillptr = sainseq->bucketfillptr;
  for (idx = 0, sainseq->currentround = 0; idx < nonspecialentries; idx++)
  {
    long position = suftab[idx];

    if (position > 0)
    {
      unsigned long currentcc;

      if (position >= (long) sainseq->totallength)
      {
        gt_assert(sainseq->roundtable != NULL);
        sainseq->currentround++;
        position -= (long) sainseq->totallength;
      }
      currentcc = gt_sainseq_getchar(sainseq,(unsigned long) position);
      if (currentcc < sainseq->numofchars)
      {
        if (position > 0)
        {
          unsigned long leftcontextcc;

          gt_assert(position > 0);
          position--;
          leftcontextcc = gt_sainseq_getchar(sainseq,(unsigned long) position);
          if (sainseq->roundtable != NULL)
          {
            unsigned long t = (currentcc << 1) |
                              (leftcontextcc < currentcc ? 1UL : 0);

            gt_assert(currentcc > 0);
            gt_assert(sainseq->roundtable[t] <= sainseq->currentround);
            if (sainseq->roundtable[t] == 0 ||
                sainseq->roundtable[t] < sainseq->currentround)
            {
              position += (long) sainseq->totallength;
              sainseq->roundtable[t] = sainseq->currentround;
            }
          }
          GT_SAINUPDATEBUCKETPTR(currentcc);
          /* negative => position does not derive L-suffix
             positive => position may derive L-suffix */
          gt_assert(suftab + idx < bucketptr);
          *bucketptr++ = (leftcontextcc < currentcc) ? ~position : position;
          suftab[idx] = 0;
#ifdef SAINSHOWSTATE
          gt_assert(bucketptr != NULL);
          printf("L-induce: suftab[%lu]=%ld\n",
                  (unsigned long) (bucketptr-1-suftab),*(bucketptr-1));
#endif
        }
      } else
      {
        suftab[idx] = 0;
      }
    } else
    {
      if (position < 0)
      {
        suftab[idx] = ~position;
      }
    }
  }
  if (sainseq->roundtable == NULL)
  {
    return;
  }
  for (idx = nonspecialentries - 1; /* Nothing */; idx--)
  {
    if (suftab[idx] > 0)
    {
      if (suftab[idx] < (long) sainseq->totallength)
      {
        suftab[idx] += (long) sainseq->totallength;
        if (idx > 0)
        {
          unsigned long nextgteq = idx - 1;

          while (nextgteq > 0 && suftab[nextgteq] < (long) sainseq->totallength)
          {
            nextgteq--;
          }
          if (suftab[nextgteq] >= (long) sainseq->totallength)
          {
            suftab[nextgteq] -= (long) sainseq->totallength;
          }
          idx = nextgteq;
        }
      }
    }
    if (idx == 0)
    {
      break;
    }
  }
  for (idx = 0; idx < nonspecialentries; idx++)
  {
    long position = suftab[idx];

    gt_assert(position >= 0);
    if (position > 0)
    {
      if (position >= (long) sainseq->totallength)
      {
        position -= (long) sainseq->totallength;
      }
    }
  }
}

static void gt_sain_special_singleSinduction1(GtSainseq *sainseq,
                                              long *suftab,
                                              long position)
{
  unsigned long currentcc = gt_sainseq_getchar(sainseq,
                                               (unsigned long) position);

  if (currentcc < sainseq->numofchars)
  {
    unsigned long leftcontextcc,
                  putidx = --sainseq->bucketfillptr[currentcc];

    gt_assert(position > 0);
    position--;
    leftcontextcc = gt_sainseq_getchar(sainseq,(unsigned long) position);
    if (sainseq->roundtable != NULL)
    {
      unsigned long t = (currentcc << 1) |
                        (leftcontextcc > currentcc ? 1UL : 0);

      if (sainseq->roundtable[t] != sainseq->currentround)
      {
        position += sainseq->totallength;
        sainseq->roundtable[t] = sainseq->currentround;
      } else
      {
        position += sainseq->totallength;
      }
    }
    suftab[putidx] = (leftcontextcc > currentcc) ? ~(position+1) : position;
#ifdef SAINSHOWSTATE
    printf("end S-induce: suftab[%lu]=%ld\n",putidx,suftab[putidx]);
#endif
  }
}

static void gt_sain_induceStypes1fromspecialranges(
                                   GtSainseq *sainseq,
                                   const GtEncseq *encseq,
                                   long *suftab)
{
  if (gt_encseq_has_specialranges(encseq))
  {
    GtSpecialrangeiterator *sri;
    GtRange range;
    sri = gt_specialrangeiterator_new(encseq,
                                      GT_ISDIRREVERSE(sainseq->readmode)
                                        ? true : false);
    while (gt_specialrangeiterator_next(sri,&range))
    {
      if (GT_ISDIRREVERSE(sainseq->readmode))
      {
        gt_range_reverse(sainseq->totallength,&range);
      }
      if (range.start > 1UL)
      {
        gt_sain_special_singleSinduction1(sainseq,
                                          suftab,
                                          (long) (range.start - 1));
      }
    }
    gt_specialrangeiterator_delete(sri);
  }
}

static void gt_sain_induceStypesuffixes1(GtSainseq *sainseq,
                                         long *suftab,
                                         unsigned long nonspecialentries)
{
  unsigned long idx, lastupdatecc = 0, *fillptr = sainseq->bucketfillptr;
  long *bucketptr = NULL;

  gt_sain_special_singleSinduction1(sainseq,
                                    suftab,
                                    (long) (sainseq->totallength-1));
  if (sainseq->seqtype == GT_SAIN_ENCSEQ)
  {
    gt_sain_induceStypes1fromspecialranges(sainseq,
                                           sainseq->seq.encseq,
                                           suftab);
  }
  if (nonspecialentries == 0)
  {
    return;
  }
  for (idx = nonspecialentries - 1; /* Nothing */; idx--)
  {
    long position = suftab[idx];

    if (position > 0)
    {
      if (position >= (long) sainseq->totallength)
      {
        gt_assert(sainseq->roundtable != NULL);
        sainseq->currentround++;
        position -= (long) sainseq->totallength;
      }
      if (position > 0)
      {
        unsigned long currentcc = gt_sainseq_getchar(sainseq,
                                                     (unsigned long) position);

        if (currentcc < sainseq->numofchars)
        {
          unsigned long leftcontextcc;

          position--;
          leftcontextcc = gt_sainseq_getchar(sainseq,(unsigned long) position);
          if (sainseq->roundtable != NULL)
          {
            unsigned long t = (currentcc << 1) |
                              (leftcontextcc > currentcc ? 1UL : 0);

            if (sainseq->roundtable[t] != sainseq->currentround)
            {
              position += sainseq->totallength;
              sainseq->roundtable[t] = sainseq->currentround;
            }
          }
          GT_SAINUPDATEBUCKETPTR(currentcc);
          gt_assert(bucketptr != NULL && bucketptr - 1 < suftab + idx);
          *(--bucketptr) = (leftcontextcc > currentcc)
                            ? ~(position+1) : position;
#ifdef SAINSHOWSTATE
          printf("S-induce: suftab[%lu]=%ld\n",
                  (unsigned long) (bucketptr - suftab),*bucketptr);
#endif
        }
      }
      suftab[idx] = 0;
    }
    if (idx == 0)
    {
      break;
    }
  }
}

static void gt_sain_moveSstar2front(GtSaininfo *saininfo,
                                    long *suftab,
                                    GT_UNUSED unsigned long nonspecialentries)
{
  unsigned long readidx, namecount = 0, writeidx = 0;
  long position;

  for (readidx = 0; (position = suftab[readidx]) < 0; readidx++)
  {
    position = ~position;
    if (saininfo->sainseq->roundtable != NULL &&
        position >= (long) saininfo->sainseq->totallength)
    {
      namecount++;
    }
    suftab[readidx] = position;
    gt_assert ((readidx + 1) < nonspecialentries);
  }
  if (readidx < saininfo->countSstartype)
  {
    for (writeidx = readidx, readidx++; /* Nothing */; readidx++)
    {
      gt_assert (readidx < nonspecialentries);
      if ((position = suftab[readidx]) < 0)
      {
        position = ~position;
        if (saininfo->sainseq->roundtable != NULL &&
            position >= (long) saininfo->sainseq->totallength)
        {
          namecount++;
        }
        gt_assert(writeidx < readidx);
        suftab[writeidx++] = position;
        suftab[readidx] = 0;
        if (writeidx == saininfo->countSstartype)
        {
          break;
        }
      } else
      {
        suftab[readidx] = 0;
      }
    }
  } else
  {
    writeidx = readidx;
  }
  gt_assert(writeidx == saininfo->countSstartype);
  if (saininfo->sainseq->roundtable != NULL)
  {
    saininfo->namecount = namecount;
  }
}

static unsigned long gt_sain_simple_assignSstarnames(const GtSaininfo *saininfo,
                                                     unsigned long *suftab,
                                                     unsigned long
                                                     nonspecialentries)
{
  unsigned long idx;

  gt_assert(saininfo->namecount <= saininfo->countSstartype);
  if (saininfo->namecount < saininfo->countSstartype)
  {
    unsigned long currentname = saininfo->namecount + 1;

    for (idx = nonspecialentries - 1; /* Nothing */; idx--)
    {
      unsigned long position = suftab[idx];

      if (position >= saininfo->sainseq->totallength)
      {
        position -= saininfo->sainseq->totallength;
        gt_assert(currentname > 0);
        currentname--;
      }
      if (currentname <= saininfo->namecount)
      {
        suftab[saininfo->countSstartype + GT_DIV2(position)] = currentname;
      }
      if (idx == 0)
      {
        break;
      }
    }
  } else
  {
    for (idx = 0; idx < nonspecialentries; idx++)
    {
      unsigned long position = suftab[idx];

      if (position >= saininfo->sainseq->totallength)
      {
        suftab[idx] = position - saininfo->sainseq->totallength;
      }
    }
  }
  return saininfo->namecount;
}

static void gt_sain_induceLtypesuffixes2(const GtSainseq *sainseq,
                                         long *suftab,
                                         unsigned long nonspecialentries)
{
  unsigned long lastupdatecc = 0, idx, *fillptr = sainseq->bucketfillptr;
  long *bucketptr = NULL;

  for (idx = 0; idx < nonspecialentries; idx++)
  {
    long position = suftab[idx];

    suftab[idx] = ~position;
    if (position > 0)
    {
      unsigned long currentcc;

      position--;
      currentcc = gt_sainseq_getchar(sainseq,(unsigned long) position);
      if (currentcc < sainseq->numofchars)
      {
        gt_assert(currentcc > 0);
        GT_SAINUPDATEBUCKETPTR(currentcc);
        gt_assert(bucketptr != NULL && suftab + idx < bucketptr);
        *bucketptr++ = (position > 0 &&
                        gt_sainseq_getchar(sainseq,
                                           (unsigned long)
                                           (position-1)) < currentcc)
                        ? ~position : position;
#ifdef SAINSHOWSTATE
        gt_assert(bucketptr != NULL);
        printf("L-induce: suftab[%lu]=%ld\n",
               (unsigned long) (bucketptr-1-suftab),*(bucketptr-1));
#endif
      }
    }
  }
}

static void gt_sain_special_singleSinduction2(const GtSainseq *sainseq,
                                              long *suftab,
                                              long position,
                                              GT_UNUSED unsigned long
                                                nonspecialentries)
{
  unsigned long currentcc;

  position--;
  currentcc = gt_sainseq_getchar(sainseq,(unsigned long) position);
  if (currentcc < sainseq->numofchars)
  {
    unsigned long putidx = --sainseq->bucketfillptr[currentcc];

    gt_assert(putidx < nonspecialentries);
    suftab[putidx] = (position == 0 ||
                      gt_sainseq_getchar(sainseq,
                                         (unsigned long)
                                         (position-1)) > currentcc)
                      ? ~position : position;
#ifdef SAINSHOWSTATE
    printf("end S-induce: suftab[%lu]=%ld\n",putidx,suftab[putidx]);
#endif
  }
}

static void gt_sain_induceStypes2fromspecialranges(
                                   const GtSainseq *sainseq,
                                   const GtEncseq *encseq,
                                   long *suftab,
                                   unsigned long nonspecialentries)
{
  if (gt_encseq_has_specialranges(encseq))
  {
    GtSpecialrangeiterator *sri;
    GtRange range;
    sri = gt_specialrangeiterator_new(encseq,
                                      GT_ISDIRREVERSE(sainseq->readmode)
                                        ? true : false);
    while (gt_specialrangeiterator_next(sri,&range))
    {
      if (GT_ISDIRREVERSE(sainseq->readmode))
      {
        gt_range_reverse(sainseq->totallength,&range);
      }
      if (range.start > 0)
      {
        gt_sain_special_singleSinduction2(sainseq,
                                          suftab,
                                          (long) range.start,
                                          nonspecialentries);
      }
    }
    gt_specialrangeiterator_delete(sri);
  }
}

static void gt_sain_induceStypesuffixes2(const GtSainseq *sainseq,
                                         long *suftab,
                                         unsigned long nonspecialentries)
{
  unsigned long lastupdatecc = 0, idx, *fillptr = sainseq->bucketfillptr;
  long *bucketptr = NULL;

  gt_sain_special_singleSinduction2(sainseq,
                                    suftab,
                                    (long) sainseq->totallength,
                                    nonspecialentries);
  if (sainseq->seqtype == GT_SAIN_ENCSEQ)
  {
    gt_sain_induceStypes2fromspecialranges(sainseq,
                                           sainseq->seq.encseq,
                                           suftab,
                                           nonspecialentries);
  }
  if (nonspecialentries == 0)
  {
    return;
  }
  for (idx = nonspecialentries - 1; /* Nothing */; idx--)
  {
    long position = suftab[idx];

    if (position > 0)
    {
      unsigned long currentcc;

      position--;
      currentcc = gt_sainseq_getchar(sainseq,(unsigned long) position);
      if (currentcc < sainseq->numofchars)
      {
        GT_SAINUPDATEBUCKETPTR(currentcc);
        gt_assert(bucketptr != NULL && bucketptr - 1 < suftab + idx);
        *(--bucketptr) = (position == 0 ||
                          gt_sainseq_getchar(sainseq,
                                             (unsigned long)
                                             (position-1)) > currentcc)
                         ? ~position : position;
#ifdef SAINSHOWSTATE
        gt_assert(bucketptr != NULL);
        printf("S-induce: suftab[%lu]=%ld\n",(unsigned long) (bucketptr-suftab),
                                             *bucketptr);
#endif
      }
    } else
    {
      suftab[idx] = ~position;
    }
    if (idx == 0)
    {
      break;
    }
  }
}

static int gt_sain_compare_Sstarstrings(const GtSainseq *sainseq,
                                        unsigned long start1,
                                        unsigned long start2,
                                        unsigned long len)
{
  unsigned long end1 = start1 + len;

  gt_assert(start1 <= sainseq->totallength &&
            start2 <= sainseq->totallength &&
            start1 != start2);

  while (start1 < end1)
  {
    unsigned long cc1, cc2;

    if (start1 == sainseq->totallength)
    {
      gt_assert(start1 > start2);
      return 1;
    }
    if (start2 == sainseq->totallength)
    {
      gt_assert(start1 < start2);
      return -1;
    }
    cc1 = gt_sainseq_getchar(sainseq,start1);
    cc2 = gt_sainseq_getchar(sainseq,start2);
    if (cc1 < cc2)
    {
      return -1;
    }
    if (cc1 > cc2)
    {
      return 1;
    }
    start1++;
    start2++;
  }
  return 0;
}

static int gt_sain_compare_suffixes(const GtSainseq *sainseq,
                                    unsigned long start1,
                                    unsigned long start2)
{
  gt_assert(start1 <= sainseq->totallength &&
            start2 <= sainseq->totallength &&
            start1 != start2);

  while (true)
  {
    unsigned long cc1, cc2;

    if (start1 == sainseq->totallength)
    {
      gt_assert(start1 > start2);
      return 1;
    }
    if (start2 == sainseq->totallength)
    {
      gt_assert(start1 < start2);
      return -1;
    }
    cc1 = gt_sainseq_getchar(sainseq,start1);
    cc2 = gt_sainseq_getchar(sainseq,start2);
    if (cc1 < cc2)
    {
      return -1;
    }
    if (cc1 > cc2)
    {
      return 1;
    }
    start1++;
    start2++;
  }
}

static void gt_sain_setundefined(unsigned long *suftab,
                                 unsigned long start, unsigned long end)
{
  unsigned long idx;

  for (idx = start; idx <= end; idx++)
  {
    suftab[idx] = 0;
  }
}

static void gt_sain_assignSstarlength(GtSainseq *sainseq,
                                      unsigned long *lentab)
{
  bool nextisStype = true;
  unsigned long position,
                nextSstartypepos = sainseq->totallength,
                nextcc = GT_UNIQUEINT(sainseq->totallength);

  for (position = sainseq->totallength-1; /* Nothing */; position--)
  {
    bool currentisStype;
    unsigned long currentcc;

    currentcc = gt_sainseq_getchar(sainseq,position);
    if (currentcc < nextcc || (currentcc == nextcc && nextisStype))
    {
      currentisStype = true;
    } else
    {
      currentisStype = false;
    }
    if (!currentisStype && nextisStype)
    {
      unsigned long currentlen;

      gt_assert(position < nextSstartypepos);
      currentlen = nextSstartypepos - position;
      lentab[GT_DIV2(position+1)] = currentlen;
      nextSstartypepos = position + 1;
    }
    nextisStype = currentisStype;
    nextcc = currentcc;
    if (position == 0)
    {
      break;
    }
  }
}

static unsigned long gt_sain_assignSstarnames(const GtSaininfo *saininfo,
                                              unsigned long *suftab,
                                              GT_UNUSED unsigned long
                                              availableentries)
{
  unsigned long idx, previouspos, previouslen, currentname = 1UL;

  previouspos = suftab[0];
  previouslen = suftab[saininfo->countSstartype + GT_DIV2(previouspos)];
  suftab[saininfo->countSstartype + GT_DIV2(previouspos)] = currentname;
  for (idx = 1UL; idx < saininfo->countSstartype; idx++)
  {
    int cmp;
    unsigned long currentlen = 0, position = suftab[idx];

    currentlen = suftab[saininfo->countSstartype + GT_DIV2(position)];
    if (previouslen == currentlen)
    {
      cmp = gt_sain_compare_Sstarstrings(saininfo->sainseq,previouspos,position,
                                         currentlen);
      gt_assert(cmp != 1);
    } else
    {
      cmp = -1;
    }
    if (cmp == -1)
    {
      currentname++;
    }
    /* write the names in order of positions. As the positions of
       the Sstar suffixes differ by at least 2, the used address
       is unique */
    previouslen = currentlen;
    gt_assert(saininfo->countSstartype + GT_DIV2(position) < availableentries);
    suftab[saininfo->countSstartype + GT_DIV2(position)] = currentname;
    previouspos = position;
  }
  return currentname;
}

static void gt_sain_movenames2front(unsigned long *suftab,
                                    unsigned long numberofsuffixes,
                                    unsigned long totallength,
                                    GT_UNUSED unsigned long availableentries)
{
  unsigned long ridx, widx;
  const unsigned long maxridx = numberofsuffixes + GT_DIV2(totallength);

  for (ridx = widx = numberofsuffixes; ridx <= maxridx; ridx++)
  {
    unsigned long position = suftab[ridx];

    if (position > 0)
    {
      gt_assert(widx < availableentries);
      suftab[widx++] = position - 1UL; /* As we have used names with
                                          offset 1 to distinguish them
                                          from the undefined values
                                          signified by 0 */
    }
  }
  gt_assert(widx == GT_MULT2(numberofsuffixes));
}

static void gt_sain_checkorder(const GtSainseq *sainseq,
                               const unsigned long *suftab,
                               unsigned long start,
                               unsigned long end)
{
  unsigned long idx;

  for (idx = start+1; idx <= end; idx++)
  {
    int cmp
      = gt_sain_compare_suffixes(sainseq,suftab[idx-1],suftab[idx]);

    if (cmp != -1)
    {
      fprintf(stderr,"%s: check interval [%lu,%lu] at idx=%lu: suffix "
                     "%lu >= %lu\n",__func__,start,end,idx,suftab[idx-1],
                     suftab[idx]);
      exit(GT_EXIT_PROGRAMMING_ERROR);
    }
  }
}

static void gt_sain_expandorder2original(GtSainseq *sainseq,
                                         unsigned long numberofsuffixes,
                                         unsigned long *suftab)
{
  unsigned long idx, position,
                writeidx = numberofsuffixes - 1,
                nextcc = GT_UNIQUEINT(sainseq->totallength),
                *sstarsuffixes = suftab + numberofsuffixes;
  bool nextisStype = true;
  unsigned long *sstarfirstcharcount = NULL, *bucketsize = NULL;

  if (sainseq->seqtype == GT_SAIN_INTSEQ)
  {
    unsigned long charidx;

    gt_assert(sainseq->sstarfirstcharcount == NULL);
    sstarfirstcharcount = sainseq->sstarfirstcharcount
                        = sainseq->bucketfillptr;
    bucketsize = sainseq->bucketsize;
    for (charidx = 0; charidx < sainseq->numofchars; charidx++)
    {
      sstarfirstcharcount[charidx] = 0;
      bucketsize[charidx] = 0;
    }
  }
  for (position = sainseq->totallength-1; /* Nothing */; position--)
  {
    bool currentisStype;
    unsigned long currentcc;

    currentcc = gt_sainseq_getchar(sainseq,position);
    if (bucketsize != NULL)
    {
      bucketsize[currentcc]++;
    }
    if (currentcc < nextcc || (currentcc == nextcc && nextisStype))
    {
      currentisStype = true;
    } else
    {
      currentisStype = false;
    }
    if (!currentisStype && nextisStype)
    {
      if (sstarfirstcharcount != NULL)
      {
        sstarfirstcharcount[nextcc]++;
      }
      sstarsuffixes[writeidx--] = position+1;
    }
    nextisStype = currentisStype;
    nextcc = currentcc;
    if (position == 0)
    {
      break;
    }
  }
  for (idx = 0; idx < numberofsuffixes; idx++)
  {
    suftab[idx] = sstarsuffixes[suftab[idx]];
  }
}

static void gt_sain_determineSstarfirstchardist(GtSainseq *sainseq)
{
  unsigned long position,
                nextcc = GT_UNIQUEINT(sainseq->totallength);
  bool nextisStype = true;

  gt_assert(sainseq->seqtype == GT_SAIN_INTSEQ);
  for (position = sainseq->totallength-1; /* Nothing */; position--)
  {
    bool currentisStype;
    unsigned long currentcc;

    currentcc = sainseq->seq.array[position];
    sainseq->bucketsize[currentcc]++;
    if (currentcc < nextcc || (currentcc == nextcc && nextisStype))
    {
      currentisStype = true;
    } else
    {
      currentisStype = false;
    }
    if (!currentisStype && nextisStype)
    {
      sainseq->sstarfirstcharcount[nextcc]++;
    }
    nextisStype = currentisStype;
    nextcc = currentcc;
    if (position == 0)
    {
      break;
    }
  }
}

static void gt_sain_insertsortedSstarsuffixes(const GtSainseq *sainseq,
                                              unsigned long *suftab,
                                              unsigned long readidx,
                                              unsigned long nonspecialentries)
{
  unsigned long cc, fillidx = nonspecialentries;

  for (cc = sainseq->numofchars - 1; /* Nothing */; cc--)
  {
    if (sainseq->sstarfirstcharcount[cc] > 0)
    {
      unsigned long putidx = fillidx - 1;

      gt_assert(readidx <= putidx);
      if (readidx < putidx)
      {
        unsigned long offset;

        for (offset = 0; offset < sainseq->sstarfirstcharcount[cc]; offset++)
        {
          suftab[putidx - offset] = suftab[readidx - offset];
          suftab[readidx - offset] = 0;
#ifdef SAINSHOWSTATE
          printf("insertsorted: suftab[%lu]=%lu\n",putidx-offset,
                                                   suftab[putidx-offset]);
          printf("insertsorted: suftab[%lu]=undef\n",readidx-offset);
#endif
        }
      }
    }
    gt_assert(fillidx >= sainseq->bucketsize[cc]);
    fillidx -= sainseq->bucketsize[cc];
    gt_assert(sainseq->bucketsize[cc] >=
              sainseq->sstarfirstcharcount[cc]);
    if (sainseq->bucketsize[cc] > sainseq->sstarfirstcharcount[cc])
    {
      gt_sain_setundefined(suftab,fillidx,
                           fillidx + sainseq->bucketsize[cc] -
                           sainseq->sstarfirstcharcount[cc] - 1);
    }
    readidx -= sainseq->sstarfirstcharcount[cc];
    if (cc == 0)
    {
      break;
    }
  }
}

static void gt_sain_filltailsuffixes(unsigned long *suftabtail,
                                     const GtEncseq *encseq,
                                     GtReadmode readmode)
{
  unsigned long specialcharacters = gt_encseq_specialcharacters(encseq),
                totallength = gt_encseq_total_length(encseq);

  if (gt_encseq_has_specialranges(encseq))
  {
    GtSpecialrangeiterator *sri;
    GtRange range;
    unsigned long countspecial = 0;

    sri = gt_specialrangeiterator_new(encseq,
                                      GT_ISDIRREVERSE(readmode) ? false : true);
    while (gt_specialrangeiterator_next(sri,&range))
    {
      unsigned long idx;

      if (GT_ISDIRREVERSE(readmode))
      {
        gt_range_reverse(totallength,&range);
      }
      for (idx = range.start; idx < range.end; idx++)
      {
        gt_assert(countspecial < specialcharacters && idx < totallength);
        suftabtail[countspecial++] = idx;
      }
    }
    gt_assert(countspecial == specialcharacters);
    gt_specialrangeiterator_delete(sri);
  }
  suftabtail[specialcharacters] = totallength;
}

static void gt_sain_rec_sortsuffixes(unsigned int level,
                                     GtSainseq *sainseq,
                                     unsigned long *suftab,
                                     unsigned long firstusable,
                                     unsigned long nonspecialentries,
                                     unsigned long availableentries,
                                     unsigned long suftabentries,
                                     bool intermediatecheck,
                                     bool finalcheck,
                                     bool verbose,
                                     GtTimer *timer)
{
  GtSaininfo *saininfo;

  printf("level %u: sort sequence of length %lu over %lu symbols (%.2f)\n",
          level,sainseq->totallength,sainseq->numofchars,
          (double) sainseq->numofchars/sainseq->totallength);
  GT_SAIN_SHOWTIMER("insert Sstar suffixes");
  saininfo = gt_saininfo_new(sainseq,suftab,nonspecialentries);
  if (verbose)
  {
    gt_saininfo_show(saininfo);
  }
  if (saininfo->countSstartype > 0)
  {
    unsigned long numberofnames;

    if (sainseq->roundtable != NULL)
    {
      gt_sain_incrementfirstSstar(sainseq,suftab);
    }
    gt_sain_startbuckets(sainseq);
    GT_SAIN_SHOWTIMER("induce L suffixes");
    gt_sain_induceLtypesuffixes1(saininfo->sainseq,(long *) suftab,
                                 nonspecialentries);
    gt_sain_endbuckets(sainseq);
    GT_SAIN_SHOWTIMER("induce S suffixes");
    gt_sain_induceStypesuffixes1(saininfo->sainseq,(long *) suftab,
                                 nonspecialentries);
    GT_SAIN_SHOWTIMER("moverStar2front");
    gt_sain_moveSstar2front(saininfo,(long *) suftab,nonspecialentries);
    if (saininfo->sainseq->roundtable == NULL)
    {
      GT_SAIN_SHOWTIMER("assignSstarlength");
      gt_sain_assignSstarlength(saininfo->sainseq,
                                suftab + saininfo->countSstartype);
      GT_SAIN_SHOWTIMER("assignSstarnames");
      numberofnames = gt_sain_assignSstarnames(saininfo,suftab,
                                               availableentries);
    } else
    {
      if (!saininfo->sainseq->roundtablepoints2suftab)
      {
        gt_free(saininfo->sainseq->roundtable);
        saininfo->sainseq->roundtable = NULL;
      }
      numberofnames = gt_sain_simple_assignSstarnames(saininfo,suftab,
                                                      nonspecialentries);
    }
    gt_assert(numberofnames <= saininfo->countSstartype);
    if (numberofnames < saininfo->countSstartype)
    {
    /* Now the name sequence is in the range from
       saininfo->countSstartype .. 2 * saininfo->countSstartype - 1 */
      unsigned long startoccupied,
                    *subseq = suftab + saininfo->countSstartype;
      GtSainseq *sainseq_rec;

      GT_SAIN_SHOWTIMER("movenames2front");
      gt_sain_setundefined(suftab,0,saininfo->countSstartype-1);
      gt_sain_movenames2front(suftab,saininfo->countSstartype,
                              saininfo->sainseq->totallength,
                              availableentries);
      if (level == 0)
      {
        firstusable = GT_MULT2(saininfo->countSstartype);
      }
      sainseq_rec = gt_sainseq_new_from_array(subseq,
                                              saininfo->countSstartype,
                                              numberofnames,
                                              suftab,
                                              firstusable,
                                              suftabentries);
      gt_sain_rec_sortsuffixes(level+1,
                               sainseq_rec,
                               suftab,
                               firstusable,
                               saininfo->countSstartype,
                               saininfo->countSstartype,
                               suftabentries,
                               intermediatecheck,
                               finalcheck,
                               verbose,
                               timer);
      startoccupied = gt_sainseq_delete(sainseq_rec);
      GT_SAIN_SHOWTIMER("expandorder2original");
      gt_sain_expandorder2original(saininfo->sainseq,saininfo->countSstartype,
                                   suftab);
    } else
    {
      if (sainseq->seqtype == GT_SAIN_INTSEQ)
      {
        unsigned long charidx;
        gt_assert(sainseq->sstarfirstcharcount == NULL);
        sainseq->sstarfirstcharcount = sainseq->bucketfillptr;

        for (charidx = 0; charidx < sainseq->numofchars; charidx++)
        {
          sainseq->sstarfirstcharcount[charidx] = 0;
          sainseq->bucketsize[charidx] = 0;
        }
        gt_sain_determineSstarfirstchardist(sainseq);
      }
    }
  }
  if (intermediatecheck && saininfo->countSstartype > 0)
  {
    gt_sain_checkorder(sainseq,suftab,0,saininfo->countSstartype-1);
  }
  GT_SAIN_SHOWTIMER("insert sorted Sstar suffixes");
  if (saininfo->countSstartype > 0)
  {
    gt_sain_insertsortedSstarsuffixes (saininfo->sainseq, suftab ,
                                       saininfo->countSstartype - 1,
                                       nonspecialentries);
  }
  gt_sain_startbuckets(sainseq);
  GT_SAIN_SHOWTIMER("induce L suffixes");
  gt_sain_induceLtypesuffixes2(saininfo->sainseq,(long *) suftab,
                               nonspecialentries);
  gt_sain_endbuckets(sainseq);
  GT_SAIN_SHOWTIMER("induce S suffixes");
  gt_sain_induceStypesuffixes2(saininfo->sainseq,(long *) suftab,
                               nonspecialentries);
  if (nonspecialentries > 0)
  {
    if (intermediatecheck)
    {
      gt_sain_checkorder(sainseq,suftab,0,nonspecialentries-1);
    }
    if (sainseq->seqtype == GT_SAIN_ENCSEQ && finalcheck)
    {
      GT_SAIN_SHOWTIMER("fill tail suffixes");
      gt_sain_filltailsuffixes(suftab + nonspecialentries,
                               sainseq->seq.encseq,
                               sainseq->readmode);
      GT_SAIN_SHOWTIMER("check suffix order");
      gt_suftab_lightweightcheck(sainseq->seq.encseq,
                                 sainseq->readmode,
                                 sainseq->totallength,
                                 suftab,
                                 NULL);
    }
  }
  gt_saininfo_delete(saininfo);
}

void gt_sain_encseq_sortsuffixes(const GtEncseq *encseq,
                                 GtReadmode readmode,
                                 bool intermediatecheck,
                                 bool finalcheck,
                                 bool verbose,
                                 GtTimer *timer)
{
  unsigned long nonspecialentries, suftabentries, totallength, *suftab;
  GtSainseq *sainseq;

  totallength = gt_encseq_total_length(encseq);
  nonspecialentries = totallength - gt_encseq_specialcharacters(encseq);
  suftabentries = totallength+1;
  suftab = gt_calloc((size_t) suftabentries,sizeof (*suftab));
  sainseq = gt_sainseq_new_from_encseq(encseq,readmode);
  gt_sain_rec_sortsuffixes(0,
                           sainseq,
                           suftab,
                           0,
                           nonspecialentries,
                           suftabentries,
                           suftabentries,
                           intermediatecheck,
                           finalcheck,
                           verbose,
                           timer);
  printf("countcharaccess=%lu (%.2f)\n",gt_sain_countcharaccess,
          (double) gt_sain_countcharaccess/sainseq->totallength);
  (void) gt_sainseq_delete(sainseq);
  gt_free(suftab);
}

void gt_sain_plain_sortsuffixes(const GtUchar *plainseq,
                                unsigned long len,
                                bool intermediatecheck,
                                bool verbose,
                                GtTimer *timer)
{
  unsigned long suftabentries, *suftab;
  GtSainseq *sainseq;

  suftabentries = len+1;
  suftab = gt_calloc((size_t) suftabentries,sizeof (*suftab));
  sainseq = gt_sainseq_new_from_plainseq(plainseq,len);
  gt_sain_rec_sortsuffixes(0,
                           sainseq,
                           suftab,
                           0,
                           sainseq->totallength,
                           suftabentries,
                           suftabentries,
                           intermediatecheck,
                           false,
                           verbose,
                           timer);
  printf("countcharaccess=%lu (%.2f)\n",gt_sain_countcharaccess,
          (double) gt_sain_countcharaccess/sainseq->totallength);
  (void) gt_sainseq_delete(sainseq);
  gt_free(suftab);
}
