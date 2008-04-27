MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Platform.make
include $(MODULE_TOPDIR)/include/Make/Grass.make
include $(MODULE_TOPDIR)/include/Make/Rules.make

default: $(ETC)/gtcltk $(ETC)/grocat$(EXE)

$(ETC)/gtcltk: *.tcl 
	if [ ! -d $(ETC)/gtcltk ]; then $(MKDIR) $(ETC)/gtcltk; fi
	for file in *.tcl ; do $(INSTALL_DATA) $$file $(ETC)/gtcltk/ ; done

$(ETC)/grocat$(EXE): $(OBJDIR)/grocat.o
	$(CC) $(LDFLAGS) $(OBJDIR)/grocat.o -o $@
	
clean:
