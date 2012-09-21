MODULE_TOPDIR = ../../../..

OBJS = \
	$(OBJDIR)/do_dots.o\
	$(OBJDIR)/do_line.o\
	$(OBJDIR)/find_area.o\
	$(OBJDIR)/ply_t_bmif.o\
	$(OBJDIR)/save_area.o\
	$(OBJDIR)/wrte_recrd.o\
	$(OBJDIR)/yadjust.o

LIBES = $(GISLIB)
DEPENDENCIES= $(GISDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

default: $(ETC)/bmif_to_cell $(ETC)/poly_to_bmif

$(ETC)/bmif_to_cell: $(OBJDIR)/bmf_to_cll.o $(DEPENDENCIES)
	$(CC) $(LDFLAGS) -o $@ $< $(FMODE_OBJ) $(LIBES) $(MATHLIB)

$(ETC)/poly_to_bmif: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(FMODE_OBJ)

