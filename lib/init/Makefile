#############################################################################
#
# MODULE:   	GRASS Initialization
# AUTHOR(S):	Original author unknown - probably CERL
#   	    	Justin Hickey - Thailand - jhickey@hpcc.nectec.or.th
#               Markus Neteler, Andrea Antonello
# PURPOSE:  	To create the various scripts and programs which are required
#   	    	to start GRASS.
# COPYRIGHT:    (C) 2000-2008 by the GRASS Development Team
#
#               This program is free software under the GNU General Public
#   	    	License (>=v2). Read the file COPYING that comes with GRASS
#   	    	for details.
#
#############################################################################

MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Platform.make
include $(MODULE_TOPDIR)/include/Make/Grass.make
include $(MODULE_TOPDIR)/include/Make/Rules.make

START_UP=grass$(GRASS_VERSION_MAJOR)$(GRASS_VERSION_MINOR)

EXTRA_CFLAGS = -DD_LOCATION_NAME=\"$(DEFAULT_LOCATION)\" -DD_GISDBASE=\"$(DEFAULT_DATABASE)\" -DGRASS_VERSION_NUMBER=\"'$(GRASS_VERSION_NUMBER)'\" -DGRASS_VERSION_UPDATE_PKG=\"'$(GRASS_VERSION_UPDATE_PKG)'\"

FILES = \
	$(ARCH_BINDIR)/$(START_UP) \
	$(ETC)/Init.sh \
	$(ETC)/prompt.sh \
	$(ETC)/clean_temp$(EXE) \
	$(ETC)/lock$(EXE) \
	$(ETC)/run$(EXE) \
	$(ETC)/echo$(EXE) \
	$(ETC)/grass_intro \
	$(ETC)/license \
	$(ETC)/welcome \
	$(ETC)/VERSIONNUMBER \
	$(ETC)/gis_set.tcl \
	$(ETC)/epsg_option.tcl \
	$(ETC)/file_option.tcl \
	$(ETC)/gintro.gif \
	$(ETC)/grass-run.sh \
	$(ETC)/grass-xterm-wrapper

ICONS = $(ETC)/grass.ico \
	$(ETC)/grass_map.ico \
	$(ETC)/grass_dialog.ico \
	$(ETC)/grass_sql.ico \
	$(ETC)/grass_nviz.ico \
	$(ETC)/grass_error.ico

ifeq ($(findstring darwin,$(ARCH)),darwin)
	FILES += \
		$(ETC)/grass-xterm-mac \
		$(ETC)/html_browser_mac.sh
endif

ifneq ($(strip $(MINGW)),)
	FILES += \
		$(ARCH_BINDIR)/$(START_UP).bat \
		$(ETC)/Init.bat \
		$(ETC)/grass-run.bat
endif

default: $(FILES) $(ICONS)
	$(MKDIR) $(GISBASE)/docs/html
	$(INSTALL_DATA) variables.html $(GISBASE)/docs/html/variables.html
	$(INSTALL_DATA) grass7.html $(GISBASE)/docs/html/grass7.html
	$(INSTALL_DATA) helptext.html $(GISBASE)/docs/html/helptext.html
	$(INSTALL_DATA) help_loc_struct.png $(GISBASE)/docs/html/help_loc_struct.png

$(ARCH_BINDIR)/$(START_UP): init.sh grass.src
	@test -d $(ARCH_BINDIR) || (echo 'ARCH_DISTDIR($(ARCH_BINDIR))' not found; exit 1)
	@test -w $(ARCH_BINDIR) || (echo '($(ARCH_BINDIR))' not writeable; exit 1)
	rm -f $(ARCH_BINDIR)/$(START_UP) ; true
	$(SHELL) -c "sed \
	-e \"s#GISBASE_VALUE#$(GISBASE)#\" \
	grass.src > $(ARCH_BINDIR)/$(START_UP) 2>/dev/null ; true"
	chmod a+x $(ARCH_BINDIR)/$(START_UP)

$(ARCH_BINDIR)/$(START_UP).bat: init.bat grass.bat
	@test -d $(ARCH_BINDIR) || (echo 'ARCH_DISTDIR($(ARCH_BINDIR))' not found; exit 1)
	@test -w $(ARCH_BINDIR) || (echo '($(ARCH_BINDIR))' not writeable; exit 1)
	rm -f $@ ; true
	$(SHELL) -c "sed \
	-e \"s#GISBASE_VALUE#$(RUN_GISBASE)#\" \
	grass.bat | \
	tr '/' '\\\' > $@"
	chmod a+x $@

$(ETC)/Init.sh: init.sh
	rm -f $@
	$(SHELL) -c "sed \
	-e \"s#GRASS_VERSION_NUMBER#$(GRASS_VERSION_NUMBER)#\" \
	-e \"s#GRASS_VERSION_DATE#$(GRASS_VERSION_DATE)#\" \
	-e \"s#GRASS_VERSION_UPDATE_PKG#$(GRASS_VERSION_UPDATE)#\" \
	-e \"s#LD_LIBRARY_PATH_VAR#$(LD_LIBRARY_PATH_VAR)#g\" \
	-e \"s#PERL_COMMAND#$(PERL)#\" \
	-e \"s#START_UP#$(START_UP)#\" \
	-e \"s#CONFIG_PROJSHARE#$(PROJSHARE)#\" \
	init.sh > $@"
	chmod +x $@

$(ETC)/Init.bat: init.bat
	rm -f $@
	$(SHELL) -c "sed \
	-e \"s#GRASS_VERSION_NUMBER#$(GRASS_VERSION_NUMBER)#\" \
	-e \"s#CONFIG_PROJSHARE#$(PROJSHARE)#\" \
	$< > $@"
	chmod +x $@

$(ETC)/echo$(EXE): $(OBJDIR)/echo.o
	$(CC) $(LDFLAGS) $(OBJDIR)/echo.o -o $@

$(ETC)/clean_temp$(EXE): $(OBJDIR)/clean_temp.o  
	$(CC) $(LDFLAGS) $(OBJDIR)/clean_temp.o $(GISLIB) $(DATETIMELIB) $(MATHLIB) $(XDRLIB) -o $@

$(ETC)/run$(EXE): $(OBJDIR)/run.o
	$(CC) $(LDFLAGS) $(OBJDIR)/run.o -o $@

$(ETC)/lock$(EXE): $(OBJDIR)/lock.o
	$(CC) $(LDFLAGS) $(OBJDIR)/lock.o $(GISLIB) $(DATETIMELIB) $(MATHLIB) $(XDRLIB) -o $@

$(BIN)/exit:
	touch $@
	chmod 600 $@

$(ETC)/VERSIONNUMBER:
	rm -f $@
	echo "$(GRASS_VERSION_NUMBER)" > $@
	chmod +r $@

$(ETC)/grass_intro: grass_intro.txt version.sed 
	rm -f $@
	sh ./version.sed "$(GRASS_VERSION_NUMBER)" "$(GRASS_VERSION_DATE)" "$(GRASS_VERSION_UPDATE_PKG)" grass_intro.txt > $@
	chmod +r $@

$(ETC)/license: license.txt version.sed
	rm -f $@
	sh ./version.sed "$(GRASS_VERSION_NUMBER)" "$(GRASS_VERSION_DATE)" "$(GRASS_VERSION_UPDATE_PKG)" license.txt > $@
	chmod +r $@

$(ETC)/welcome: welcome.txt version.sed
	rm -f $@
	sh ./version.sed "$(GRASS_VERSION_NUMBER)" "$(GRASS_VERSION_DATE)" "$(GRASS_VERSION_UPDATE_PKG)" welcome.txt > $@
	chmod +r $@

$(ETC)/msgs: msgs
	rm -rf $@
	$(MKDIR) $@
	$(INSTALL_DATA) msgs/*.msg $@	

$(ETC)/gintro.gif: gintro.gif
	rm -f $@
	$(INSTALL_DATA) gintro.gif $@

$(ICONS): grass.ico grass_map.ico grass_sql.ico grass_dialog.ico grass_nviz.ico grass_error.ico
	rm -f $@
	$(INSTALL_DATA) $< $@

$(ETC)/gis_set.tcl: gis_set.tcl
	rm -f $@
	$(INSTALL) gis_set.tcl $(ETC)

$(ETC)/epsg_option.tcl: epsg_option.tcl
	rm -f $@
	$(INSTALL) epsg_option.tcl $(ETC)

$(ETC)/file_option.tcl: file_option.tcl
	rm -f $@
	$(INSTALL) file_option.tcl $(ETC)

$(ETC)/grass-run.sh: grass-run.src
	rm -f $@
	$(SHELL) -c "sed \
	-e \"s#LD_LIBRARY_PATH_VAR#$(LD_LIBRARY_PATH_VAR)#g\" \
	grass-run.src > $@"
	chmod +x $@

$(ETC)/grass-run.bat: grass-run.bat
	$(INSTALL) grass-run.bat $(ETC)

$(ETC)/grass-xterm-wrapper: grass-xterm-wrapper
	$(INSTALL) grass-xterm-wrapper $(ETC)

$(ETC)/grass-xterm-mac: grass-xterm-mac
	$(INSTALL) grass-xterm-mac $(ETC)

$(ETC)/prompt.sh: prompt.sh
	rm -f $@
	$(INSTALL) prompt.sh $(ETC)

$(ETC)/html_browser_mac.sh: html_browser_mac.sh
	$(INSTALL) html_browser_mac.sh $(ETC)

