/*
  Copyright (c) 2006-2007 Gordon Gremme <gremme@zbh.uni-hamburg.de>
  Copyright (c) 2006-2007 Center for Bioinformatics, University of Hamburg
  See LICENSE file or http://genometools.org/license.html for license details.
*/

#ifndef DISC_DISTRI_H
#define DISC_DISTRI_H

#include "env.h"

/* A descrete distribution */
typedef struct DiscDistri DiscDistri;

DiscDistri* disc_distri_new(Env*);
void        disc_distri_add(DiscDistri*, unsigned long, Env*);
void        disc_distri_show(const DiscDistri*); /* on stdout */
void        disc_distri_delete(DiscDistri*, Env*);

#endif
