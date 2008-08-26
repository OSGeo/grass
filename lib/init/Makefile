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

include $(MODULE_TOPDIR)/include/Make/Other.make
include $(MODULE_TOPDIR)/include/Make/Compile.make

START_UP=grass$(GRASS_VERSION_MAJOR)$(GRASS_VERSION_MINOR)

EXTRA_CFLAGS = \
	-DD_LOCATION_NAME=\"$(DEFAULT_LOCATION)\" \
	-DD_GISDBASE=\"$(DEFAULT_DATABASE)\" \
	-DGRASS_VERSION_NUMBER=\"'$(GRASS_VERSION_NUMBER)'\" \
	-DGRASS_VERSION_UPDATE_PKG=\"'$(GRASS_VERSION_UPDATE_PKG)'\"

LIBES = $(GISLIB) $(DATETIMELIB)

FILES = \
	$(ARCH_BINDIR)/$(START_UP) \
	$(ETC)/Init.sh \
	$(ETC)/functions.sh \
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
	$(HTMLDIR)/variables.html \
	$(HTMLDIR)/grass7.html \
	$(HTMLDIR)/helptext.html \
	$(HTMLDIR)/help_loc_struct.png

ifeq ($(findstring darwin,$(ARCH)),darwin)
	FILES += \
		$(ETC)/html_browser_mac.sh
endif

ifneq ($(strip $(MINGW)),)
	FILES += \
		$(ARCH_BINDIR)/$(START_UP).bat \
		$(ETC)/Init.bat
endif

default: $(FILES)

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
	-e \"s#@GRASS_VERSION_NUMBER@#$(GRASS_VERSION_NUMBER)#\" \
	-e \"s#@LD_LIBRARY_PATH_VAR@#$(LD_LIBRARY_PATH_VAR)#g\" \
	-e \"s#@START_UP@#$(START_UP)#\" \
	-e \"s#@CONFIG_PROJSHARE@#$(PROJSHARE)#\" \
	init.sh > $@"
	chmod +x $@

$(ETC)/Init.bat: init.bat
	rm -f $@
	$(SHELL) -c "sed \
	-e \"s#GRASS_VERSION_NUMBER#$(GRASS_VERSION_NUMBER)#\" \
	-e \"s#CONFIG_PROJSHARE#$(PROJSHARE)#\" \
	$< > $@"
	chmod +x $@

$(ETC)/functions.sh: functions.sh
	$(INSTALL_DATA) $< $@

$(ETC)/echo$(EXE) $(ETC)/run$(EXE): $(ETC)/%$(EXE): $(OBJDIR)/%.o
	$(CC) $(LDFLAGS) $< -o $@

$(ETC)/clean_temp$(EXE) $(ETC)/lock$(EXE): $(ETC)/%$(EXE): $(OBJDIR)/%.o
	$(call linker)

$(ETC)/VERSIONNUMBER:
	rm -f $@
	echo "$(GRASS_VERSION_NUMBER)" > $@
	chmod +r $@

$(ETC)/grass_intro $(ETC)/license $(ETC)/welcome: $(ETC)/%: %.txt version.sed 
	rm -f $@
	sh ./version.sed "$(GRASS_VERSION_NUMBER)" "$(GRASS_VERSION_DATE)" "$(GRASS_VERSION_UPDATE_PKG)" $< > $@
	chmod +r $@

$(HTMLDIR)/%: %
	$(INSTALL_DATA) $< $@

$(ETC)/%: %
	$(INSTALL) $< $@

