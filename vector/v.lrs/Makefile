
MODULE_TOPDIR = ../..

SUBDIRS1 = \
	v.lrs.create \
	v.lrs.segment \
	v.lrs.label \
	v.lrs.where

SUBDIRS = lib $(SUBDIRS1)

#to install documentation:
PGM = lrs

include $(MODULE_TOPDIR)/include/Make/Dir.make

default: parsubdirs htmldir


$(SUBDIRS1): lib

