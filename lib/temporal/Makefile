MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Other.make

SQLDIR = $(ETC)/sql

SQLFILES:= $(wildcard *.sql)

default: copy_sql_files

copy_sql_files: $(SQLDIR)
	for i in $(SQLFILES); do $(INSTALL_DATA) $$i $(SQLDIR)/; done

$(SQLDIR): 
	$(MKDIR) $@