MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Other.make

ETCDIR = $(ETC)/gui/images

FILES := $(wildcard *.png)

default: $(patsubst %,$(ETCDIR)/%,$(FILES)) symbol_thumbnails

$(ETCDIR)/%.png: %.png | $(ETCDIR)
	$(INSTALL_DATA) $< $@

symbol_thumbnails: $(ETCDIR)
	if [ ! -d $(ETCDIR)/symbols ]; then $(MKDIR) $(ETCDIR)/symbols ; fi
	for category in basic demo extra geology ; do \
		if [ ! -d $(ETCDIR)/symbols/$$category ] ; then \
			$(MKDIR) $(ETCDIR)/symbols/$$category ; \
		fi ; \
		for file in symbols/$$category/*.png ; do \
			$(INSTALL_DATA) $$file $(ETCDIR)/symbols/$$category ; \
		done ; \
	done

$(ETCDIR):
	$(MKDIR) $@
