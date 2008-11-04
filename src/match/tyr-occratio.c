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

#include <errno.h>
#include "core/str.h"
#include "core/unused_api.h"
#include "core/arraydef.h"
#include "core/fa.h"
#include "esa-seqread.h"
#include "alphadef.h"
#include "verbose-def.h"
#include "spacedef.h"
#include "format64.h"
#include "divmodmul.h"

struct Dfsinfo /* information stored for each node of the lcp interval tree */
{
  Seqpos leftmostleaf,
         rightmostleaf,
         suftabrightmostleaf,
         lcptabrightmostleafplus1;
};

DECLAREARRAYSTRUCT(uint64_t);

struct Dfsstate /* global information */
{
  const Encodedsequence *encseq;
  Readmode readmode;
  Seqpos totallength;
  unsigned long minmersize,
                maxmersize;
  const Bitstring *outputvector;
  Arrayuint64_t uniquedistribution,
                nonuniquedistribution,
                nonuniquemultidistribution;
};

#include "esa-dfs.h"

static Dfsinfo *allocateDfsinfo(GT_UNUSED Dfsstate *state)
{
  Dfsinfo *dfsinfo;

  ALLOCASSIGNSPACE(dfsinfo,NULL,Dfsinfo,1);
  return dfsinfo;
}

static void freeDfsinfo(Dfsinfo *dfsinfo, GT_UNUSED Dfsstate *state)
{
  FREESPACE(dfsinfo);
}

static void adddistributionuint64_t(Arrayuint64_t *occdistribution,
                                    unsigned long countocc,
                                    unsigned long value)
{
  if (countocc >= occdistribution->allocateduint64_t)
  {
    const unsigned long addamount = 128UL;
    unsigned long idx;

    ALLOCASSIGNSPACE(occdistribution->spaceuint64_t,
                     occdistribution->spaceuint64_t,
                     uint64_t,countocc+addamount);
    for (idx=occdistribution->allocateduint64_t;
         idx<countocc+addamount; idx++)
    {
      occdistribution->spaceuint64_t[idx] = 0;
    }
    occdistribution->allocateduint64_t = countocc+addamount;
  }
  if (countocc + 1 > occdistribution->nextfreeuint64_t)
  {
    occdistribution->nextfreeuint64_t = countocc+1;
  }
  occdistribution->spaceuint64_t[countocc] += value;
}

static void iteritvdistribution(Arrayuint64_t *distribution,
                                const Encodedsequence *encseq,
                                Readmode readmode,
                                Seqpos totallength,
                                unsigned long minmersize,
                                unsigned long maxmersize,
                                Seqpos length,
                                Seqpos startpos)
{

  if (length <= (Seqpos) maxmersize)
  {
    Seqpos ulen, pos;

    for (ulen = length,
         pos = startpos + length - 1;
         ulen <= (Seqpos) maxmersize &&
         pos < totallength &&
         ISNOTSPECIAL(getencodedchar(encseq,pos,readmode));
         pos++, ulen++)
    {
      if (ulen >= (Seqpos) minmersize)
      {
        adddistributionuint64_t(distribution,(unsigned long) ulen,1UL);
      }
    }
  }
}

static int processleafedge(GT_UNUSED bool firstsucc,
                           Seqpos fatherdepth,
                           GT_UNUSED Dfsinfo *father,
                           Seqpos leafnumber,
                           Dfsstate *state,
                           GtError *err)
{
  gt_error_check(err);
  iteritvdistribution(&state->uniquedistribution,
                      state->encseq,
                      state->readmode,
                      state->totallength,
                      state->minmersize,
                      state->maxmersize,
                      fatherdepth+1,
                      leafnumber);
  return 0;
}

static int processcompletenode(Seqpos nodeptrdepth,
                               Dfsinfo *nodeptr,
                               Seqpos nodeptrminusonedepth,
                               Dfsstate *state,
                               GtError *err)
{
  Seqpos fatherdepth;
  unsigned long startlength, endlength;

  gt_error_check(err);
  fatherdepth = nodeptr->lcptabrightmostleafplus1;
  if (fatherdepth < nodeptrminusonedepth)
  {
    fatherdepth = nodeptrminusonedepth;
  }
  startlength = (unsigned long) (fatherdepth + 1);
  if (startlength < state->minmersize)
  {
    startlength = state->minmersize;
  }
  endlength = (unsigned long) nodeptrdepth;
  if (endlength > state->maxmersize)
  {
    endlength = state->maxmersize;
  }
  if (startlength <= endlength)
  {
    unsigned long lenval;
    Seqpos occcount = nodeptr->rightmostleaf - nodeptr->leftmostleaf + 1;

    for (lenval = startlength; lenval <= endlength; lenval++)
    {
      adddistributionuint64_t(&state->nonuniquemultidistribution,
                              lenval,
                              (unsigned long) occcount);
      adddistributionuint64_t(&state->nonuniquedistribution,
                              lenval,
                              1UL);
    }
  }
  return 0;
}

static void assignleftmostleaf(Dfsinfo *dfsinfo,Seqpos leftmostleaf,
                               GT_UNUSED Dfsstate *dfsstate)
{
  dfsinfo->leftmostleaf = leftmostleaf;
}

static void assignrightmostleaf(Dfsinfo *dfsinfo,Seqpos currentindex,
                                Seqpos previoussuffix,Seqpos currentlcp,
                                GT_UNUSED Dfsstate *dfsstate)
{
  dfsinfo->rightmostleaf = currentindex;
  dfsinfo->suftabrightmostleaf = previoussuffix;
  dfsinfo->lcptabrightmostleafplus1 = currentlcp;
}

static int computeoccurrenceratio(Sequentialsuffixarrayreader *ssar,
                                  unsigned long minmersize,
                                  unsigned long maxmersize,
                                  const Bitstring *outputvector,
                                  Verboseinfo *verboseinfo,
                                  GtError *err)
{
  Dfsstate state;
  bool haserr = false;

  gt_error_check(err);
  state.encseq = encseqSequentialsuffixarrayreader(ssar);
  state.readmode = readmodeSequentialsuffixarrayreader(ssar);
  state.totallength = getencseqtotallength(state.encseq);
  state.minmersize = minmersize;
  state.maxmersize = maxmersize;
  state.outputvector = outputvector;
  INITARRAY(&state.uniquedistribution,uint64_t);
  INITARRAY(&state.nonuniquedistribution,uint64_t);
  INITARRAY(&state.nonuniquemultidistribution,uint64_t);
  if (depthfirstesa(ssar,
                    allocateDfsinfo,
                    freeDfsinfo,
                    processleafedge,
                    NULL,
                    processcompletenode,
                    assignleftmostleaf,
                    assignrightmostleaf,
                    &state,
                    verboseinfo,
                    err) != 0)
  {
    haserr = true;
  }
  FREEARRAY(&state.uniquedistribution,uint64_t);
  FREEARRAY(&state.nonuniquedistribution,uint64_t);
  FREEARRAY(&state.nonuniquemultidistribution,uint64_t);
  return haserr ? -1 : 0;
}

int tyr_occratio(const GtStr *str_inputindex,
                 bool scanfile,
                 unsigned long minmersize,
                 unsigned long maxmersize,
                 const Bitstring *outputvector,
                 Verboseinfo *verboseinfo,
                 GtError *err)
{
  bool haserr = false;
  Sequentialsuffixarrayreader *ssar;

  gt_error_check(err);
  ssar = newSequentialsuffixarrayreaderfromfile(str_inputindex,
                                                SARR_LCPTAB |
                                                SARR_SUFTAB |
                                                SARR_ESQTAB,
                                                scanfile ? SEQ_scan
                                                         : SEQ_mappedboth,
                                                err);
  if (ssar == NULL)
  {
    haserr = true;
  }
  if (!haserr)
  {
    if (computeoccurrenceratio(ssar,
                               minmersize,
                               maxmersize,
                               outputvector,
                               verboseinfo,
                               err) != 0)
    {
      haserr = true;
    }
  }
  if (ssar != NULL)
  {
    freeSequentialsuffixarrayreader(&ssar);
  }
  return haserr ? -1 : 0;
}
