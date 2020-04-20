MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Other.make
include $(MODULE_TOPDIR)/include/Make/Compile.make

#for i18N support
PACKAGE ="grasslibs"

START_UP=grass$(GRASS_VERSION_MAJOR)$(GRASS_VERSION_MINOR)

EXTRA_CFLAGS = \
	-DD_LOCATION_NAME=\"$(DEFAULT_LOCATION)\" \
	-DD_GISDBASE=\"$(DEFAULT_DATABASE)\" \
	-DGRASS_VERSION_NUMBER=\"'$(GRASS_VERSION_NUMBER)'\" \
	-DGRASS_VERSION_UPDATE_PKG=\"'$(GRASS_VERSION_UPDATE_PKG)'\"

LIBES = $(GISLIB) $(DATETIMELIB)

FILES = \
	$(ARCH_BINDIR)/$(START_UP) \
	$(ARCH_DISTDIR)/$(START_UP).tmp	\
	$(ETC)/clean_temp$(EXE) \
	$(ETC)/lock$(EXE) \
	$(ETC)/run$(EXE) \
	$(ETC)/echo$(EXE) \
	$(ETC)/license \
	$(ETC)/VERSIONNUMBER \
	$(HTMLDIR)/help_loc_struct.png \
	$(HTMLDIR)/circle_1.png \
	$(HTMLDIR)/circle_2.png \
	$(HTMLDIR)/circle_3.png \
	$(HTMLDIR)/circle_4.png \
	$(HTMLDIR)/circle_5.png \
	$(HTMLDIR)/grass_start.png

ifeq ($(findstring darwin,$(ARCH)),darwin)
	FILES += \
		$(ETC)/html_browser_mac.sh
endif

ifneq ($(strip $(MINGW)),)
	FILES += \
		$(ARCH_BINDIR)/$(START_UP).py \
		$(ETC)/winlocale$(EXE)
endif

HTMLFILES := $(wildcard *.html)

default: $(FILES) $(patsubst %,$(HTMLDIR)/%,$(HTMLFILES))

ifneq ($(strip $(MINGW)),)
$(ARCH_BINDIR)/$(START_UP): grass.sh
	@test -d $(ARCH_BINDIR) || (echo 'ARCH_BINDIR($(ARCH_BINDIR))' not found; exit 1)
	@test -w $(ARCH_BINDIR) || (echo 'ARCH_BINDIR($(ARCH_BINDIR))' not writable; exit 1)
	rm -f $@ ; true
	sed \
	-e 's#@BINDIR@#$(ARCH_BINDIR)#' \
	$< > $@
	chmod a+x $@
endif

ifneq ($(strip $(MINGW)),)
$(ARCH_BINDIR)/$(START_UP).py: grass.py
else
$(ARCH_BINDIR)/$(START_UP): grass.py
endif
	rm -f $@
	sed \
	-e 's#@GISBASE@#$(RUN_GISBASE)#' \
	-e 's#@GRASS_VERSION_NUMBER@#$(GRASS_VERSION_NUMBER)#' \
	-e 's#@GRASS_VERSION_GIT@#$(GRASS_VERSION_GIT)#' \
	-e 's#@LD_LIBRARY_PATH_VAR@#$(LD_LIBRARY_PATH_VAR)#' \
	-e 's#@START_UP@#$(START_UP)#' \
	-e 's#@CONFIG_PROJSHARE@#$(PROJSHARE)#' \
	$< > $@
	chmod +x $@

$(ARCH_DISTDIR)/$(START_UP).tmp: grass.py
	rm -f $@
	sed \
	-e 's#@GRASS_VERSION_NUMBER@#$(GRASS_VERSION_NUMBER)#' \
	-e 's#@GRASS_VERSION_GIT@#$(GRASS_VERSION_GIT)#' \
	-e 's#@START_UP@#$(START_UP)#' \
	-e 's#@GRASS_CONFIG_DIR@#$(GRASS_CONFIG_DIR)#' \
	-e 's#@LD_LIBRARY_PATH_VAR@#$(LD_LIBRARY_PATH_VAR)#' \
	-e 's#@CONFIG_PROJSHARE@#$(PROJSHARE)#' \
	$< > $@

$(ETC)/echo$(EXE) $(ETC)/run$(EXE): $(ETC)/%$(EXE): $(OBJDIR)/%.o
	$(call linker_base,$(LINK),$(LDFLAGS),$(MANIFEST_OBJ))

$(ETC)/clean_temp$(EXE) $(ETC)/lock$(EXE): $(ETC)/%$(EXE): $(OBJDIR)/%.o
	$(call linker)

$(ETC)/winlocale$(EXE): $(OBJDIR)/winlocale.o
	$(call linker_base,$(LINK),$(LDFLAGS) $(EXTRA_LDFLAGS),$(MANIFEST_OBJ) -lkernel32)

$(ETC)/VERSIONNUMBER:
	rm -f $@
	@if [ -n "$(GRASS_VERSION_GIT)" ] ; then \
		echo "$(GRASS_VERSION_NUMBER) $(GRASS_VERSION_GIT)" > $@ ; \
	else \
		echo "$(GRASS_VERSION_NUMBER)" > $@ ; \
	fi
	chmod +r $@

$(ETC)/license: $(ETC)/%: %.txt version.sed
	rm -f $@
	sh ./version.sed "$(GRASS_VERSION_NUMBER)" "$(GRASS_VERSION_DATE)" "$(GRASS_VERSION_UPDATE_PKG)" $< > $@
	chmod +r $@

$(ETC)/%: %
	$(INSTALL) $< $@
