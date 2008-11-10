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

from gt.dlload import gtlib, CollectFunc
from gt.annotationsketch.color import Color
from gt.core.error import Error, gterror
from gt.core.str import Str
from gt.core.str_array import StrArray
from gt.extended.genome_node import GenomeNode

class Style:
  def __init__(self):
    e = Error()
    self.style = gtlib.gt_style_new(False, e)
    if self.style == 0 or self.style == None:
      gterror(e)
    self._as_parameter_ = self.style

  def __del__(self):
    try:
      gtlib.gt_style_delete(self.style)
    except AttributeError:
      pass

  def from_param(cls, obj):
    if not isinstance(obj, Style):
      raise TypeError, "argument must be a Style"
    return obj._as_parameter_
  from_param = classmethod(from_param)

  def load_file(self, filename):
      err = Error()
      rval = gtlib.gt_style_load_file(self.style, filename, err)
      if rval != 0:
        gterror(err)

  def load_str(self, string):
    err = Error()
    strg = Str(string)
    rval = gtlib.gt_style_load_str(self.style, strg, err)
    if rval != 0:
        gterror(err)

  def to_str(self):
    err = Error()
    string = Str()
    if gtlib.gt_style_to_str(self.style, string, err) == 0:
      return str(string)
    else:
      gterror(err)

  def clone(self):
    sty = Style()
    str = self.to_str()
    sty.load_str(str)
    return sty

  def get_color(self, section, key, gn = None):
    from ctypes import byref
    color = Color()
    if gtlib.gt_style_get_color(self.style, section, key, byref(color), gn) == 1:
      return color
    else:
      return None

  def set_color(self, section, key, color):
    from ctypes import byref
    gtlib.gt_style_set_color(self.style, section, key, byref(color))

  def get_cstr(self, section, key, gn = None):
    string = Str()
    if gtlib.gt_style_get_str(self.style, section, key, string, gn) == 1:
      return str(string)
    else:
      return None

  def set_cstr(self, section, key, value):
    string = Str(value)
    gtlib.gt_style_set_str(self.style, section, key, string)

  def get_num(self, section, key, gn = None):
    from ctypes import c_double, byref
    double = c_double()
    if gtlib.gt_style_get_num(self.style, section, key, byref(double), gn) == 1:
      return double.value
    else:
      return None

  def set_num(self, section, key, number):
    from ctypes import c_double
    num = c_double(number)
    gtlib.gt_style_set_num(self.style, section, key, num)

  def get_bool(self, section, key, gn = None):
    from ctypes import byref, c_int
    bool = c_int()
    if gtlib.gt_style_get_bool(self.style, section, key, byref(bool), gn) == 1:
      if bool.value == 1:
        return True
      else:
        return False
    else:
      return None

  def set_bool(self, section, key, val):
    if val == True:
      gtlib.gt_style_set_bool(self.style, section, key, 1)
    else:
      gtlib.gt_style_set_bool(self.style, section, key, 0)

  def unset(self, section, key):
    gtlib.gt_style_unset(self.style, section, key)

  def register(cls, gtlib):
    from ctypes import c_char_p, c_double, c_float, c_void_p, POINTER, c_int
    gtlib.gt_style_get_bool.restype = c_int
    gtlib.gt_style_get_bool.argtypes = [c_void_p, c_char_p, c_char_p, \
                                        POINTER(c_int), c_void_p]
    gtlib.gt_style_get_num.restype = c_int
    gtlib.gt_style_get_num.argtypes = [c_void_p, c_char_p, c_char_p, \
                                       POINTER(c_double), c_void_p]
    gtlib.gt_style_get_str.restype = c_int
    gtlib.gt_style_get_str.argtypes = [c_void_p, c_char_p, c_char_p, \
                                       Str, c_void_p]
    gtlib.gt_style_get_color.restype = c_int
    gtlib.gt_style_get_color.argtypes = [c_void_p, c_char_p, c_char_p, \
                                         POINTER(Color), c_void_p]
    gtlib.gt_style_load_str.argtypes = [c_void_p, Str, Error]
    gtlib.gt_style_load_file.argtypes = [c_void_p, c_char_p, Error]
    gtlib.gt_style_unset.argtypes = [c_void_p, c_char_p, c_char_p]
    gtlib.gt_style_to_str.argtypes = [c_void_p, Str, Error]
  register = classmethod(register)
