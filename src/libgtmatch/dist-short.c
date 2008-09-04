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

#include <limits.h>
#include <assert.h>
#include "libgtcore/symboldef.h"
#include "libgtcore/chardef.h"
#include "encseq-def.h"
#include "defined-types.h"
#include "initeqsvec.h"
#include "dist-short.h"

#define DECLARELOCALVARS\
        unsigned long Pv = ~0UL,\
                      Mv = 0,\
                      Eq,\
                      Xv,\
                      Xh,\
                      Ph,\
                      Mh,\
                      Ebit = (1UL << (ulen-1)),\
                      distval = ulen

#define COMPUTENEWDIST(CC)\
        assert((CC) != (Uchar) SEPARATOR);\
        Eq = eqsvector[(unsigned long) (CC)];\
        Xv = Eq | Mv;\
        Xh = (((Eq & Pv) + Pv) ^ Pv) | Eq;\
        Ph = Mv | ~ (Xh | Pv);\
        Mh = Pv & Xh;\
        if (Pv & Ebit)\
        {\
          distval++;\
        } else\
        {\
          if (Mv & Ebit)\
          {\
            distval--;\
          }\
        }\
        /*\
           the original version of Myers included the statement\
           Ph <<= 1. As an effect, the first element of each column\
           was 0. We add a 1 to the shifted Ph vector. As a consequence,\
           we obtain an increment by 1 in the first column.\
        */\
        Ph = (Ph << 1) | 1UL;\
        Pv = (Mh << 1) | ~ (Xv | Ph);\
        Mv = Ph & Xv

unsigned long distanceofshortstrings(unsigned long *eqsvector,
                                     unsigned int alphasize,
                                     const Uchar *useq,
                                     unsigned long ulen,
                                     const Uchar *vseq,
                                     unsigned long vlen)
{
  DECLARELOCALVARS;
  const Uchar *vptr;

  initeqsvector(eqsvector,(unsigned long) alphasize,useq,ulen);
  for (vptr = vseq; vptr < vseq + vlen; vptr++)
  {
    COMPUTENEWDIST(*vptr);
  }
  return distval;
}

unsigned long reversesuffixmatch(unsigned long *eqsvector,
                                 unsigned int alphasize,
                                 const Uchar *useq,
                                 unsigned long ulen,
                                 const Uchar *vseq,
                                 unsigned long vlen,
                                 unsigned long maxdistance)
{
  DECLARELOCALVARS;
  const Uchar *vptr;

  initeqsvectorrev(eqsvector,(unsigned long) alphasize,useq,ulen);
  assert(maxdistance > 0);
  for (vptr = vseq + vlen - 1; vptr >= vseq; vptr--)
  {
    COMPUTENEWDIST(*vptr);
    if (distval <= maxdistance)
    {
      break;
    }
  }
  /* assert(distval <= maxdistance); */
  return (unsigned long) (vseq + vlen - vptr);
}

Definedunsignedlong forwardprefixmatch(const Encodedsequence *encseq,
                                       unsigned int alphasize,
                                       Seqpos startpos,
                                       bool nowildcards,
                                       unsigned long *eqsvector,
                                       const Uchar *useq,
                                       unsigned long ulen,
                                       unsigned long maxdistance)
{
  DECLARELOCALVARS;
  Seqpos pos, totallength = getencseqtotallength(encseq);
  Uchar cc;
  Definedunsignedlong result;

  initeqsvector(eqsvector,(unsigned long) alphasize,useq,ulen);
  assert(maxdistance > 0);
  for (pos = startpos; /* Nothing */; pos++)
  {
    /*
    if (pos - startpos > (Seqpos) (ulen + maxdistance))
    {
      fprintf(stderr,"pos=%lu,startpos=%lu,ulen=%lu,maxdistance=%lu\n",
             (unsigned long) pos,(unsigned long) startpos,ulen,maxdistance);
      exit(EXIT_FAILURE);
    }
    assert(pos - startpos <= (Seqpos) (ulen + maxdistance));
    */
    cc = getencodedchar(encseq,pos,Forwardmode);
    if (nowildcards && cc == (Uchar) WILDCARD)
    {
      result.defined = false;
      result.valueunsignedlong = 0;
      return result;
    }
    COMPUTENEWDIST(cc);
    if (distval <= maxdistance || pos == totallength-1)
    {
      break;
    }
  }
  result.defined = true;
  result.valueunsignedlong = (unsigned long) (pos - startpos + 1);
  return result;
}
