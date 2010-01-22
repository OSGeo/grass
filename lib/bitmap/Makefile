MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Vars.make

MOD_OBJS := bitmap.o sparse.o

LIB = BITMAP

include $(MODULE_TOPDIR)/include/Make/Lib.make

default: lib

$(OBJDIR)/main$(EXE): $(OBJDIR)/main.o $(BITMAPDEP) $(LINKMDEP)
	$(call linker)

$(OBJDIR)/smain$(EXE): $(OBJDIR)/smain.o $(BITMAPDEP) $(LINKMDEP)
	$(call linker)

$(OBJDIR)/main$(EXE) $(OBJDIR)/smain$(EXE): LIBES = $(BITMAPLIB) $(LINKMLIB)
