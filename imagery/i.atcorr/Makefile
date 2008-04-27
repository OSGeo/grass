MODULE_TOPDIR = ../..

PGM = i.atcorr

MOD_OBJS = $(subst .cpp,.o,$(wildcard *.cpp))

include $(MODULE_TOPDIR)/include/Make/Module.make

LIBES = $(GISLIB) 
DEPENDENCIES = $(DEPGISLIB)

default: cmd

$(BIN)/$(PGM)$(EXE): $(ARCH_CMD_OBJS) $(DEPENDENCIES) 
	$(CXX) $(LDFLAGS) $(XTRA_LDFLAGS) $(EXTRA_CFLAGS) $(NLS_CFLAGS) -o $@ $(ARCH_CMD_OBJS) $(FMODE_OBJ) $(LIBES) $(MATHLIB) $(XDRLIB)

