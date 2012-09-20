
MODULE_TOPDIR = ../..

PGM = g.mkfontcap
CAPFILE = $(ETC)/fontcap

LIBES = $(GISLIB) $(FTLIB)
DEPENDENCIES = $(GISDEP)
EXTRA_CFLAGS = $(FTINC)

include $(MODULE_TOPDIR)/include/Make/Module.make

ifdef CROSS_COMPILING
default: cmd
else
default: cmd $(CAPFILE)
endif

$(CAPFILE): $(BIN)/$(PGM)$(EXE)
	GISRC=junk GISBASE=$(RUN_GISBASE) \
	$(LD_LIBRARY_PATH_VAR)="$(ARCH_LIBDIR):$(BASE_LIBDIR):$($(LD_LIBRARY_PATH_VAR))" \
	$< -s > $@
