MODULE_TOPDIR = ../../..

FILES = wxGUI wxGUI.Digitization_Tool wxGUI.Attribute_Table_Manager

include $(MODULE_TOPDIR)/include/Make/Platform.make
include $(MODULE_TOPDIR)/include/Make/Grass.make
include $(MODULE_TOPDIR)/include/Make/Rules.make

default:
	for file in $(FILES) ; do $(MAKE) htmldir PGM=$$file; done
