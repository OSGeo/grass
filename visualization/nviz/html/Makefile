MODULE_TOPDIR = ../../..

include $(MODULE_TOPDIR)/include/Make/Vars.make

GRASS_NVIZ_DOCS=$(ARCH_DISTDIR)/docs/html/nviz

SRCFILES := $(wildcard  *.* img/nv/*.* nvimg/*/*.*)
DSTFILES := $(patsubst %,$(GRASS_NVIZ_DOCS)/%,$(SRCFILES))

default:
	-if [ ! -d $(ARCH_DISTDIR)/docs ] ; then $(MKDIR) $(ARCH_DISTDIR)/docs ; fi
	-if [ ! -d $(ARCH_DISTDIR)/docs/html ] ; then $(MKDIR) $(ARCH_DISTDIR)/docs/html ; fi
	-for dir in '' img img/nv nvimg nvimg/panel nvimg/work ; do \
	if [ ! -d $(GRASS_NVIZ_DOCS)/$$dir ] ; then $(MKDIR) $(GRASS_NVIZ_DOCS)/$$dir ; fi \
	done
	$(MAKE) $(DSTFILES)

$(GRASS_NVIZ_DOCS)/%: %
	$(INSTALL_DATA) $< $@
