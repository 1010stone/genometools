#
# Copyright (c) 2008 Sascha Steinbiss <steinbiss@zbh.uni-hamburg.de>
# Copyright (c) 2008 Center for Bioinformatics, University of Hamburg
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

from gt.dlload import gtlib

class Str:
  def __init__(self, s = None):
    self.strg = gtlib.gt_str_new_cstr(s)
    self._as_parameter_ = self.strg

  def __del__(self):
    gtlib.gt_str_delete(self.strg)

  def __str__(self):
    return gtlib.gt_str_get(self.strg)

  def length(self):
    return gtlib.gt_str_length(self.strg)

  def get_mem(self):
    return gtlib.gt_str_get_mem(self.strg)

  def register(cls, gtlib):
    from ctypes import c_void_p, c_char_p, c_ulong
    gtlib.gt_str_new.restype  = c_void_p
    gtlib.gt_str_new_cstr.restype  = c_void_p
    gtlib.gt_str_new_cstr.argtypes = [c_char_p]
    gtlib.gt_str_get.restype  = c_char_p
    gtlib.gt_str_get_mem.restype  = c_void_p
    gtlib.gt_str_length.restype = c_ulong
  register = classmethod(register)
