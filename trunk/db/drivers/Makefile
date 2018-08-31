
MODULE_TOPDIR = ../..

#always compile dialog lib and dbf driver:
SUBDIRS1 = dbf

include $(MODULE_TOPDIR)/include/Make/Vars.make

# add odbc:
ifneq ($(strip $(ODBCLIB)),)
    SUBDIRS2 = odbc
endif

# add postgresql:
ifneq ($(strip $(PQLIB)),)
    SUBDIRS3 = postgres
endif

# add mysql:
ifneq ($(strip $(MYSQLLIB)),)
    SUBDIRS4 = mysql
endif

# add SQLite:
ifneq ($(strip $(SQLITELIB)),)
    SUBDIRS5 = sqlite
endif

# add ogr:
ifneq ($(strip $(USE_OGR)),)
    SUBDIRS6 = ogr
endif

SUBDIRS =  $(SUBDIRS1) $(SUBDIRS2) $(SUBDIRS3) $(SUBDIRS4) $(SUBDIRS5) $(SUBDIRS6)

include $(MODULE_TOPDIR)/include/Make/Dir.make

default: parsubdirs
