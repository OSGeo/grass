MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Vars.make

MOD_OBJS := bitmap.o sparse.o

LIB = BITMAP

include $(MODULE_TOPDIR)/include/Make/Lib.make


default: lib


#$(OBJ): #linkm.h

main: main.o $(BITMAPLIB) $(LINKMLIB)
	$(CC) $(LDFLAGS) main.o $(BITMAPLIB) $(LINKMLIB) -o main

smain: smain.o $(BITMAPLIB) $(LINKMLIB)
	$(CC) $(LDFLAGS) smain.o $(BITMAPLIB) $(LINKMLIB) -o smain

#$(LINKMLIB): #
