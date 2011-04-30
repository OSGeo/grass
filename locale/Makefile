# written by Markus Neteler
# tcl stuff by Cedric Shock
# wxpython by Martin Landa

MODULE_TOPDIR = ..
include $(MODULE_TOPDIR)/include/Make/Other.make

default:
	@if [ "$(HAVE_NLS)" != "" ] ; then \
		echo "Creating translations (= 'make mo')" >&2 ; \
		$(MAKE) mo ; \
	else \
		echo "NLS disabled." ; \
	fi

all:
	@echo 'Usage:'
	@echo '   make pot        create grass.pot (containing original messages)'
	@echo '   make update-po  merge new messages into existing *.po files'
	@echo '   make mo         create the *.mo files'

# Directory for installing tcl .msg files:
PO_DIR = po

#distinguish between library messages and modules:
LIBDOMAIN = grasslibs
MODDOMAIN = grassmods
WXPYDOMAIN = grasswxpy
DOMAINS = $(LIBDOMAIN) $(MODDOMAIN) $(WXPYDOMAIN)

LIB_POTFILES = find ../lib \( -name "*.c" -o -name "*.py" \) | xargs grep -l "_(\""
MOD_POTFILES = find ../ -name '*.c' | grep -v '../lib' | xargs grep -l "_(\""
WXPY_POTFILES = find ../gui/wxpython -name '*.py' | xargs grep -l "_(\""

#The xgettext utility is used to automate the creation of
#portable message files (.po)
pot:
	if [ ! -f ../gui/wxpython/menustrings.py ] ; then \
		echo "Build GRASS before running 'make pot'" >&2 ; \
		exit 1 ; \
	fi
	@echo "Generating $(LIBDOMAIN)..."
	xgettext -k_ -o ./templates/$(LIBDOMAIN).pot `$(LIB_POTFILES)`
	@echo "Generating $(MODDOMAIN)..."
	xgettext -k_ -o ./templates/$(MODDOMAIN).pot `$(MOD_POTFILES)`
	@echo "Generating $(WXPYDOMAIN)..."
	xgettext -k_ -o ./templates/$(WXPYDOMAIN).pot `$(WXPY_POTFILES)` 

#merge already existing translations with new messages in POT template file and create new po files:
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
		if msgmerge -o $$prefix\_$$suffix.new $$prefix\_$$suffix ../templates/$$prefix.pot; then\
		  mv $$prefix\_$$suffix.new $$prefix\_$$suffix; \
		  echo "Merged new messages into $$prefix\_$$suffix" ; \
		else \
		    echo "Merging failed"; \
		fi \
		done
	@echo "Be careful with SVN commits as .po file updates must be syncronized with the individual translators."

verify:
	@cd ./po/ ; for po in `ls *_*.po 2>/dev/null` ; do \
		echo "----- $$po:" ; \
		msgfmt -c $$po; \
	done

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
