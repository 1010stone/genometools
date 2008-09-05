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

#ifndef MYERSAPM_H
#define MYERSAPM_H

#include "seqpos-def.h"
#include "alphadef.h"
#include "encseq-def.h"
#include "procmatch.h"

typedef struct Myersonlineresources Myersonlineresources;

Myersonlineresources *newMyersonlineresources(
                        unsigned int mapsize,
                        bool nowildcards,
                        const Encodedsequence *encseq,
                        Processmatch processmatch,
                        void *processmatchinfo);

void freeMyersonlineresources(Myersonlineresources **ptrmyersonlineresources);

void edistmyersbitvectorAPM(Myersonlineresources *mor,
                            const Uchar *pattern,
                            unsigned long patternlength,
                            bool rcmatch,
                            unsigned long maxdistance);

#endif
