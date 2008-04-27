MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Platform.make

PACKAGE ="libcairodriver"
DEFS=-DPACKAGE=\"$(PACKAGE)\"

EXTRA_CFLAGS=-I../driver $(CAIROINC) $(PICFLAGS) $(DEFS)
EXTRA_LIBS=$(DRIVERLIB) $(GISLIB) $(CAIROLIB)

# added constant to /include/Make/Grass.make.in
LIB_NAME = $(CAIRODRIVER_LIBNAME)

LIB_OBJS = \
	Driver.o	\
	Client.o	\
	Box.o		\
	Graph.o		\
	Erase.o		\
	Set_window.o	\
	Line_width.o	\
	Draw_line.o	\
	Draw_bitmap.o	\
	Draw_point.o	\
	Poly.o		\
	Raster.o	\
	Respond.o	\
	Color.o		\
	read.o		\
	read_bmp.o	\
	read_ppm.o	\
	write.o		\
	write_bmp.o	\
	write_ppm.o

include $(MODULE_TOPDIR)/include/Make/Lib.make

default: lib
