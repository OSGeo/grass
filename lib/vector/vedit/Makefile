# SPDX-License-Identifier: GPL-2.0-or-later
MODULE_TOPDIR = ../../..

LIB = VEDIT

DEPENDENCIES = $(VECTORDEP) $(GISDEP)

EXTRA_INC = $(VECT_INC)
EXTRA_CFLAGS = $(VECT_CFLAGS)

include $(MODULE_TOPDIR)/include/Make/Lib.make
include $(MODULE_TOPDIR)/include/Make/Doxygen.make

default: lib

# doxygen
DOXNAME=vedit
