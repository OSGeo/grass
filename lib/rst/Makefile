# SPDX-License-Identifier: GPL-2.0-or-later

MODULE_TOPDIR = ../..

SUBDIRS = data \
	qtree \
	interp_float

include $(MODULE_TOPDIR)/include/Make/Dir.make

default: parsubdirs

qtree: data

interp_float: data qtree
