
include $(MODULE_TOPDIR)/include/Make/Vars.make

ifdef MINGW
SCRIPT = $(SCRIPTDIR)/$(PGM).py
else
SCRIPT = $(SCRIPTDIR)/$(PGM)
endif

HTMLSRC = $(SCRIPT)

ETCDIR = $(ETC)/$(PGM)
ETCPYFILES := $(patsubst %,$(ETCDIR)/%.py,$(ETCFILES))
ETCPYCFILES := $(patsubst %,$(ETCDIR)/%.pyc,$(ETCFILES))

include $(MODULE_TOPDIR)/include/Make/Rules.make
include $(MODULE_TOPDIR)/include/Make/Html.make
include $(MODULE_TOPDIR)/include/Make/ScriptRules.make

SCRIPT_ACTIONS = $(SCRIPT) $(ETCPYFILES) $(ETCPYCFILES) html scriptstrings
ifdef MINGW
SCRIPT_ACTIONS += $(BIN)/$(PGM).bat
SCRIPT_DIR = %GISBASE%/scripts
endif

script: $(SCRIPT_ACTIONS)

$(BIN)/$(PGM).bat: $(MODULE_TOPDIR)/scripts/windows_launch.bat
	sed -e "s#SCRIPT_NAME#$(PGM)#" -e "s#SCRIPT_DIR#$(SCRIPT_DIR)#" $(MODULE_TOPDIR)/scripts/windows_launch.bat > $@
	unix2dos $@

scriptstrings: $(STRINGDIR)/$(PGM)_to_translate.c

install:
	$(INSTALL) $(SCRIPT) $(INST_DIR)/scripts/
	$(INSTALL_DATA) $(HTMLDIR)/$(PGM).html $(INST_DIR)/docs/html/
	$(eval IMG := $(wildcard $(HTMLDIR)/*.png) $(wildcard $(HTMLDIR)/*.jpg) $(wildcard $(HTMLDIR)/*.gif))
	if [ -n "$(IMG)" ] ; then \
		$(INSTALL_DATA) $(IMG)  $(INST_DIR)/docs/html/ ; \
	fi
	$(MKDIR) $(INST_DIR)/docs/mkdocs/source
	$(INSTALL_DATA) $(MDDIR)/source/$(PGM).md $(INST_DIR)/docs/mkdocs/source/
	$(eval IMG_MD := $(wildcard $(MDDIR)/source/*.png) $(wildcard $(MDDIR)/source/*.jpg) $(wildcard $(MDDIR)/source/*.gif))
	if [ -n "$(IMG_MD)" ] ; then \
		$(INSTALL_DATA) $(IMG_MD)  $(INST_DIR)/docs/mkdocs/source/ ; \
	fi
	$(INSTALL_DATA) $(ARCH_DISTDIR)/docs/man/man1/$(PGM).1 $(INST_DIR)/docs/man/man1/
	if [ -d "$(ETC)/$(PGM)" ] ; then \
		cp -RL $(ETC)/$(PGM) $(INST_DIR)/etc/ ; \
	fi

.PHONY: script scriptstrings
