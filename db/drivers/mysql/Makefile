MODULE_TOPDIR = ../../..

DEPENDENCIES = $(GISDEP)

include $(MODULE_TOPDIR)/include/Make/Vars.make

ifeq ($(PGM),mysql)
MOD_OBJS := $(filter-out dbe.o,$(MOD_OBJS))
else
MOD_OBJS := $(filter-out db.o,$(MOD_OBJS))
endif

include $(MODULE_TOPDIR)/include/Make/DB.make

LIBES = $(DBMIDRIVERLIB) $(DBMIBASELIB) $(DBSTUBSLIB) \
	 $(GISLIB) $(DATETIMELIB) 
EXTRA_CFLAGS = $(MYSQLINCPATH)
EXTRA_LDFLAGS = $(MYSQLLIBPATH)

ifeq ($(PGM),mysql)
LIBES += $(MYSQLLIB)
else
LIBES += $(MYSQLDLIB)
LINK = $(CXX)
endif

ifeq ($(PGM),)
default:
	$(MAKE) PGM=mysql
ifneq ($(strip $(MYSQLDLIB)),)
ifneq ($(strip $(CXX)),)
	$(MAKE) PGM=mesql
endif
endif
else
default: dbmi
endif
