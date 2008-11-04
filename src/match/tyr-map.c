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
#include "encseq-def.h"
#include "divmodmul.h"
#include "defined-types.h"
#include "opensfxfile.h"
#include "tyr-basic.h"
#include "tyr-map.h"
#include "spacedef.h"

#include "sfx-apfxlen.pr"

struct Tyrindex
{
  void *mappedfileptr;
  const GtStr *indexfilename;
  unsigned int alphasize;
  size_t numofmers;
  unsigned long mersize,
                merbytes;
  Uchar *mertable,
        *lastmer;
};

struct Tyrcountinfo
{
  void *mappedmctfileptr;
  const GtStr *indexfilename;
  Uchar *smallcounts;
  Largecount *largecounts;
  unsigned long numoflargecounts;
};

typedef struct
{
  Uchar *leftmer,
        *rightmer;
} Merbounds;

unsigned long decodesingleinteger(const Uchar *start)
{
  unsigned long idx, value;

  value = (unsigned long) start[0];
  for (idx=1UL; idx < (unsigned long) sizeof (unsigned long); idx++)
  {
    value |= (((unsigned long) start[idx]) << MULT8(idx));
  }
  return value;
}

Tyrindex *tyrindex_new(const GtStr *tyrindexname,GtError *err)
{
  bool haserr = false;
  size_t numofbytes, rest;
  Tyrindex *tyrindex;

  gt_error_check(err);
  ALLOCASSIGNSPACE(tyrindex,NULL,Tyrindex,1);
  tyrindex->indexfilename = tyrindexname;
  tyrindex->mappedfileptr = genericmaponlytable(tyrindexname,
                                                MERSUFFIX,&numofbytes,err);
  if (tyrindex->mappedfileptr == NULL)
  {
    haserr = true;
  }
  if (!haserr)
  {
    tyrindex->mertable = (Uchar *) tyrindex->mappedfileptr;
    rest = sizeof (unsigned long) * EXTRAINTEGERS;
    if (rest > numofbytes)
    {
      gt_error_set(err,"index must contain at least %lu bytes",
                        (unsigned long) rest);
      haserr = true;
    }
  }
  if (!haserr)
  {
    gt_assert(tyrindex->mertable != NULL);
    tyrindex->mersize
      = decodesingleinteger(tyrindex->mertable + numofbytes - rest);
    tyrindex->alphasize
      = (unsigned int) decodesingleinteger(tyrindex->mertable +
                                           numofbytes - rest +
                                           sizeof (unsigned long));
    tyrindex->merbytes = MERBYTES(tyrindex->mersize);
    if ((numofbytes - rest) % tyrindex->merbytes != 0)
    {
      gt_error_set(err,"size of index is %lu which is not a multiple of %lu",
                   (unsigned long) (numofbytes - rest),
                   tyrindex->merbytes);
      haserr = true;
    }
  }
  if (!haserr)
  {
    tyrindex->numofmers = (numofbytes - rest) / tyrindex->merbytes;
    gt_assert(tyrindex->mertable != NULL);
    if (tyrindex->numofmers == 0)
    {
      tyrindex->lastmer = tyrindex->mertable - 1;
    } else
    {
      tyrindex->lastmer
        = tyrindex->mertable + numofbytes - rest - tyrindex->merbytes;
    }
  }
  if (haserr)
  {
    if (tyrindex->mappedfileptr != NULL)
    {
      gt_fa_xmunmap(tyrindex->mappedfileptr);
      tyrindex->mappedfileptr = NULL;
    }
    FREESPACE(tyrindex);
  }
  return haserr ? NULL : tyrindex;
}

void tyrindex_show(const Tyrindex *tyrindex)
{
  printf("# indexfilename = %s\n",gt_str_get(tyrindex->indexfilename));
  printf("# alphasize = %u\n",tyrindex->alphasize);
  printf("# mersize = %lu\n",tyrindex->mersize);
  printf("# numofmers = %lu\n",(unsigned long) tyrindex->numofmers);
  printf("# merbytes = %lu\n",tyrindex->merbytes);
}

bool tyrindex_isempty(const Tyrindex *tyrindex)
{
  return tyrindex->numofmers == 0 ? true : false;
}

const Uchar *tyrindex_mertable(const Tyrindex *tyrindex)
{
  return tyrindex->mertable;
}

const Uchar *tyrindex_lastmer(const Tyrindex *tyrindex)
{
  return tyrindex->lastmer;
}

unsigned long tyrindex_merbytes(const Tyrindex *tyrindex)
{
  return tyrindex->merbytes;
}

unsigned long tyrindex_mersize(const Tyrindex *tyrindex)
{
  return tyrindex->mersize;
}

unsigned int tyrindex_alphasize(const Tyrindex *tyrindex)
{
  return tyrindex->alphasize;
}

unsigned long tyrindex_ptr2number(const Tyrindex *tyrindex,const Uchar *result)
{
  return (unsigned long) (result - tyrindex->mertable)/tyrindex->merbytes;
}

void tyrindex_delete(Tyrindex **tyrindexptr)
{
  Tyrindex *tyrindex = *tyrindexptr;

  gt_fa_xmunmap(tyrindex->mappedfileptr);
  tyrindex->mappedfileptr = NULL;
  FREESPACE(tyrindex);
  *tyrindexptr = NULL;
}

Tyrcountinfo *tyrcountinfo_new(const Tyrindex *tyrindex,
                               const GtStr *tyrindexname,
                               GtError *err)
{
  size_t numofbytes;
  void *tmp;
  bool haserr = false;
  Tyrcountinfo *tyrcountinfo;

  gt_error_check(err);
  ALLOCASSIGNSPACE(tyrcountinfo,NULL,Tyrcountinfo,1);
  tyrcountinfo->indexfilename = tyrindexname;
  tyrcountinfo->mappedmctfileptr
    = genericmaponlytable(tyrindexname,COUNTSSUFFIX,&numofbytes,err);
  if (tyrcountinfo->mappedmctfileptr == NULL)
  {
    tyrcountinfo->smallcounts = NULL;
    haserr = true;
  } else
  {
    tyrcountinfo->smallcounts
      = (Uchar *) tyrcountinfo->mappedmctfileptr;
    tmp = &tyrcountinfo->smallcounts[tyrindex->numofmers];
    tyrcountinfo->largecounts = (Largecount *) tmp;
    if (numofbytes < tyrindex->numofmers)
    {
      gt_error_set(err,"size of file \"%s.%s\" is smaller than minimum size "
                       "%lu",gt_str_get(tyrindexname),COUNTSSUFFIX,
                       (unsigned long) tyrindex->numofmers);
      haserr = true;
    }
  }
  if (!haserr && (numofbytes - tyrindex->numofmers) % sizeof (Largecount) != 0)
  {
    gt_error_set(err,"(numofbytes - numofmers) = %lu must be a multiple of %lu",
           (unsigned long) (numofbytes - tyrindex->numofmers),
           (unsigned long) sizeof (Largecount));
    haserr = true;
  }
  if (!haserr)
  {
    tyrcountinfo->numoflargecounts
      = (unsigned long) (numofbytes - tyrindex->numofmers)/
        (unsigned long) sizeof (Largecount);
  }
  if (haserr)
  {
    if (tyrcountinfo->mappedmctfileptr != NULL)
    {
      gt_fa_xmunmap(tyrcountinfo->mappedmctfileptr);
      tyrcountinfo->mappedmctfileptr = NULL;
    }
    FREESPACE(tyrcountinfo);
  }
  return haserr ? NULL : tyrcountinfo;
}

static /*@null@*/ const Largecount *binsearchLargecount(unsigned long key,
                                                        const Largecount *left,
                                                        const Largecount *right)
{
  const Largecount *leftptr = left, *midptr, *rightptr = right;
  unsigned long len;

  while (leftptr<=rightptr)
  {
    len = (unsigned long) (rightptr-leftptr);
    midptr = leftptr + DIV2(len); /* halve len */
    if (key < midptr->idx)
    {
      rightptr = midptr-1;
    } else
    {
      if (key > midptr->idx)
      {
        leftptr = midptr + 1;
      } else
      {
        return midptr;
      }
    }
  }
  return NULL;
}

unsigned long tyrcountinfo_get(const Tyrcountinfo *tyrcountinfo,
                               unsigned long mernumber)
{
  if (tyrcountinfo->smallcounts[mernumber] == 0)
  {
    const Largecount *lc
      = binsearchLargecount(mernumber,
                            tyrcountinfo->largecounts,
                            tyrcountinfo->largecounts +
                            tyrcountinfo->numoflargecounts-1);
    gt_assert (lc != NULL);
    if (lc == NULL)
    {
      fprintf(stderr,"cannot find count for mer number %lu",mernumber);
      exit(EXIT_FAILURE);
    }
    return lc->value;
  }
  return (unsigned long) tyrcountinfo->smallcounts[mernumber];
}

void tyrcountinfo_delete(Tyrcountinfo **tyrcountinfoptr)
{
  Tyrcountinfo *tyrcountinfo = *tyrcountinfoptr;

  gt_fa_xmunmap(tyrcountinfo->mappedmctfileptr);
  tyrcountinfo->mappedmctfileptr = NULL;
  FREESPACE(tyrcountinfo);
  *tyrcountinfoptr = NULL;
}

static int mymemcmp(unsigned long *offset,const Uchar *s1,const Uchar *s2,
                    unsigned long len)
{
  unsigned long idx;

  for (idx=*offset; idx<len; idx++)
  {
    if (s1[idx] < s2[idx])
    {
      *offset = idx;
      return -1;
    }
    if (s1[idx] > s2[idx])
    {
      *offset = idx;
      return 1;
    }
  }
  return 0;
}

/*@null@*/ const Uchar *tyrindex_binmersearch(const Tyrindex *tyrindex,
                                              unsigned long offset,
                                              const Uchar *key)
{
  const Uchar *leftptr, *midptr, *rightptr;
  int cmpval;
  unsigned long leftlength = offset, rightlength = offset, len;

  leftptr = tyrindex->mertable;
  rightptr = tyrindex->lastmer;
  while (leftptr <= rightptr)
  {
    len = (unsigned long) (rightptr-leftptr)/MULT2(tyrindex->merbytes);
    midptr = leftptr + tyrindex->merbytes * len;
    cmpval = mymemcmp(&offset,midptr,key,tyrindex->merbytes);
    if (cmpval < 0)
    {
      leftptr = midptr + tyrindex->merbytes;
      leftlength = offset;
      if (offset > rightlength)
      {
        offset = rightlength;
      }
    } else
    {
      if (cmpval > 0)
      {
        rightptr = midptr - tyrindex->merbytes;
        rightlength = offset;
        if (offset > leftlength)
        {
          offset = leftlength;
        }
      } else
      {
        return midptr;
      }
    }
  }
  return NULL;
}

void tyrindex_check(const Tyrindex *tyrindex)
{
  Uchar *mercodeptr;
  const Uchar *result;
  unsigned long position, previousposition = 0;

  for (mercodeptr = tyrindex->mertable;
       mercodeptr <= tyrindex->lastmer;
       mercodeptr += tyrindex->merbytes)
  {
    result = tyrindex_binmersearch(tyrindex,0,mercodeptr);
    gt_assert(result != NULL);
    if ((result - tyrindex->mertable) % tyrindex->merbytes != 0)
    {
      fprintf(stderr,"result is not multiple of %lu\n",tyrindex->merbytes);
      exit(EXIT_FAILURE);
    }
    position = (unsigned long) (result-tyrindex->mertable)/
                               tyrindex->merbytes;
    if (position > 0)
    {
      if (previousposition + 1 != position)
      {
        fprintf(stderr,"position %lu is not increasing\n",position);
        exit(EXIT_FAILURE);
      }
    }
    previousposition = position;
  }
}

int determinetyrbckpfxlen(unsigned int *prefixlength,
                          const Tyrindex *tyrindex,
                          const Definedunsignedint *callprefixlength,
                          GtError *err)
{
  gt_error_check(err);
  if (callprefixlength->defined)
  {
    unsigned int maxprefixlen
      = whatisthemaximalprefixlength(tyrindex->alphasize,tyrindex->numofmers,0);
    if (maxprefixlen > (unsigned int) tyrindex->mersize)
    {
      maxprefixlen = (unsigned int) tyrindex->mersize;
    }
    if (checkprefixlength(maxprefixlen,callprefixlength->valueunsignedint,
                          err) != 0)
    {
      return -1;
    }
    *prefixlength = callprefixlength->valueunsignedint;
  } else
  {
    unsigned int recommended = recommendedprefixlength(tyrindex->alphasize,
                                                       tyrindex->numofmers);
    if (recommended > (unsigned int) tyrindex->mersize)
    {
      recommended = (unsigned int) tyrindex->mersize;
    }
    *prefixlength = recommended;
  }
  return 0;
}
