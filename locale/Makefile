# written by Markus Neteler
# tcl stuff by Cedric Shock
# wxpython by Martin Landa

MODULE_TOPDIR = ..
include $(MODULE_TOPDIR)/include/Make/Other.make

default:
	@if [ "$(HAVE_NLS)" != "" ] ; then \
		echo "Creating translations (= 'make mo')" >&2 ; \
		$(MAKE) mo ; \
		echo "Creating translation statistics (= 'make statistics')" >&2 ; \
		$(MAKE) statistics ; \
	else \
		echo "NLS disabled." ; \
	fi

all:
	@echo 'Usage:'
	@echo '   make pot        create grass.pot (containing original messages)'
	@echo '   make update-po  merge new messages into existing *.po files'
	@echo '   make mo         create the *.mo files'
	@echo '   make verify     verify the *.po files'

# Directory for installing tcl .msg files:
PO_DIR = po

#distinguish between library messages and modules:
LIBDOMAIN = grasslibs
MODDOMAIN = grassmods
WXPYDOMAIN = grasswxpy
DOMAINS = $(LIBDOMAIN) $(MODDOMAIN) $(WXPYDOMAIN)

LIB_POTFILES = find ../lib \( -name "*.c" -o -name "*.py" \) | xargs grep -l "_(\"\|n_(\""
MOD_POTFILES = find ../ \( -name "*.c" -o -name "*.cpp" \) | grep -v '../lib' | xargs grep -l "_(\"\|n_(\""
WXPY_POTFILES = find ../gui/wxpython -name '*.py' | xargs grep -l "_(\"\|n_(\""
#For Python script module messages
MOD_PYFILES = find ../scripts -name '*.py' | xargs grep -l "_(\"\|n_(\""

# XGETTEXT_ARGS = --copyright-holder='GRASS Development Team' --no-wrap --sort-by-file --package-name='GRASS' --package-version='$(GRASS_VERSION_MAJOR)'
XGETTEXT_ARGS = --no-wrap --sort-by-file
# MSGMERGE_ARGS = --no-wrap --sort-by-file --previous --update
MSGMERGE_ARGS = --no-wrap --sort-by-file --previous
MSGCONV_ARGS = --no-wrap --sort-by-file
# MSGCONV_ARGS = --sort-by-file
# MSGUNIQ_ARGS = --repeated
# MSGUNIQ_ARGS = --sort-by-file 
# MSGUNIQ_ARGS = --no-wrap
MSGUNIQ_ARGS = --no-wrap --sort-by-file
# MSGUNIQ_PO_ARGS = --no-wrap --sort-by-file
MSGUNIQ_PO_ARGS = --no-wrap --sort-by-file
# MSGUNIQ_PO_ARGS = --unique

define po_stats
GISBASE="$(RUN_GISBASE)" $(PYTHON) ./grass_po_stats.py
endef

#The xgettext utility is used to automate the creation of
#portable message files (.po)
pot:
	if [ ! -f ../gui/wxpython/menustrings.py ] ; then \
		echo "Build GRASS before running 'make pot'" >&2 ; \
		exit 1 ; \
	fi
	@rm -f ./templates/*.pot
	@echo "Generating $(LIBDOMAIN)..."
	xgettext $(XGETTEXT_ARGS) --from-code=utf-8 --keyword=_ --keyword=n_:1,2 -cGTC -o ./templates/$(LIBDOMAIN).pot `$(LIB_POTFILES)`
	@echo "Generating $(MODDOMAIN)..."
	xgettext $(XGETTEXT_ARGS) --from-code=utf-8 --keyword=_ --keyword=n_:1,2 -cGTC -o ./templates/$(MODDOMAIN).pot `$(MOD_POTFILES)`
	xgettext $(XGETTEXT_ARGS) --from-code=utf-8 -j --keyword=_ --keyword=n_:1,2 -cGTC -o ./templates/$(MODDOMAIN).pot `$(MOD_PYFILES)`
	@echo "Generating $(WXPYDOMAIN)..."
	xgettext $(XGETTEXT_ARGS) --from-code=utf-8 --keyword=_ --keyword=n_:1,2 -cGTC -o ./templates/$(WXPYDOMAIN).pot `$(WXPY_POTFILES)`

sort-pot:
	if [ ! -f ../gui/wxpython/menustrings.py ] ; then \
		echo "Build GRASS before running 'make pot'" >&2 ; \
		exit 1 ; \
	fi
	@echo "Sorting $(LIBDOMAIN)..."
	msgconv $(MSGCONV_ARGS) -o ./templates/$(LIBDOMAIN).pot ./templates/$(LIBDOMAIN).pot
	@echo "Sorting $(MODDOMAIN)..."
	msgconv $(MSGCONV_ARGS) -o ./templates/$(MODDOMAIN).pot ./templates/$(MODDOMAIN).pot
	@echo "Sorting $(WXPYDOMAIN)..."
	msgconv $(MSGCONV_ARGS) -o ./templates/$(WXPYDOMAIN).pot ./templates/$(WXPYDOMAIN).pot

sort-pot-unique:
	@echo "Sorting $(LIBDOMAIN)..."
	msguniq $(MSGUNIQ_ARGS) -o ./templates/$(LIBDOMAIN).pot ./templates/$(LIBDOMAIN).pot
	@echo "Sorting $(MODDOMAIN)..."
	msguniq $(MSGUNIQ_ARGS) -o ./templates/$(MODDOMAIN).pot ./templates/$(MODDOMAIN).pot
	@echo "Sorting $(WXPYDOMAIN)..."
	msguniq $(MSGUNIQ_ARGS) -o ./templates/$(WXPYDOMAIN).pot ./templates/$(WXPYDOMAIN).pot


unique-po:
	@for i in $(DOMAINS) ; do \
		if [ "`ls ./po/$$i\_*.po 2>/dev/null`" = "" ] ; then \
		 echo "No $$i.po file found in ./po/ directory. Will create new po files from template." ; \
		 cp ./templates/$$i.pot ./po/$$i.po ; \
		 echo "Created ./po/$$i.po - Rename to ./po/$$i\_LANG.po (with LANG: de, ru, ...)" ; \
		 echo "Then you can translate the messages in this file (e.g with kbabel)" ; \
		fi ;\
	done
	@cd ./po/ ; for po in `ls *_*.po 2>/dev/null` ; do \
		suffix=`echo $$po | cut -d'_' -f2-`; \
		lingua=`basename $$suffix .po`; \
		prefix=`echo $$po | cut -d'_' -f1`; \
		if msguniq $(MSGUNIQ_PO_ARGS) $$prefix\_$$suffix -o $$prefix\_$$suffix; then \
			echo "Unique messages into $$prefix\_$$suffix" ; \
		else \
		    echo "Unique failed"; \
		fi \
		done
	@echo "Be careful with git commits as .po file updates must be synchronized with the individual translators."
#merge already existing translations with new messages in POT template file, deduplicate it and create new po files:
update-po:
	@for i in $(DOMAINS) ; do \
		if [ "`ls ./po/$$i\_*.po 2>/dev/null`" = "" ] ; then \
		 echo "No $$i.po file found in ./po/ directory. Will create new po files from template." ; \
		 cp ./templates/$$i.pot ./po/$$i.po ; \
		 echo "Created ./po/$$i.po - Rename to ./po/$$i\_LANG.po (with LANG: de, ru, ...)" ; \
		 echo "Then you can translate the messages in this file (e.g with kbabel)" ; \
		fi ;\
	done
	@cd ./po/ ; for po in `ls *_*.po 2>/dev/null` ; do \
		suffix=`echo $$po | cut -d'_' -f2-`; \
		lingua=`basename $$suffix .po`; \
		prefix=`echo $$po | cut -d'_' -f1`; \
		if msgmerge $(MSGMERGE_ARGS) -o $$prefix\_$$suffix.new $$prefix\_$$suffix ../templates/$$prefix.pot; then\
		  msguniq $(MSGUNIQ_ARGS) --use-first $$prefix\_$$suffix.new > $$prefix\_$$suffix; \
		  rm -f $$prefix\_$$suffix.new; \
		  echo "Merged new messages into $$prefix\_$$suffix" ; \
		else \
		  echo "Merging failed"; \
		fi \
		done
	@echo "Be careful with git commits as .po file updates must be synchronized with the individual translators."

verify:
	@cd ./po/ ; for po in `ls *_*.po 2>/dev/null` ; do \
		echo "----- $$po:" ; \
		msgfmt -c $$po; \
	done
	@rm -f po/messages.mo
	@echo "Note: In case of translations errors to be fixed, edit the respective translation on transifex (and not in git)"

statistics:
	$(call po_stats)

define dom_rule
$(1)_FILES := $$(patsubst po/grass$(1)_%.po,$$(MO_DIR)/%/LC_MESSAGES/grass$(1).mo,$$(wildcard po/grass$(1)_*.po))
$$(MO_DIR)/%/LC_MESSAGES/grass$(1).mo: po/grass$(1)_%.po
	@ [ -d $$(MO_DIR)/$$*/LC_MESSAGES ] || $(MKDIR) $$(MO_DIR)/$$*/LC_MESSAGES
	msgfmt --statistics -o $$@ $$<
endef
$(foreach domain,libs mods wxpy,$(eval $(call dom_rule,$(domain))))

$(MSG_DIR)/%.msg: po/grasstcl_%.po
	@ [ -d $(MSG_DIR) ] || $(MKDIR) $(MSG_DIR)
	msgfmt --statistics --tcl -l $* -d $(MSG_DIR)/ $<

MSGFILES := $(patsubst po/grasstcl_%.po,$(MSG_DIR)/%.msg,$(wildcard po/grasstcl_*.po))

#create binary messages files
mo: $(libs_FILES) $(mods_FILES) $(wxpy_FILES) $(MSGFILES)

.PHONY: mo
