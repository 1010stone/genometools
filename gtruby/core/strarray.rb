#
# Copyright (c) 2007-2008 Gordon Gremme <gremme@zbh.uni-hamburg.de>
# Copyright (c) 2007-2008 Center for Bioinformatics, University of Hamburg
#
# Permission to use, copy, modify, and distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#

require 'gtdlload'

module GT
  extend DL::Importable
  gtdlload "libgenometools"
  extern "GtStrArray* gt_strarray_new()"
  extern "void gt_strarray_add_cstr(GtStrArray*, const char*)"
  extern "const char* gt_strarray_get(const GtStrArray*, unsigned long)"
  extern "unsigned long gt_strarray_size(const GtStrArray*)"
  extern "void gt_strarray_delete(GtStrArray*)"

  class StrArray
    attr_reader :strarray
    def initialize(strarray_ptr = GT.gt_strarray_new())
      @strarray = strarray_ptr
      @strarray.free = GT::symbol("gt_strarray_delete", "0P")
    end

    def add_list(list)
      list.each { |cstr| GT.gt_strarray_add_cstr(@strarray, cstr) }
    end

    def to_a
      strings = []
      1.upto(GT.gt_strarray_size(@strarray)) do |i|
        strings.push(GT.gt_strarray_get(@strarray, i-1))
      end
      strings
    end

    def to_ptr
      @strarray
    end
  end
end
