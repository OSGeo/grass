
MODULE_TOPDIR = ../../..

include $(MODULE_TOPDIR)/include/Make/Platform.make
include $(MODULE_TOPDIR)/include/Make/Grass.make
include $(MODULE_TOPDIR)/include/Make/Rules.make

LIBES = $(RASTERLIB) $(GISLIB)
DEPENDENCIES = $(GISDEP)

default:$(ETC)/mon.list \
	$(ETC)/mon.release \
	$(ETC)/mon.select \
	$(ETC)/mon.start \
	$(ETC)/mon.status \
	$(ETC)/mon.stop \
	$(ETC)/mon.which

$(ETC)/mon.list:	$(OBJDIR)/list.o $(DEPENDENCIES)
	$(CC) $(LDFLAGS) -o $@ $(OBJDIR)/list.o $(LIBES) $(MATHLIB) $(XDRLIB)
$(ETC)/mon.start:	$(OBJDIR)/start.o $(DEPENDENCIES)
	$(CC) $(LDFLAGS) -o $@ $(OBJDIR)/start.o $(LIBES) $(MATHLIB) $(XDRLIB)
$(ETC)/mon.status:	$(OBJDIR)/status.o $(DEPENDENCIES) 
	$(CC) $(LDFLAGS) -o $@ $(OBJDIR)/status.o $(LIBES) $(MATHLIB) $(XDRLIB)
$(ETC)/mon.stop:	$(OBJDIR)/stop.o $(DEPENDENCIES) 
	$(CC) $(LDFLAGS) -o $@ $(OBJDIR)/stop.o $(LIBES) $(MATHLIB) $(XDRLIB)
$(ETC)/mon.which:	$(OBJDIR)/which.o $(DEPENDENCIES) 
	$(CC) $(LDFLAGS) -o $@ $(OBJDIR)/which.o $(GISLIB) $(MATHLIB) $(XDRLIB) 
$(ETC)/mon.select:	$(OBJDIR)/select.o $(DEPENDENCIES) 
	$(CC) $(LDFLAGS) -o $@ $(OBJDIR)/select.o $(DISPLAYLIB) $(LIBES) $(MATHLIB) $(XDRLIB)
$(ETC)/mon.release:	$(OBJDIR)/release.o $(DEPENDENCIES) 
	$(CC) $(LDFLAGS) -o $@ $(OBJDIR)/release.o $(LIBES)  $(MATHLIB) $(XDRLIB)

#list.o: $(SRC)/include/monitors.h
#release.o: $(SRC)/include/monitors.h
#release.o: open.h
#select.o: $(SRC)/include/monitors.h
#start.o: $(SRC)/include/monitors.h
#status.o: $(SRC)/include/monitors.h
#status.o: open.h
#stop.o: $(SRC)/include/monitors.h
#stop.o: open.h
