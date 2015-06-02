#ifndef FT_FRONT_PRUNE_H
#define FT_FRONT_PRUNE_H
#include "core/types_api.h"
#include "ft-polish.h"
#include "ft-front-generation.h"
#include "core/encseq_api.h"

#define WITHCACHE

#ifndef OUTSIDE_OF_GT
#define WITHCACHE
#endif

#ifdef WITHCACHE
typedef struct
{
  void *space;
  GtUword offset, allocated;
} GtAllocatedMemory;
#endif

#ifndef OUTSIDE_OF_GT
typedef struct
{
  const GtEncseq *encseq;
  GtAllocatedMemory *sequence_cache;
  GtEncseqReader *encseq_r;
} FTsequenceResources;
#endif

GtUword front_prune_edist_inplace(
#ifndef OUTSIDE_OF_GT
                       bool forward,
                       GtAllocatedMemory *frontspace_reservoir,
#endif
                       Polished_point *best_polished_point,
                       Fronttrace *fronttrace,
                       const Polishing_info *pol_info,
                       GtUword history,
                       GtUword minmatchnum,
                       GtUword maxalignedlendifference,
                       FTsequenceResources *ufsr,
                       GtUword ustart,
                       GtUword uulen,
                       FTsequenceResources *vfsr,
                       GtUword vstart,
                       GtUword vlen);

#endif
