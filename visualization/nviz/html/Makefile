MODULE_TOPDIR = ../../..

include $(MODULE_TOPDIR)/include/Make/Vars.make

GRASS_NVIZ_DOCS=$(ARCH_DISTDIR)/docs/html/nviz

SRCFILES := $(wildcard  *.* img/nv/*.* nvimg/*/*.*)
DSTFILES := $(patsubst %,$(GRASS_NVIZ_DOCS)/%,$(SRCFILES))

DSTDIRS := $(ARCH_DISTDIR)/docs $(ARCH_DISTDIR)/docs/html $(GRASS_NVIZ_DOCS) \
	$(patsubst %,$(GRASS_NVIZ_DOCS)/%,img img/nv nvimg/panel nvimg/work)

default: $(DSTFILES)

$(DSTDIRS): %:
	$(MKDIR) $@

$(GRASS_NVIZ_DOCS)/%: % | $(DSTDIRS)
	$(INSTALL_DATA) $< $@
