
# generic rest rules for all commands

ifdef CROSS_COMPILING

rest:

else

#check for program
checkpandoc:
	@(type pandoc > /dev/null || (echo "ERROR: Install 'pandoc' software first (get from http://johnmacfarlane.net/pandoc/)" && exit 1))

restdesc = $(call run_grass,$(1) --rest-description < /dev/null > $(2)) || exit 0

IMGSRC := $(wildcard *.png) $(wildcard *.jpg)
IMGDST := $(patsubst %,$(RESTDIR)/%,$(IMGSRC))

$(RESTDIR)/%.txt: %.html %.tmp.txt $(HTMLSRC) $(IMGDST) | $(RESTDIR)
	$(PYTHON) $(GISBASE)/tools/mkrest.py $* $(GRASS_VERSION_DATE) > $@;
ifneq ($(strip $(IMGDST)),)
.SECONDARY: $(IMGDST)
endif

$(RESTDIR)/%.png: %.png | $(RESTDIR)
	$(INSTALL_DATA) $< $@

$(RESTDIR)/%.jpg: %.jpg | $(RESTDIR)
	$(INSTALL_DATA) $< $@

%.tmp.txt: $(HTMLSRC)
	if [ "$(HTMLSRC)" != "" ] ; then $(call restdesc,$<,$@) ; fi

rest: $(RESTDIR)/$(PGM).txt # checkpandoc

endif

.PHONY: rest
