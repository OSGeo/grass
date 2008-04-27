MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Platform.make

EXTRA_CFLAGS=$(FTINC)
EXTRA_LIBS=$(GISLIB) $(FTLIB) $(ICONVLIB)
LIB_NAME = $(DRIVER_LIBNAME)

LIB_OBJS =\
	Bitmap.o\
	Box.o\
	Client.o\
	Color.o\
	Cont.o\
	Draw.o\
	Erase.o\
	Font.o\
	Get_location.o\
	Get_t_box.o\
	Graph.o\
	Line_width.o\
	Move.o\
	Panel.o\
	Polydots.o\
	Polyline.o\
	Polygon.o\
	Raster.o\
	Respond.o\
	Returns.o\
	Set_window.o\
	Text.o\
	Text_size.o\
	Work.o\
	font2.o\
	font_freetype.o\
	init.o\
	pad.o\
	parse_ftcap.o\
	text2.o\
	text3.o

ifneq ($(HAVE_SOCKET),)
  LIB_OBJS += \
	main.o\
	command.o\
	connect_sock.o
endif

include $(MODULE_TOPDIR)/include/Make/Lib.make

default: lib

