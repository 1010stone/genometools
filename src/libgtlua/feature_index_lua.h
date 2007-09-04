/*
  Copyright (c) 2007 Gordon Gremme <gremme@zbh.uni-hamburg.de>
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

#ifndef FEATURE_INDEX_LUA_H
#define FEATURE_INDEX_LUA_H

#include "lua.h"

/* exports the FeatureIndex class to Lua:

   feature_index    = gt.feature_index_new()
   -- returns the genome features (of type genome_node) in a table
   table            = feature_index:get_features_for_seqid(string)
   table            = feature_index:get_features_for_range(seqid,
                                                           startpos, endpos)
   seqid            = feature_index:get_first_seqid()
   startpos, endpos = feature_index:get_range_for_seqid(seqid)
*/
int luaopen_feature_index(lua_State*);

#define FEATURE_INDEX_METATABLE  "GenomeTools.feature_index"
#define check_feature_index(L, POS) \
          (FeatureIndex**) luaL_checkudata(L, POS, FEATURE_INDEX_METATABLE)

#endif
