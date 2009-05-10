MODULE_TOPDIR = ../../..

include $(MODULE_TOPDIR)/include/Make/Dir.make

ETCDIR = $(ETC)/gui/scripts

ifdef MINGW
SCRIPT_ACTIONS += create.bat
endif

default: install_scripts

install_scripts: $(SCRIPT_ACTIONS)
	if [ ! -d $(ETCDIR) ] ; then $(MKDIR) $(ETCDIR) ; fi
	$(MAKE) $(ETCDIR)/d.rast3d

$(ETCDIR)/%: %
	$(INSTALL) $< $@

create.bat:
	sed -e "s#SCRIPT_NAME#d.rast3d#" $(MODULE_TOPDIR)/gui/scripts/windows_launch.bat \
		> $(ETCDIR)/d.rast3d.bat

clean: cleansubdirs
