MODULE_TOPDIR = ../..

PGM = vectorascii

SUBDIRS = rtree \
	dglib \
	diglib \
	Vlib \
	vedit \
	neta

include $(MODULE_TOPDIR)/include/Make/Dir.make
include $(MODULE_TOPDIR)/include/Make/Doxygen.make

DOXNAME = vector

default: htmldir

htmldir: parsubdirs

diglib: rtree

Vlib: rtree dglib diglib

vedit: Vlib

neta: Vlib
