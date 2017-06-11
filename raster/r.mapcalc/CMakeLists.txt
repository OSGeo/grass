
# MODULE_TOPDIR = ../..

# include $(MODULE_TOPDIR)/include/Make/Vars.make

# PGM2 = r.mapcalc
# PGM3 = r3.mapcalc
# PROGRAMS = $(PGM2) $(PGM3)

# r_mapcalc_OBJS  := $(filter-out map3.o xcoor3.o xres3.o, $(AUTO_OBJS))
# r3_mapcalc_OBJS := $(filter-out map.o xcoor.o xres.o, $(AUTO_OBJS))


# include $(MODULE_TOPDIR)/include/Make/Multi.make

# EXTRA_CFLAGS = $(READLINEINCPATH) $(PTHREADINCPATH)
# LIBES2 = $(CALCLIB) $(GISLIB) $(RASTERLIB) $(BTREELIB) $(READLINELIBPATH) $(READLINELIB) $(HISTORYLIB) $(PTHREADLIBPATH) $(PTHREADLIB)
# LIBES3 = $(CALCLIB) $(RASTER3DLIB) $(GISLIB) $(RASTERLIB) $(BTREELIB) $(READLINELIBPATH) $(READLINELIB) $(HISTORYLIB) $(PTHREADLIBPATH) $(PTHREADLIB)

# default: multi

# $(BIN)/$(PGM2)$(EXE): LIBES = $(LIBES2)
# $(BIN)/$(PGM3)$(EXE): LIBES = $(LIBES3)

# $(OBJDIR)/mapcalc.yy.o: mapcalc.tab.h

# .SECONDARY: mapcalc.tab.c mapcalc.tab.h mapcalc.output


FLEX_TARGET(mapcalc.yy.c    mapcalc.l  ${CMAKE_CURRENT_BINARY_DIR}/mapcalc.yy.c  )
BISON_TARGET(mapcalc.tab.c  mapcalc.y  ${CMAKE_CURRENT_BINARY_DIR}/mapcalc.tab.c )
ADD_FLEX_BISON_DEPENDENCY(mapcalc.yy.c mapcalc.tab.c)
SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_BINARY_DIR}/mapcalc.yy.c  GENERATED)
SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_BINARY_DIR}/mapcalc.tab.c GENERATED)

set(r_mapcalc_common_SRCS
  column_shift.c evaluate.c expression.c function.c main.c xrowcol.c
  ${CMAKE_CURRENT_BINARY_DIR}/mapcalc.tab.c
  ${CMAKE_CURRENT_BINARY_DIR}/mapcalc.yy.c )

set(r_mapcalc_SRCS  ${r_mapcalc_common_SRCS} map.c xarea.c xcoor.c xres.c)
set(r3_mapcalc_SRCS ${r_mapcalc_common_SRCS} map3.c xarea.c xcoor3.c xres3.c)

build_program(NAME r.mapcalc
   SOURCES "${r_mapcalc_SRCS}"
   DEPENDS grass_gis grass_btree grass_calc grass_raster )

build_program(NAME r3.mapcalc
   SOURCES "${r3_mapcalc_SRCS}"
   DEPENDS grass_gis grass_btree grass_calc grass_raster3d )
