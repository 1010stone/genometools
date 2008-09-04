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

#include "libgtcore/chardef.h"
#include "libgtcore/symboldef.h"
#include "seqpos-def.h"
#include "spacedef.h"
#include "encseq-def.h"
#include "myersapm.h"
#include "defined-types.h"
#include "alphadef.h"
#include "procmatch.h"
#include "dist-short.h"
#include "initeqsvec.h"

struct Myersonlineresources
{
  Encodedsequencescanstate *esr;
  const Encodedsequence *encseq;
  Seqpos totallength;
  unsigned long *eqsvectorrev,
                *eqsvector;
  unsigned int alphasize;
  bool nowildcards;
  Processmatch processmatch;
  void *processmatchinfo;
};

Myersonlineresources *newMyersonlineresources(
                            unsigned int mapsize,
                            bool nowildcards,
                            const Encodedsequence *encseq,
                            Processmatch processmatch,
                            void *processmatchinfo)
{
  Myersonlineresources *mor;

  ALLOCASSIGNSPACE(mor,NULL,Myersonlineresources,1);
  ALLOCASSIGNSPACE(mor->eqsvectorrev,NULL,unsigned long,mapsize-1);
  ALLOCASSIGNSPACE(mor->eqsvector,NULL,unsigned long,mapsize-1);
  mor->encseq = encseq;
  mor->esr = newEncodedsequencescanstate();
  assert(mapsize > 0 && mapsize-1 <= MAXALPHABETCHARACTER);
  mor->alphasize = mapsize-1;
  mor->totallength = getencseqtotallength(encseq);
  mor->nowildcards = nowildcards;
  mor->processmatch = processmatch;
  mor->processmatchinfo = processmatchinfo;
  return mor;
}

void freeMyersonlineresources(Myersonlineresources **ptrmyersonlineresources)
{
  Myersonlineresources *mor = *ptrmyersonlineresources;

  FREESPACE(mor->eqsvectorrev);
  FREESPACE(mor->eqsvector);
  freeEncodedsequencescanstate(&mor->esr);
  FREESPACE(*ptrmyersonlineresources);
}

void edistmyersbitvectorAPM(Myersonlineresources *mor,
                            const Uchar *pattern,
                            unsigned long patternlength,
                            bool rcmatch,
                            unsigned long maxdistance)
{
  unsigned long Pv = ~0UL,
                Mv = 0UL,
                Eq,
                Xv,
                Xh,
                Ph,
                Mh,
                score;
  const unsigned long Ebit = 1UL << (patternlength-1);
  Uchar cc;
  Seqpos pos;
  const Readmode readmode = Reversemode;

  initeqsvectorrev(mor->eqsvectorrev,
                   (unsigned long) mor->alphasize,
                   pattern,patternlength);
  score = patternlength;
  initEncodedsequencescanstate(mor->esr,
                               mor->encseq,
                               readmode,
                               0);
  for (pos = 0; pos < mor->totallength; pos++)
  {
    cc = sequentialgetencodedchar(mor->encseq,
                                  mor->esr,
                                  pos,
                                  readmode);
    if (cc == (Uchar) SEPARATOR)
    {
      Pv = ~0UL;
      Mv = 0UL;
      score = patternlength;
    } else
    {
      if (cc == (Uchar) WILDCARD)
      {
        Eq = 0;
      } else
      {
        Eq = mor->eqsvectorrev[(unsigned long) cc];   /*  6 */
      }
      Xv = Eq | Mv;                                   /*  7 */
      Xh = (((Eq & Pv) + Pv) ^ Pv) | Eq;              /*  8 */

      Ph = Mv | ~ (Xh | Pv);                          /*  9 */
      Mh = Pv & Xh;                                   /* 10 */

      if (Ph & Ebit)
      {
        score++;
      } else
      {
        if (Mh & Ebit)
        {
          assert(score > 0);
          score--;
        }
      }

      Ph <<= 1;                                       /* 15 */
      Pv = (Mh << 1) | ~ (Xv | Ph);                   /* 17 */
      Mv = Ph & Xv;                                   /* 18 */
      if (score <= maxdistance)
      {
        Seqpos dbstartpos = REVERSEPOS(mor->totallength,pos);
        Definedunsignedlong matchlength;

        /*
        matchlength = apm_findshortestmatchforward(mor->encseq,
                                                   mor->nowildcards,
                                                   mor->alphasize,
                                                   pattern,
                                                   patternlength,
                                                   maxdistance,
                                                   dbstartpos);
        */
        if (maxdistance > 0)
        {
          matchlength = forwardprefixmatch(mor->encseq,
                                           mor->alphasize,
                                           dbstartpos,
                                           mor->nowildcards,
                                           mor->eqsvector,
                                           pattern,
                                           patternlength,
                                           maxdistance);
        } else
        {
          matchlength.defined = true;
          matchlength.valueunsignedlong = patternlength;
        }
        assert (matchlength.defined || mor->nowildcards);
        if (matchlength.defined)
        {
          mor->processmatch(mor->processmatchinfo,
                            rcmatch,
                            dbstartpos,
                            (Seqpos) matchlength.valueunsignedlong,
                            NULL,
                            patternlength);
        }
      }
    }
  }
}
