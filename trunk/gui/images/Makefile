MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Other.make

DSTDIR = $(GUIDIR)/images

# images
IMGSRC := $(wildcard *.png)
IMGDST := $(patsubst %,$(DSTDIR)/%,$(IMGSRC))

# symbols
CATEGORIES = basic demo extra geology legend n_arrows
SYMSRC := $(foreach dir,$(CATEGORIES),$(wildcard symbols/$(dir)/*.png))
SYMDST := $(patsubst symbols/%,$(DSTDIR)/symbols/%,$(SYMSRC))

default: $(IMGDST) $(SYMDST)

define symbols_rule
$(DSTDIR)/symbols/$(1)/%.png: symbols/$(1)/%.png | $(DSTDIR)/symbols/$(1)
	$(INSTALL_DATA) $$< $$@

$(DSTDIR)/symbols/$(1): | $(DSTDIR)/symbols
	$(MKDIR) $$@
endef

$(foreach category,$(CATEGORIES),$(eval $(call symbols_rule,$(category))))

$(DSTDIR)/%.png: %.png | $(DSTDIR)
	$(INSTALL_DATA) $< $@

$(DSTDIR):
	$(MKDIR) $@

$(DSTDIR)/symbols: $(DSTDIR)
	$(MKDIR) $@
