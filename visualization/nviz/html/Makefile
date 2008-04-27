MODULE_TOPDIR = ../../..

include $(MODULE_TOPDIR)/include/Make/Platform.make
include $(MODULE_TOPDIR)/include/Make/Grass.make

GRASS_NVIZ_DOCS=$(GISBASE)/docs/html/nviz

default:
	-$(MKDIR) $(GISBASE)/docs
	-$(MKDIR) $(GISBASE)/docs/html
	-$(MKDIR) $(GISBASE)/docs/html/nviz
	-$(MKDIR) $(GRASS_NVIZ_DOCS)/img
	-$(MKDIR) $(GRASS_NVIZ_DOCS)/img/nv
	-$(MKDIR) $(GRASS_NVIZ_DOCS)/nvimg
	-$(MKDIR) $(GRASS_NVIZ_DOCS)/nvimg/panel
	-$(MKDIR) $(GRASS_NVIZ_DOCS)/nvimg/work
	find . -type f \( -name '*.html' -o -name '*.png' -o -name '*.gif' \) -print | \
		while read file ; do $(INSTALL_DATA) $$file $(GRASS_NVIZ_DOCS)/$$file ; done
