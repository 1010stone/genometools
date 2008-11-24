/*
  Copyright (c) 2003-2008 Gordon Gremme <gremme@zbh.uni-hamburg.de>
  Copyright (c) 2003-2008 Center for Bioinformatics, University of Hamburg

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

#include "gtt.h"
#include "core/array.h"
#include "core/array2dim.h"
#include "core/basename.h"
#include "core/bitpackarray.h"
#include "core/bitpackstring.h"
#include "core/bittab.h"
#include "core/bsearch.h"
#include "core/countingsort.h"
#include "core/cstr_table.h"
#include "core/disc_distri.h"
#include "core/dlist.h"
#include "core/dynbittab.h"
#include "core/grep.h"
#include "core/hashmap.h"
#include "core/hashtable.h"
#include "core/interval_tree.h"
#include "core/queue.h"
#include "core/splitter.h"
#include "core/tokenizer.h"
#include "extended/alignment.h"
#include "extended/evaluator.h"
#include "extended/feature_node.h"
#include "extended/genome_node_iterator.h"
#include "extended/gff3_escaping.h"
#include "extended/hmm.h"
#include "extended/luaserialize.h"
#include "extended/splicedseq.h"
#include "extended/string_matching.h"
#include "extended/tag_value_map.h"
#include "extended/union_find.h"
#include "extended/redblack.h"
#include "ltr/gt_ltrdigest.h"
#include "ltr/ltrelement.h"
#include "ltr/ppt.h"
#include "tools/gt_bed_to_gff3.h"
#include "tools/gt_bioseq.h"
#include "tools/gt_cds.h"
#include "tools/gt_chseqids.h"
#include "tools/gt_clean.h"
#include "tools/gt_csa.h"
#include "tools/gt_congruence.h"
#include "tools/gt_dev.h"
#include "tools/gt_eval.h"
#include "tools/gt_exercise.h"
#include "tools/gt_extractfeat.h"
#include "tools/gt_extractseq.h"
#include "tools/gt_filter.h"
#include "tools/gt_fingerprint.h"
#include "tools/gt_gff3.h"
#include "tools/gt_gff3validator.h"
#include "tools/gt_gff3_to_gtf.h"
#include "tools/gt_gtf_to_gff3.h"
#include "tools/gt_ltrharvest.h"
#include "tools/gt_matchingstatistics.h"
#include "tools/gt_merge.h"
#include "tools/gt_mgth.h"
#include "tools/gt_mkfmindex.h"
#include "tools/gt_mmapandread.h"
#include "tools/gt_mutate.h"
#include "tools/gt_packedindex.h"
#include "tools/gt_prebwt.h"
#include "tools/gt_seqfilter.h"
#include "tools/gt_sequniq.h"
#include "tools/gt_shredder.h"
#include "tools/gt_splitfasta.h"
#include "tools/gt_splicesiteinfo.h"
#include "tools/gt_stat.h"
#include "tools/gt_suffixerator.h"
#include "tools/gt_tallymer.h"
#include "tools/gt_tagerator.h"
#include "tools/gt_template.h"
#include "tools/gt_uniq.h"
#include "tools/gt_uniquesub.h"

#ifndef WITHOUT_CAIRO
#include "annotationsketch/block.h"
#include "annotationsketch/diagram.h"
#include "annotationsketch/feature_index.h"
#include "annotationsketch/feature_index_memory.h"
#include "annotationsketch/gt_sketch.h"
#include "annotationsketch/image_info.h"
#include "annotationsketch/track.h"
#include "annotationsketch/rec_map.h"
#include "annotationsketch/style.h"
#endif

GtToolbox* gtt_tools(void)
{
  GtToolbox *tools = gt_toolbox_new();

  /* add tools */
  gt_toolbox_add_tool(tools, "bed_to_gff3", gt_bed_to_gff3());
  gt_toolbox_add_tool(tools, "bioseq", gt_bioseq());
  gt_toolbox_add_tool(tools, "cds", gt_cds());
  gt_toolbox_add(tools, "chseqids", gt_chseqids);
  gt_toolbox_add(tools, "clean", gt_clean);
  gt_toolbox_add_tool(tools, "csa", gt_csa());
  gt_toolbox_add_tool(tools, "congruence", gt_congruence());
  gt_toolbox_add_tool(tools, "dev", gt_dev());
  gt_toolbox_add(tools, "eval", gt_eval);
  gt_toolbox_add_tool(tools, "exercise", gt_exercise());
  gt_toolbox_add_tool(tools, "extractfeat", gt_extractfeat());
  gt_toolbox_add_tool(tools, "extractseq", gt_extractseq());
  gt_toolbox_add_tool(tools, "filter", gt_filter());
  gt_toolbox_add_tool(tools, "fingerprint", gt_fingerprint());
  gt_toolbox_add_tool(tools, "gff3", gt_gff3());
  gt_toolbox_add_tool(tools, "gff3validator", gt_gff3validator());
  gt_toolbox_add(tools, "gff3_to_gtf", gt_gff3_to_gtf);
  gt_toolbox_add_tool(tools, "ltrdigest", gt_ltrdigest());
  gt_toolbox_add_tool(tools, "gtf_to_gff3", gt_gtf_to_gff3());
  gt_toolbox_add(tools, "ltrharvest", gt_ltrharvest);
  gt_toolbox_add(tools, "matstat", gt_matchingstatistics);
  gt_toolbox_add(tools, "merge", gt_merge);
  gt_toolbox_add(tools, "mgth", gt_mgth);
  gt_toolbox_add(tools, "mmapandread", gt_mmapandread);
  gt_toolbox_add_tool(tools, "mutate", gt_mutate());
  gt_toolbox_add(tools, "mkfmindex", gt_mkfmindex);
  gt_toolbox_add_tool(tools, "packedindex", gt_packedindex());
  gt_toolbox_add_tool(tools, "prebwt", gt_prebwt());
  gt_toolbox_add_tool(tools, "seqfilter", gt_seqfilter());
  gt_toolbox_add_tool(tools, "sequniq", gt_sequniq());
  gt_toolbox_add_tool(tools, "shredder", gt_shredder());
  gt_toolbox_add_tool(tools, "splitfasta", gt_splitfasta());
  gt_toolbox_add(tools, "splicesiteinfo", gt_splicesiteinfo);
  gt_toolbox_add(tools, "stat", gt_stat);
  gt_toolbox_add(tools, "suffixerator", gt_suffixerator);
  gt_toolbox_add_tool(tools, "tallymer", gt_tallymer());
  gt_toolbox_add_tool(tools, "tagerator", gt_tagerator());
  gt_toolbox_add_tool(tools, "template", gt_template());
  gt_toolbox_add(tools, "uniq", gt_uniq);
  gt_toolbox_add(tools, "uniquesub", gt_uniquesub);
#ifndef WITHOUT_CAIRO
  gt_toolbox_add(tools, "sketch", gt_sketch);
#endif

  return tools;
}

GtHashmap* gtt_unit_tests(void)
{
  GtHashmap *unit_tests = gt_hashmap_new(HASH_STRING, NULL, NULL);

  /* add unit tests */
  gt_hashmap_add(unit_tests, "alignment class", gt_alignment_unit_test);
  gt_hashmap_add(unit_tests, "array class", gt_array_unit_test);
  gt_hashmap_add(unit_tests, "array example", gt_array_example);
  gt_hashmap_add(unit_tests, "array2dim example", gt_array2dim_example);
  gt_hashmap_add(unit_tests, "basename module", gt_basename_unit_test);
  gt_hashmap_add(unit_tests, "bit pack array class", gt_bitpackarray_unit_test);
  gt_hashmap_add(unit_tests, "bit pack string module",
                 gt_bitPackString_unit_test);
  gt_hashmap_add(unit_tests, "bittab class", gt_bittab_unit_test);
  gt_hashmap_add(unit_tests, "bittab example", gt_bittab_example);
  gt_hashmap_add(unit_tests, "bsearch module", gt_bsearch_unit_test);
  gt_hashmap_add(unit_tests, "countingsort module", gt_countingsort_unit_test);
  gt_hashmap_add(unit_tests, "cstr table class", gt_cstr_table_unit_test);
  gt_hashmap_add(unit_tests, "disc distri class", gt_disc_distri_unit_test);
  gt_hashmap_add(unit_tests, "dlist class", gt_dlist_unit_test);
  gt_hashmap_add(unit_tests, "dlist example", gt_dlist_example);
  gt_hashmap_add(unit_tests, "dynamic bittab class", gt_dynbittab_unit_test);
  gt_hashmap_add(unit_tests, "evaluator class", gt_evaluator_unit_test);
  gt_hashmap_add(unit_tests, "genome feature class", gt_feature_node_unit_test);
  gt_hashmap_add(unit_tests, "genome node iterator example",
                 gt_genome_node_iterator_example);
  gt_hashmap_add(unit_tests, "gff3 escaping module",
                 gt_gff3_escaping_unit_test);
  gt_hashmap_add(unit_tests, "grep module", gt_grep_unit_test);
  gt_hashmap_add(unit_tests, "hashmap class", gt_hashmap_unit_test);
  gt_hashmap_add(unit_tests, "hashtable class", gt_hashtable_unit_test);
  gt_hashmap_add(unit_tests, "hmm class", gt_hmm_unit_test);
  gt_hashmap_add(unit_tests, "interval tree class", gt_interval_tree_unit_test);
  gt_hashmap_add(unit_tests, "Lua serializer module",
                 gt_lua_serializer_unit_test);
  gt_hashmap_add(unit_tests, "ltrelement module", gt_ltrelement_unit_test);
  gt_hashmap_add(unit_tests, "PPT module", gt_ppt_unit_test);
  gt_hashmap_add(unit_tests, "queue class", gt_queue_unit_test);
  gt_hashmap_add(unit_tests, "range class", gt_range_unit_test);
  gt_hashmap_add(unit_tests, "red-black tree class", gt_rbt_unit_test);
  gt_hashmap_add(unit_tests, "safearith module", gt_safearith_unit_test);
  gt_hashmap_add(unit_tests, "safearith example", gt_safearith_example);
  gt_hashmap_add(unit_tests, "splicedseq class", gt_splicedseq_unit_test);
  gt_hashmap_add(unit_tests, "splitter class", gt_splitter_unit_test);
  gt_hashmap_add(unit_tests, "string class", gt_str_unit_test);
  gt_hashmap_add(unit_tests, "string matching module",
                 gt_string_matching_unit_test);
  gt_hashmap_add(unit_tests, "tag value map example", gt_tag_value_map_example);
  gt_hashmap_add(unit_tests, "tokenizer class", gt_tokenizer_unit_test);
  gt_hashmap_add(unit_tests, "union find class", gt_union_find_unit_test);
#ifndef WITHOUT_CAIRO
  gt_hashmap_add(unit_tests, "block class", gt_block_unit_test);
  gt_hashmap_add(unit_tests, "style class", gt_style_unit_test);
/*  gt_hashmap_add(unit_tests, "diagram class", gt_diagram_unit_test); */
  gt_hashmap_add(unit_tests, "element class", gt_element_unit_test);
  gt_hashmap_add(unit_tests, "memory feature index class",
                 gt_feature_index_memory_unit_test);
  gt_hashmap_add(unit_tests, "imageinfo class", gt_image_info_unit_test);
  gt_hashmap_add(unit_tests, "line class", gt_line_unit_test);
  gt_hashmap_add(unit_tests, "track class", gt_track_unit_test);
#endif

  return unit_tests;
}
