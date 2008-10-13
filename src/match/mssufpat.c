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

#include <string.h>
#include <stdarg.h>
#include "core/assert_api.h"
#include "core/symboldef.h"
#include "core/unused_api.h"
#include "core/chardef.h"
#include "core/ma.h"
#include "intbits.h"
#include "seqpos-def.h"
#include "absdfstrans-imp.h"
#include "initeqsvec.h"

typedef struct
{
  unsigned long prefixofsuffixbits;
} Limdfsstate;

typedef struct
{
  unsigned long patternlength,
                mstatlength[INTWORDSIZE],
                *eqsvector;
  Seqpos mstatwitnessleftbound[INTWORDSIZE],
         mstatwitnessrightbound[INTWORDSIZE];
} Matchtaskinfo;

#ifdef SKDEBUG

static void pms_showLimdfsstate(const DECLAREPTRDFSSTATE(aliascol),
                                unsigned long depth,
                                const void *dfsconstinfo)
{
  const Matchtaskinfo *mti = (const Matchtaskinfo *) dfsconstinfo;
  const Limdfsstate *col = (const Limdfsstate *) aliascol;
  bool first = true;

  unsigned long idx, backmask;

  printf("at depth %lu: [",depth);
  for (idx=0, backmask = 1UL; idx<mti->patternlength; idx++, backmask <<= 1)
  {
    if (col->prefixofsuffixbits & backmask)
    {
      if (first)
      {
        printf("%lu",idx);
        first = false;
      } else
      {
        printf(",%lu",idx);
      }
    }
  }
  printf("]\n");
}

#endif

static void *pms_allocatedfsconstinfo(unsigned int alphasize)
{
  Matchtaskinfo *mti = gt_malloc(sizeof(Matchtaskinfo));
  mti->eqsvector = gt_malloc(sizeof(*mti->eqsvector) * alphasize);
  return mti;
}

static void pms_initdfsconstinfo(void *dfsconstinfo,
                                 unsigned int alphasize,
                                 ...)
                                 /* Variable argument list is as follows:
                                    const Uchar *pattern,
                                    unsigned long patternlength
                                 */
{
  va_list ap;
  const Uchar *pattern;
  Matchtaskinfo *mti = (Matchtaskinfo *) dfsconstinfo;

  va_start(ap,alphasize);
  pattern = va_arg(ap, const Uchar *);
  mti->patternlength = va_arg(ap, unsigned long);
  va_end(ap);
  initeqsvector(mti->eqsvector,(unsigned long) alphasize,
                pattern,mti->patternlength);
}

static void pms_extractdfsconstinfo(Processresult processresult,
                                    void *processinfo,
                                    const void *patterninfo,
                                    void *dfsconstinfo)
{
  unsigned long idx;
  Matchtaskinfo *mti = (Matchtaskinfo *) dfsconstinfo;

  for (idx=0; idx<mti->patternlength; idx++)
  {
    processresult(processinfo,patterninfo,idx,mti->mstatlength[idx],
                                              mti->mstatwitnessleftbound[idx],
                                              mti->mstatwitnessrightbound[idx]);
  }
}

static void pms_freedfsconstinfo(void **dfsconstinfo)
{
  Matchtaskinfo *mti = (Matchtaskinfo *) *dfsconstinfo;

  gt_free(mti->eqsvector);
  gt_free(mti);
  *dfsconstinfo = NULL;
}

static unsigned long zerosontheright(unsigned long v)
{
  unsigned long c;     /* c will be the number of zero bits on the right,
                         so if v is 1101000 (base 2), then c will be 3 */
  gt_assert(v > 0);
  if (v & 0x1)
  {
    c = 0; /* special case for odd v (assumed to happen half of the time) */
  } else
  {
    c = 1UL;
#ifdef _LP64
    if ((v & 0xffffffff) == 0)
    {
      v >>= 32;
      c+= 32UL;
    }
#endif
    if ((v & 0xffff) == 0)
    {
      v >>= 16;
      c += 16UL;
    }
    if ((v & 0xff) == 0)
    {
      v >>= 8;
      c += 8UL;
    }
    if ((v & 0xf) == 0)
    {
      v >>= 4;
      c += 4UL;
    }
    if ((v & 0x3) == 0)
    {
      v >>= 2;
      c += 2UL;
    }
    c -= v & 0x1;
  }
  return c;
}

static void pms_initLimdfsstate(DECLAREPTRDFSSTATE(aliascolumn),
                                void *dfsconstinfo)
{
  Limdfsstate *column = (Limdfsstate *) aliascolumn;
  Matchtaskinfo *mti = (Matchtaskinfo *) dfsconstinfo;
  unsigned long idx;

  column->prefixofsuffixbits = ~0UL;
  gt_assert(mti->patternlength <= (unsigned long) INTWORDSIZE);
  for (idx = 0; idx<mti->patternlength; idx++)
  {
    mti->mstatlength[idx] = 0;
    mti->mstatwitnessleftbound[idx] = 0;
    mti->mstatwitnessrightbound[idx] = 0;
  }
}

static unsigned long pms_fullmatchLimdfsstate(DECLAREPTRDFSSTATE(aliascolumn),
                                              Seqpos leftbound,
                                              Seqpos rightbound,
                                              GT_UNUSED Seqpos width,
                                              unsigned long currentdepth,
                                              void *dfsconstinfo)
{
  Limdfsstate *limdfsstate = (Limdfsstate *) aliascolumn;

  if (limdfsstate->prefixofsuffixbits > 0)
  {
    Matchtaskinfo *mti = (Matchtaskinfo *) dfsconstinfo;
    unsigned long bitindex = 0,
                  first1,
                  tmp = limdfsstate->prefixofsuffixbits;
    do
    {
      first1 = zerosontheright(tmp);
      gt_assert(bitindex + first1 < mti->patternlength);
      if (mti->mstatlength[bitindex+first1] < currentdepth)
      {
        /*
        printf("set mstatlength[%lu]=%lu\n",bitindex+first1,currentdepth);
        printf("set mstatwitnessleftbound[%lu]=%lu\n",bitindex+first1,
                                                 (unsigned long) leftbound);
        */
        mti->mstatlength[bitindex+first1] = currentdepth;
        mti->mstatwitnessleftbound[bitindex+first1] = leftbound;
        mti->mstatwitnessrightbound[bitindex+first1] = rightbound;
      }
      tmp >>= (first1+1);
      bitindex += (first1+1);
    } while (tmp != 0);
    return 1UL; /* continue with depth first traversal */
  }
  return 0; /* stop depth first traversal */
}

static void pms_nextLimdfsstate(const void *dfsconstinfo,
                                DECLAREPTRDFSSTATE(aliasoutcol),
                                unsigned long currentdepth,
                                Uchar currentchar,
                                const DECLAREPTRDFSSTATE(aliasincol))
{
#ifdef SKDEBUG
  char buffer1[32+1], buffer2[32+1];
#endif
  const Matchtaskinfo *mti = (const Matchtaskinfo *) dfsconstinfo;
  Limdfsstate *outcol = (Limdfsstate *) aliasoutcol;
  const Limdfsstate *incol = (const Limdfsstate *) aliasincol;

  gt_assert(ISNOTSPECIAL(currentchar));
  gt_assert(currentdepth > 0);

  if (currentdepth > 1UL)
  {
    outcol->prefixofsuffixbits
      = incol->prefixofsuffixbits &
        (mti->eqsvector[currentchar] >> (currentdepth-1));
  } else
  {
    outcol->prefixofsuffixbits = mti->eqsvector[currentchar];
  }
#ifdef SKDEBUG
  uint32_t2string(buffer1,(uint32_t) incol->prefixofsuffixbits);
  uint32_t2string(buffer2,(uint32_t) outcol->prefixofsuffixbits);
  printf("next(%s,%u,depth=%lu)->%s\n",buffer1,(unsigned int) currentchar,
                                       currentdepth,buffer2);
#endif
}

static void pms_inplacenextLimdfsstate(const void *dfsconstinfo,
                                       DECLAREPTRDFSSTATE(aliascol),
                                       unsigned long currentdepth,
                                       Uchar currentchar)
{
#ifdef SKDEBUG
  char buffer1[32+1], buffer2[32+1];
  unsigned long tmp;
#endif
  const Matchtaskinfo *mti = (const Matchtaskinfo *) dfsconstinfo;
  Limdfsstate *col = (Limdfsstate *) aliascol;

  gt_assert(ISNOTSPECIAL(currentchar));
#ifdef SKDEBUG
  tmp = col->prefixofsuffixbits;
#endif
  col->prefixofsuffixbits &= (mti->eqsvector[currentchar] >> (currentdepth-1));
#ifdef SKDEBUG
  uint32_t2string(buffer1,(uint32_t) tmp);
  uint32_t2string(buffer2,(uint32_t) col->prefixofsuffixbits);
  printf("inplacenext(%s,%u,%lu)->%s\n",buffer1,(unsigned int) currentchar,
                                        currentdepth,buffer2);
#endif
}

const AbstractDfstransformer *pms_AbstractDfstransformer(void)
{
  static const AbstractDfstransformer pms_adfst =
  {
    sizeof (Limdfsstate),
    pms_allocatedfsconstinfo,
    pms_initdfsconstinfo,
    pms_extractdfsconstinfo,
    pms_freedfsconstinfo,
    pms_initLimdfsstate,
    pms_fullmatchLimdfsstate,
    pms_nextLimdfsstate,
    pms_inplacenextLimdfsstate,
#ifdef SKDEBUG
    pms_showLimdfsstate,
#endif
  };
  return &pms_adfst;
}

/*
  define bitvector prefixofsuffixbits_{d} such that after processing a
  sequence v of length d we have: for all i\in[0,m-1]
  prefixofsuffixbits_{d}[i] is 1 iff P[i..i+d-1] = v[0..d-1]

  Let eqsvector_{a} be a vector of size m such that
  eqsvector_{a}[i]=1 if P[i]=a

  Let d=0 (i.e. at the root). Then
  P[i..i+d-1]=P[i..i-1]=\varepsilon=v[0..-1]=v[0..d-1] for all i \in[0..m-1]
  and hence prefixofsuffixbits_{d}[i]=1. In other words
  prefixofsuffixbits_{d} = 1^{m}.

  Now suppose d > 0 and assume we have computed
  prefixofsuffixbits_{d-1}. Then by definition
  prefixofsuffixbits_{d}[i]
    iff P[i..i+d-1] = v[0..d-1]
    iff P[i..i+d-2] = v[0..d-2] && P[i+d-1]=v[d-1]
    iff prefixofsuffixbits_{d-1][i]=1 && eqsvector_{v[d-1]}[i+d-1]=1
    iff prefixofsuffixbits_{d-1][i] & eqsvector_{v[d-1]}[i+d-1]

  All values in prefixofsuffixbits_{d} are independent and can be computed
  in parallel by

  prefixofsuffixbits_{d} = prefixofsuffixbits_{d-1} & (eqsvector_{a} << (d-1))
  where a=v[d-1]

  prefixofsuffixbits_{d] = 0 and
  prefixofsuffixbits_{d-1} != 0 then  for all i satisfying
  prefixofsuffixbits_{d-1][i] = 1 do:
    if mstats[i]<d then mstats[i]=d and store first suffixposition of current
    interval.
*/
