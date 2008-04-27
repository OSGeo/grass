
MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Module.make

PGM = current_time_s_ms

default: $(ETC)/$(PGM)

$(ETC)/$(PGM): main.c
	$(CC) -c $<
	$(CC) -o $(ETC)/$(PGM) $<

clean:
	\rm -f $(ETC)/$(PGM) main.o
