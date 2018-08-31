MODULE_TOPDIR = ../../..

include $(MODULE_TOPDIR)/include/Make/Other.make

SQLDIR = $(ETC)/sql
SQLFILES:= $(wildcard *.sql)
DSTFILES := $(patsubst %.sql,$(SQLDIR)/%.sql,$(SQLFILES))

default: $(DSTFILES)
 
$(SQLDIR):
	$(MKDIR) $@
 
$(SQLDIR)/%.sql: %.sql | $(SQLDIR)
	$(INSTALL_DATA) $< $@
