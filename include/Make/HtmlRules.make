
# common html rules (included by Html.make and GuiScript.make)

htmldesc = $(call run_grass,$(1) --html-description < /dev/null | grep -v '</body>\|</html>\|</div> <!-- end container -->' > $(2))

mddesc = $(call run_grass,$(1) --md-description < /dev/null > $(2))

IMGSRC := $(wildcard *.png) $(wildcard *.jpg) $(wildcard *.gif)
IMGDST := $(patsubst %,$(HTMLDIR)/%,$(IMGSRC))
IMGDST_MD := $(patsubst %,$(MDDIR)/source/%,$(IMGSRC))

ifneq ($(strip $(IMGDST)),)
.SECONDARY: $(IMGDST)
endif

ifneq ($(strip $(IMGDST_MD)),)
.SECONDARY: $(IMGDST_MD)
endif

$(HTMLDIR)/%.png: %.png | $(HTMLDIR)
	$(INSTALL_DATA) $< $@

$(HTMLDIR)/%.jpg: %.jpg | $(HTMLDIR)
	$(INSTALL_DATA) $< $@

$(HTMLDIR)/%.gif: %.gif | $(HTMLDIR)
	$(INSTALL_DATA) $< $@

$(MDDIR)/source/%.png: %.png | $(MDDIR)
	$(INSTALL_DATA) $< $@

$(MDDIR)/source/%.jpg: %.jpg | $(MDDIR)
	$(INSTALL_DATA) $< $@

$(MDDIR)/source/%.gif: %.gif | $(MDDIR)
	$(INSTALL_DATA) $< $@
