MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Dir.make

# FIXME: adjust so SRCFILES doesn't install windows_launch.bat
SRCFILES := $(wildcard *.*)
DSTFILES := $(patsubst %,$(ETC)/gui/scripts/%,$(SRCFILES))

ifdef MINGW
SCRIPT_ACTIONS += create.bat
endif

default: $(SCRIPT_ACTIONS)
	if [ ! -d $(ETC)/gui ] ; then $(MKDIR) $(ETC)/gui ; fi
	if [ ! -d $(ETC)/gui/scripts ] ; then $(MKDIR) $(ETC)/gui/scripts ; fi
	$(MAKE) $(DSTFILES)

$(ETC)/gui/scripts/%: %
	$(INSTALL) $< $@

create.bat:
	for file in d.* g.* r.* v.* ; do \
	sed -e "s#SCRIPT_NAME#$$file#" $(MODULE_TOPDIR)/gui/scripts/windows_launch.bat \
		> $(ETC)/gui/scripts/$$file.bat ; done
