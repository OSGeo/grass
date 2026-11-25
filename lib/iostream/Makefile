# SPDX-License-Identifier: GPL-2.0-or-later
MODULE_TOPDIR = ../..

LIB = IOSTREAM

IS_CXX = yes

include $(MODULE_TOPDIR)/include/Make/Lib.make

ifneq ($(strip $(CXX)),)
ifneq ($(strip $(SHLIB_LDX)),)
default: lib
else
default: stlib
endif
endif
