
MODULE_TOPDIR = ../..

#the order is relevant
SUBDIRS = \
	dbmi_base \
	dbmi_client \
	stubs \
	dbmi_driver \
	sqlp

#doxygen:
DOXNAME=dbmi

include $(MODULE_TOPDIR)/include/Make/Dir.make
include $(MODULE_TOPDIR)/include/Make/Doxygen.make

default: parsubdirs


stubs: dbmi_base

dbmi_client: dbmi_base

dbmi_driver: dbmi_base stubs
