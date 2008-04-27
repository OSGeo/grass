MODULE_TOPDIR = ../..

PGM = i.vpoints

LIBES     = $(IMAGERYLIB) $(SYMBLIB) $(DISPLAYLIB) $(RASTERLIB) $(VECTLIB) $(GISLIB) $(CURSES) $(GMATHLIB)
DEPENDENCIES= $(IMAGERYDEP) $(SYMBDEP) $(DISPLAYDEP) $(RASTERDEP) $(VECTDEP) $(GISDEP) $(GMATHDEP)
EXTRA_INC = $(VECT_INC)
EXTRA_CFLAGS = $(VECT_CFLAGS)

MOD_OBJS =  main.o \
        ask.o \
        ask_mag.o \
        analyze.o \
        call.o \
        cell.o \
        cellhd.o \
        conv.o \
        colors.o \
        curses.o \
        crs.o \
        debug.o \
        digit.o \
        dot.o \
        drawcell.o \
        drawvect.o \
        driver.o \
        equ.o \
        find.o \
        graphics.o \
        group.o \
        input.o \
        mark.o \
        mouse.o \
        plot.o \
        points.o \
        setup.o \
        target.o \
        title.o \
        view.o \
        where.o \
        zoom.o \
        zoom_box.o \
        zoom_pnt.o

include $(MODULE_TOPDIR)/include/Make/Module.make

default: cmd
