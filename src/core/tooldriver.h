/*
  Copyright (c) 2007-2008 Gordon Gremme <gremme@zbh.uni-hamburg.de>
  Copyright (c) 2007-2008 Center for Bioinformatics, University of Hamburg

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

#ifndef TOOLDRIVER_H
#define TOOLDRIVER_H

#include "core/error.h"
#include "core/tool.h"

/* The tool driver module allows to compile a tool into a separate binary. This
   is mostly useful for legacy applications like GenomeThreader.
   The tool driver creates an GtError object, calls <tool>, and reports errors.
   See below for example code to create a separate binary for the eval tool.
   XXX: change example to reflect the real gth application
*/
int gt_tooldriver(int(*tool)(int argc, const char **argv, GtError*),
               int argc, char *argv[]);

int toolobjdriver(GtToolConstructor, int argc, char *argv[]);

#if 0

#include "core/tooldriver.h"
#include "tools/gt_gff3.h"

int main(int argc, char *argv[])
{
  return gt_tooldriver(gt_gff3, argc, argv);
}

#endif

#endif
