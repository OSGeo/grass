# written by Markus Neteler
# tcl stuff by Cedric Shock
# wxpython by Martin Landa

MODULE_TOPDIR = ..
include $(MODULE_TOPDIR)/include/Make/Module.make

default:
	@if [ "$(HAVE_NLS)" != "" ] ; then echo "Creating translations (= 'make mo')" ; $(MAKE) mo ; else echo "NLS disabled, cannot translate (re-configure GRASS first)." ; fi

all:
	@echo 'Usage:'
	@echo '   make pot        create grass.pot (containing original messages)'
	@echo '   make update-po  merge new messages into existing *.po files'
	@echo '   make mo         create the *.mo files'

# Directory for installing tcl .msg files:
MSG_DIR = $(GISBASE)/etc/msgs
MO_DIR = $(GISBASE)/locale
PO_DIR = po

#distinguish between library messages and modules:
LIBDOMAIN = grasslibs
MODDOMAIN = grassmods
TCLDOMAIN = grasstcl
WXPYDOMAIN = grasswxpy
DOMAINS = $(LIBDOMAIN) $(MODDOMAIN) $(TCLDOMAIN) $(WXPYDOMAIN)

LIB_POTFILES = find ../lib -name '*.c' | xargs grep -l "_(\""
MOD_POTFILES = find ../ -name '*.c' | grep -v '../lib' | xargs grep -l "_(\""
TCL_POTFILES = find ../ -name '*.tcl' | grep -v '../dist' | xargs grep -l "\\[_ \|\\[G_msg"
WXPY_POTFILES = find ../gui/wxpython -name '*.py' | xargs grep -l "_(\""

#The xgettext utility is used to automate the creation of
#portable message files (.po)
pot:
	@echo "Generating $(LIBDOMAIN)..."
	xgettext -k_ -o ./templates/$(LIBDOMAIN).pot `$(LIB_POTFILES)`
	@echo "Generating $(MODDOMAIN)..."
	xgettext -k_ -o ./templates/$(MODDOMAIN).pot `$(MOD_POTFILES)`
	@echo "Generating $(TCLDOMAIN)..."
	xgettext -k_ --keyword=G_msg -o ./templates/$(TCLDOMAIN).pot `$(TCL_POTFILES)`
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

#create binary messages files
mo:
	@(cd ./po/ ; for po in `ls *_*.po 2>/dev/null` ; do\
		suffix=`echo $$po | cut -d'_' -f2-`; \
		lingua=`basename $$suffix .po`; \
		prefix=`echo $$po | cut -d'_' -f1`; \
		install -d $(MO_DIR)/$$lingua/LC_MESSAGES/ ; \
		echo -n $$po": "; \
		if [ $$prefix = $(TCLDOMAIN) ]; then\
		install -d $(MSG_DIR) ; \
		msgfmt --statistics --tcl -l $$lingua \
			-d $(MSG_DIR)/ $$po ;\
		else \
		msgfmt --statistics \
			-o $(MO_DIR)/$$lingua/LC_MESSAGES/$$prefix.mo $$po ;\
		fi \
	done \
	)
