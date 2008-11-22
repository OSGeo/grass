MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Vars.make

PGM2 = r.mapcalc
PGM3 = r3.mapcalc
PROGRAMS = $(PGM2) $(PGM3)

r_mapcalc_OBJS  := $(filter-out map3.o xcoor3.o xres3.o, $(AUTO_OBJS))
r3_mapcalc_OBJS := $(filter-out map.o xcoor.o xres.o, $(AUTO_OBJS))

include $(MODULE_TOPDIR)/include/Make/Multi.make

EXTRA_CFLAGS = $(READLINEINCPATH)
LIBES2 = $(GISLIB) $(BTREELIB) $(ROWIOLIB) $(READLINELIBPATH) $(READLINELIB) $(HISTORYLIB)
LIBES3 = $(G3DLIB) $(GISLIB) $(BTREELIB) $(READLINELIBPATH) $(READLINELIB) $(HISTORYLIB)

ifneq ($(USE_PTHREAD),)
PTHREADLIB = -lpthread
EXTRA_CFLAGS += -DUSE_PTHREAD
LIBES2 += $(PTHREADLIB)
LIBES3 += $(PTHREADLIB)
endif

default: multi

$(BIN)/$(PGM2)$(EXE): LIBES = $(LIBES2)
$(BIN)/$(PGM3)$(EXE): LIBES = $(LIBES3)

$(OBJDIR)/mapcalc.yy.o: mapcalc.tab.h

.INTERMEDIATE: mapcalc.tab.c mapcalc.tab.h mapcalc.output

.INTERMEDIATE: mapcalc.yy.c
