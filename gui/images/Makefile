MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Other.make

ETCDIR = $(ETC)/gui/images

# images
IMGSRC := $(wildcard *.png)
IMGDST := $(patsubst %,$(ETCDIR)/%,$(IMGSRC))

# symbols
CATEGORIES = basic demo extra geology n_arrows
SYMSRC := $(foreach dir,$(CATEGORIES),$(wildcard symbols/$(dir)/*.png))
SYMDST := $(patsubst symbols/%,$(ETCDIR)/symbols/%,$(SYMSRC))

default: $(IMGDST) $(SYMDST)

define symbols_rule
$(ETCDIR)/symbols/$(1)/%.png: symbols/$(1)/%.png | $(ETCDIR)/symbols/$(1)
	$(INSTALL_DATA) $$< $$@

$(ETCDIR)/symbols/$(1): | $(ETCDIR)/symbols
	$(MKDIR) $$@
endef

$(foreach category,$(CATEGORIES),$(eval $(call symbols_rule,$(category))))

$(ETCDIR)/%.png: %.png | $(ETCDIR)
	$(INSTALL_DATA) $< $@

$(ETCDIR):
	$(MKDIR) $@

$(ETCDIR)/symbols: $(ETCDIR)
	$(MKDIR) $@
