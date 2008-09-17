MODULE_TOPDIR = ../../..

include $(MODULE_TOPDIR)/include/Make/Vars.make

GRASS_NVIZ_DOCS=$(ARCH_DISTDIR)/docs/html/nviz

SRCFILES := $(wildcard  *.* img/nv/*.* nvimg/*/*.*)
DSTFILES := $(patsubst %,$(GRASS_NVIZ_DOCS)/%,$(SRCFILES))

default:
	-$(MKDIR) $(ARCH_DISTDIR)/docs
	-$(MKDIR) $(ARCH_DISTDIR)/docs/html
	-$(MKDIR) $(GRASS_NVIZ_DOCS)
	-$(MKDIR) $(GRASS_NVIZ_DOCS)/img
	-$(MKDIR) $(GRASS_NVIZ_DOCS)/img/nv
	-$(MKDIR) $(GRASS_NVIZ_DOCS)/nvimg
	-$(MKDIR) $(GRASS_NVIZ_DOCS)/nvimg/panel
	-$(MKDIR) $(GRASS_NVIZ_DOCS)/nvimg/work
	$(MAKE) $(DSTFILES)

$(GRASS_NVIZ_DOCS)/%: %
	$(INSTALL_DATA) $< $@
