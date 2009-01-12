MODULE_TOPDIR = ../..

PGM=grocat
include $(MODULE_TOPDIR)/include/Make/Etc.make
include $(MODULE_TOPDIR)/include/Make/NoHtml.make

default: etc $(ETC)/gtcltk

$(ETC)/gtcltk: *.tcl 
	if [ ! -d $(ETC)/gtcltk ]; then $(MKDIR) $(ETC)/gtcltk; fi
	for file in *.tcl ; do $(INSTALL_DATA) $$file $(ETC)/gtcltk/ ; done
