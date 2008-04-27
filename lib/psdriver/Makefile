MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Platform.make

PACKAGE ="libpsdriver"
DEFS=-DPACKAGE=\"$(PACKAGE)\"
EXTRA_CFLAGS=-I../driver $(PICFLAGS) $(DEFS)

EXTRA_LIBS=$(DRIVERLIB) $(GISLIB)
LIB_NAME = $(PSDRIVER_LIBNAME)

LIB_OBJS =	\
	Box.o			\
	Client.o		\
	Color.o			\
	Color_table.o		\
	Draw_bitmap.o		\
	Draw_line.o		\
	Draw_point.o		\
	Driver.o		\
	Erase.o			\
	Graph_close.o		\
	Graph_set.o		\
	Line_width.o		\
	Polygon.o		\
	Polyline.o		\
	Raster.o		\
	Respond.o		\
	Set_window.o

include $(MODULE_TOPDIR)/include/Make/Lib.make

default: lib $(ETC)/psdriver.ps

$(ETC)/psdriver.ps: psdriver.ps
	$(INSTALL_DATA) $< $@
