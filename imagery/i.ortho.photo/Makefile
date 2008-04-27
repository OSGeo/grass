
MODULE_TOPDIR = ../..

SUBDIRS1 = \
	menu \
	photo.2image \
	photo.2target \
	photo.camera \
	photo.elev \
	photo.init \
	photo.rectify \
	photo.target

SUBDIRS = libes $(SUBDIRS1)

include $(MODULE_TOPDIR)/include/Make/Dir.make

$(SUBDIRS1): libes

default: parsubdirs

clean: cleansubdirs

