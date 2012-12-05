
# common html rules (included by Html.make and GuiScript.make)

htmldesc = $(call run_grass,$(1) --html-description < /dev/null | grep -v '</body>\|</html>' > $(2))

IMGSRC := $(wildcard *.png) $(wildcard *.jpg)
IMGDST := $(patsubst %,$(HTMLDIR)/%,$(IMGSRC))

ifneq ($(strip $(IMGDST)),)
.SECONDARY: $(IMGDST)
endif

$(HTMLDIR)/%.png: %.png | $(HTMLDIR)
	$(INSTALL_DATA) $< $@

$(HTMLDIR)/%.jpg: %.jpg | $(HTMLDIR)
	$(INSTALL_DATA) $< $@
