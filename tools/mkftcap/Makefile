MODULE_TOPDIR = ../..

PGM = mkftcap

CAPFILE = $(ETC)/freetypecap

include $(MODULE_TOPDIR)/include/Make/Script.make
include $(MODULE_TOPDIR)/include/Make/NoHtml.make

default: script

$(CAPFILE): $(SCRIPTDIR)/$(PGM)
	$< > $@
