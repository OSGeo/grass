MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Platform.make

EXTRA_CFLAGS=$(GETHOSTNAME) $(ZLIBINCPATH) $(PNGINC) -I../driver

EXTRA_LIBS=$(DRIVERLIB) $(GISLIB) $(PNGLIB)
LIB_NAME = $(PNGDRIVER_LIBNAME)
PGM = cairodriver

LIB_OBJS =	\
	Box.o			\
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
	Raster.o		\
	Respond.o		\
	Set_window.o		\
	read.o			\
	read_ppm.o		\
	read_bmp.o		\
	write.o			\
	write_ppm.o		\
	write_bmp.o		

ifneq ($(USE_PNG),)
  LIB_OBJS += read_png.o write_png.o
endif

include $(MODULE_TOPDIR)/include/Make/Lib.make

default: lib htmldir
