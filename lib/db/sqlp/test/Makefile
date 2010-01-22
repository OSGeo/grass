
MODULE_TOPDIR = ../../../..

include $(MODULE_TOPDIR)/include/Make/Other.make
include $(MODULE_TOPDIR)/include/Make/Compile.make

LIBES = $(SQLPLIB) $(DBMILIB) $(GISLIB) $(MATHLIB) $(ZLIBLIBPATH) $(ZLIB)

EXTRA_CLEAN_FILES = $(OBJDIR)/sqlptest$(EXE)

default: $(OBJDIR)/sqlptest$(EXE)

$(OBJDIR)/sqlptest$(EXE): $(OBJDIR)/sqlptest.o
	$(call linker)
