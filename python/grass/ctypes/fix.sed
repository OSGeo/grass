#!/usr/bin/sed -f
/^# End loader$/a\
from .ctypes_preamble import *\
from .ctypes_preamble import _variadic_function\
from .ctypes_loader import *
/^# Begin preamble$/,/^# End preamble$/d
/^# Begin loader$/,/^# End loader$/d
