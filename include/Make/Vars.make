include $(MODULE_TOPDIR)/include/Make/Platform.make
include $(MODULE_TOPDIR)/include/Make/Grass.make

ifndef BROKEN_MAKE
ifneq ($(MAKE_VERSION),3.81)
BROKEN_MAKE=1
endif
endif

C_SOURCES    := $(wildcard *.c)
CPP_SOURCES  := $(wildcard *.cpp)
LEX_SOURCES  := $(wildcard *.l)
YACC_SOURCES := $(wildcard *.y)

AUTO_OBJS := \
	$(sort $(subst .c,.o,$(C_SOURCES))) \
	$(sort $(subst .cpp,.o,$(CPP_SOURCES))) \
	$(sort $(subst .l,.yy.o,$(LEX_SOURCES))) \
	$(sort $(subst .y,.tab.o,$(YACC_SOURCES)))

ifndef MOD_OBJS
MOD_OBJS = $(AUTO_OBJS)
endif

ARCH_OBJS = $(patsubst %.o,$(OBJDIR)/%.o,$(MOD_OBJS))

LOCAL_HEADERS := $(wildcard *.h)

LINK = $(CC)

