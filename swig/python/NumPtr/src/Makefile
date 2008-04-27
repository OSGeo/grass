# Makes SWIGed Python module 

# Rodrigo Caballero (rca@geosci.uchicago.edu), 
# Department of the Geophysical Sciences, University of Chicago
# October 2003

name := NumPtr

ifeq ($(MAKELEVEL),0)
  CC := pgcc
  F90 := pgf90
  PYTHONINC := /usr/include/python2.2 
#  PYTHONINC := /sw/include/python2.3 
  CCFLAGS :=
  F90FLAGS := -r8
  LDFLAGS := -Mnomain -shared
  SWIG := /home/rca/bin/swig
#  SWIG := swig
endif

OS = $(shell uname -s)
ifeq ($(OS),Darwin)
  CC := cc
  CCFLAGS := -fno-common
  F90 := cc
  LDFLAGS := -bundle -flat_namespace -undefined suppress
  SWIG := swig
endif

%.o : %.c
	$(CC) $(CCFLAGS) -c -I$(PYTHONINC) -o $@ $<

%.o : %.f
	$(F90) $(F90FLAGS) -c -o $@ $<

SRC := $(wildcard *.f *.c)
OBJ := $(SRC:.f=.o)
OBJ := $(OBJ:.c=.o)
ifeq (,$(findstring $(name)_wrap.o,$(OBJ)))
 wrapper := $(name)_wrap.o
endif

_$(name)_lib.so :  $(OBJ) $(name)_wrap.o
	$(F90) $(LDFLAGS) -o $@  $(OBJ) $(wrapper)

$(name)_wrap.o: $(name).i
	$(SWIG) -python $(name).i
	$(CC) -c -I$(PYTHONINC) -o $(name)_wrap.o $(name)_wrap.c

clean:
	rm -f $(OBJ) *.so *_wrap.* *.pyc *_lib.*

test:
	python test.py
