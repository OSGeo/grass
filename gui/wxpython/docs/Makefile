MODULE_TOPDIR = ../../..

include $(MODULE_TOPDIR)/include/Make/Other.make

FILES = wxGUI \
	wxGUI.Vector_Digitizer \
	wxGUI.Attribute_Table_Manager \
	wxGUI.Nviz \
	wxGUI.GCP_Manager \
	wxGUI.Modeler

default: $(patsubst %,$(HTMLDIR)/%.html,$(FILES))
