/*
  Copyright (c) 2008 Stefan Kurtz <kurtz@zbh.uni-hamburg.de>
  Copyright (c) 2008 Center for Bioinformatics, University of Hamburg

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

#include "core/fa.h"
#include "core/unused_api.h"
#include "core/seqiterator.h"
#include "core/chardef.h"
#include "revcompl.h"
#include "alphadef.h"
#include "format64.h"
#include "encseq-def.h"
#include "tyr-map.h"
#include "tyr-search.h"
#include "tyr-show.h"
#include "spacedef.h"

static unsigned long containsspecialbytestring(const Uchar *seq,
                                               unsigned long len)
{
  const Uchar *sptr;

  for (sptr=seq; sptr < seq + len; sptr++)
  {
    if (ISSPECIAL(*sptr))
    {
      return (unsigned long) (sptr - seq);
    }
  }
  return len;
}

typedef struct
{
  Uchar *bytecode,  /* buffer for encoded word to be searched */
        *rcbuf;
  unsigned long mersize;
  unsigned int showmode,
               searchstrand;
  Alphabet *dnaalpha;
} Tyrsearchinfo;

static void tyrsearchinfo_init(Tyrsearchinfo *tyrsearchinfo,
                               const Tyrindex *tyrindex,
                               unsigned int showmode,
                               unsigned int searchstrand)
{
  unsigned long merbytes;

  merbytes = tyrindex_merbytes(tyrindex);
  tyrsearchinfo->mersize = tyrindex_mersize(tyrindex);
  tyrsearchinfo->showmode = showmode;
  tyrsearchinfo->searchstrand = searchstrand;
  tyrsearchinfo->dnaalpha = assigninputalphabet(true,false,NULL,NULL,NULL);
  ALLOCASSIGNSPACE(tyrsearchinfo->bytecode,NULL,Uchar,merbytes);
  ALLOCASSIGNSPACE(tyrsearchinfo->rcbuf,NULL,Uchar,tyrsearchinfo->mersize);
}

void tyrsearchinfo_delete(Tyrsearchinfo *tyrsearchinfo)
{
  freeAlphabet(&tyrsearchinfo->dnaalpha);
  FREESPACE(tyrsearchinfo->bytecode);
  FREESPACE(tyrsearchinfo->rcbuf);
}

/*@null@*/ const Uchar *searchsinglemer(const Uchar *qptr,
                                        const Tyrindex *tyrindex,
                                        const Tyrsearchinfo *tyrsearchinfo)
{
  const Uchar *result;

  plainseq2bytecode(tyrsearchinfo->bytecode,qptr,tyrsearchinfo->mersize);
  result = tyrindex_binmersearch(tyrindex,0,tyrsearchinfo->bytecode);
  return result;
}

#define ADDTABULATOR\
        if (firstitem)\
        {\
          firstitem = false;\
        } else\
        {\
          (void) putchar('\t');\
        }

static void mermatchoutput(const Tyrindex *tyrindex,
                           const Tyrcountinfo *tyrcountinfo,
                           const Tyrsearchinfo *tyrsearchinfo,
                           const Uchar *result,
                           const Uchar *query,
                           const Uchar *qptr,
                           uint64_t unitnum,
                           bool forward)
{
  bool firstitem = true;
  unsigned long queryposition;

  queryposition = (unsigned long) (qptr-query);
  if (tyrsearchinfo->showmode & SHOWQSEQNUM)
  {
    printf(Formatuint64_t,PRINTuint64_tcast(unitnum));
    firstitem = false;
  }
  if (tyrsearchinfo->showmode & SHOWQPOS)
  {
    ADDTABULATOR;
    printf("%c%lu",forward ? '+' : '-',queryposition);
  }
  if (tyrsearchinfo->showmode & SHOWCOUNTS)
  {
    unsigned long mernumber = tyrindex_ptr2number(tyrindex,result);
    ADDTABULATOR;
    printf("%lu",tyrcountinfo_get(tyrcountinfo,mernumber));
  }
  if (tyrsearchinfo->showmode & SHOWSEQUENCE)
  {
    ADDTABULATOR;
    fprintfsymbolstring(stdout,tyrsearchinfo->dnaalpha,qptr,
                        tyrsearchinfo->mersize);
  }
  if (tyrsearchinfo->showmode & (SHOWSEQUENCE | SHOWQPOS | SHOWCOUNTS))
  {
    (void) putchar('\n');
  }
}

static void singleseqtyrsearch(const Tyrindex *tyrindex,
                               const Tyrcountinfo *tyrcountinfo,
                               const Tyrsearchinfo *tyrsearchinfo,
                               uint64_t unitnum,
                               const Uchar *query,
                               unsigned long querylen,
                               GT_UNUSED const char *desc)
{
  const Uchar *qptr, *result;
  unsigned long skipvalue;

  if (tyrsearchinfo->mersize > querylen)
  {
    return;
  }
  qptr = query;
  while (qptr <= query + querylen - tyrsearchinfo->mersize)
  {
    skipvalue = containsspecialbytestring(qptr,tyrsearchinfo->mersize);
    if (skipvalue == tyrsearchinfo->mersize)
    {
      if (tyrsearchinfo->searchstrand & STRAND_FORWARD)
      {
        result = searchsinglemer(qptr,tyrindex,tyrsearchinfo);
        if (result != NULL)
        {
          mermatchoutput(tyrindex,
                         tyrcountinfo,
                         tyrsearchinfo,
                         result,
                         query,
                         qptr,
                         unitnum,
                         true);
        }
      }
      if (tyrsearchinfo->searchstrand & STRAND_REVERSE)
      {
        gt_assert(tyrsearchinfo->rcbuf != NULL);
        copy_reversecomplement(tyrsearchinfo->rcbuf,qptr,
                               tyrsearchinfo->mersize);
        result = searchsinglemer(tyrsearchinfo->rcbuf,tyrindex,
                                 tyrsearchinfo);
        if (result != NULL)
        {
          mermatchoutput(tyrindex,
                         tyrcountinfo,
                         tyrsearchinfo,
                         result,
                         query,
                         qptr,
                         unitnum,
                         false);
        }
      }
      qptr++;
    } else
    {
      qptr += (skipvalue+1);
    }
  }
}

int tyrsearch(const GtStr *tyrindexname,
              const GtStrArray *queryfilenames,
              unsigned int showmode,
              unsigned int searchstrand,
              bool verbose,
              bool performtest,
              GtError *err)
{
  Tyrindex *tyrindex;
  Tyrcountinfo *tyrcountinfo = NULL;
  bool haserr = false;
  GtSeqIterator *seqit;

  gt_error_check(err);
  tyrindex = tyrindex_new(tyrindexname,err);
  if (tyrindex == NULL)
  {
    haserr = true;
  } else
  {
    if (verbose)
    {
      tyrindex_show(tyrindex);
    }
    if (performtest)
    {
      tyrindex_check(tyrindex);
    }
  }
  if (!haserr)
  {
    gt_assert(tyrindex != NULL);
    if ((showmode & SHOWCOUNTS) && !tyrindex_isempty(tyrindex))
    {
      tyrcountinfo = tyrcountinfo_new(tyrindex,tyrindexname,err);
      if (tyrcountinfo == NULL)
      {
        haserr = true;
      }
    }
  }
  if (!haserr)
  {
    const Uchar *query;
    unsigned long querylen;
    char *desc = NULL;
    uint64_t unitnum;
    int retval;
    Tyrsearchinfo tyrsearchinfo;

    gt_assert(tyrindex != NULL);
    tyrsearchinfo_init(&tyrsearchinfo,tyrindex,showmode,searchstrand);
    seqit = gt_seqiterator_new(queryfilenames,
                               getsymbolmapAlphabet(tyrsearchinfo.dnaalpha),
                               true);
    for (unitnum = 0; /* Nothing */; unitnum++)
    {
      retval = gt_seqiterator_next(seqit,
                                   &query,
                                   &querylen,
                                   &desc,
                                   err);
      if (retval < 0)
      {
        haserr = true;
        break;
      }
      if (retval == 0)
      {
        break;
      }
      singleseqtyrsearch(tyrindex,
                         tyrcountinfo,
                         &tyrsearchinfo,
                         unitnum,
                         query,
                         querylen,
                         desc);
      gt_free(desc);
    }
    gt_seqiterator_delete(seqit);
    tyrsearchinfo_delete(&tyrsearchinfo);
  }
  if (tyrcountinfo != NULL)
  {
    tyrcountinfo_delete(&tyrcountinfo);
  }
  if (tyrindex != NULL)
  {
    tyrindex_delete(&tyrindex);
  }
  return haserr ? -1 : 0;
}
