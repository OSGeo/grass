MODULE_TOPDIR = ../../..

include $(MODULE_TOPDIR)/include/Make/GuiScript.make

default: guiscript sample_model

sample_model: g_gui_gmodeler_zipcodes_avg_elevation.gxm
	$(INSTALL_DATA) $< $(HTMLDIR)
