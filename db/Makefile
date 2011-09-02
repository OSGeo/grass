
MODULE_TOPDIR = ..

SUBDIRS = \
	db.columns \
	db.connect \
	db.copy \
	db.createdb \
	db.databases \
	db.describe \
	db.drivers \
	db.dropdb \
	db.execute \
	db.login \
	db.select \
	db.tables

CLEAN_SUBDIRS = drivers

PGM = databaseintro

include $(MODULE_TOPDIR)/include/Make/Dir.make

default: htmldir

htmldir:
	$(MAKE) -C drivers
	$(MAKE) parsubdirs

