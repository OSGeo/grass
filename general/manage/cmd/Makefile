
MODULE_TOPDIR = ../../..

PROGRAMS = g.copy g.list g.remove g.rename

g_copy_OBJS = copy.o init.o
g_list_OBJS = list.o init.o
g_remove_OBJS = remove.o init.o
g_rename_OBJS = rename.o init.o

include $(MODULE_TOPDIR)/include/Make/Multi.make

LIBES = $(MANAGELIB) $(VECTLIB) $(DBMILIB) $(GISLIB) $(G3DLIB)
LIST = $(OBJDIR)/init.o  
DEPENDENCIES = $(GISDEP) $(VECTDEP) $(DBMIDEP) $(MANAGEDEP) $(G3DDEP)

default: multi

