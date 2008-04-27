
MODULE_TOPDIR = ../../..

PGM=xdriver

include $(MODULE_TOPDIR)/include/Make/Platform.make
include $(MODULE_TOPDIR)/include/Make/Grass.make
include $(MODULE_TOPDIR)/include/Make/Rules.make

LIBES = $(DRIVERLIB) $(GISLIB) $(FTLIB) $(ICONVLIB)
DEPENDENCIES = $(DRIVERDEP) $(GISDEP)

# if you are compiling under X11R3 add this define
#R3DEFINE=-DX11R3
# and comment out this line
R3DEFINE=

# For those compiling on Intergraph machines, uncomment the following 2 lines:
#XCFLAGS=-DINTERGRAPH -DX11R3

# these are for the CDC mips machine
#XCFLAGS=-systype bsd43
#XLDFLAGS=-systype bsd43

# One user reported that he had to load -lsockstub for SCO-UNIX under X11R3
# we record his comment here for anyone that may have problems under SCO-UNIX
# This problem was NOT reported when CWU did the port to SCO-UNIX
#XEXTRALIBS=-lsockstub

# On the Silcon Graphics Boxes, the standard XOpenDisplay() fails when
# trying to display to another machine if the machine is named on
# the DISPLAY variable instead of it IP address. Loading the -lsun
# library resolves this problem
#XEXTRALIBS=-lsun

EXTRA_CFLAGS=$(XCFLAGS) $(R3DEFINE) -I../../../lib/driver

OBJS =  \
	Box.o\
	Client.o\
	Color_table.o\
	Color.o\
	Draw_bitmap.o\
	Draw_line.o\
	Draw_point.o\
	Get_w_box.o\
	Get_w_line.o\
	Get_w_pointer.o\
	Graph_close.o\
	Graph_set.o\
	Line_width.o\
	Panel.o\
	Polydots.o\
	Polygon.o\
	Polyline.o\
	Raster.o\
	Respond.o\
	Serve_Xevent.o\
	Set_window.o\
	Work.o\
	alloc.o\
	main.o

ARCH_OBJS := $(foreach obj,$(OBJS),OBJ.$(ARCH)/$(obj))

#FREETYPEBASED = Draw_point.o
#compile if FreeType Lib present:
#ifneq ($(strip $(FTLIB)),)
#    OBJS += $(FREETYPEBASED)
#endif

default: $(DRIVERDIR)/XDRIVER $(HTMLDIR)/$(PGM).html moncap

moncap:
	@if [ -f $(ETC)/monitorcap ] && egrep '^x0' $(ETC)/monitorcap >/dev/null 2>&1 ; then \
	echo XDRIVER already defined in $(ETC)/monitorcap ; \
	else \
	echo adding XDRIVER to $(ETC)/monitorcap ; \
	cat moncap.entry >> $(ETC)/monitorcap ; \
	fi

$(DRIVERDIR)/XDRIVER: $(ARCH_OBJS) $(DEPENDENCIES)
	$(CC) $(LDFLAGS) -o $@ $(XLDFLAGS) $(LIBPATH) $(ARCH_OBJS) $(LIBES) $(XLIBPATH) $(XLIB) $(XEXTRALIBS) $(MATHLIB) $(XDRLIB)
