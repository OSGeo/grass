
# common dependencies and rules for building DBMI driver

PACKAGE = "grassmods"

include $(MODULE_TOPDIR)/include/Make/Vars.make
include $(MODULE_TOPDIR)/include/Make/Rules.make
include $(MODULE_TOPDIR)/include/Make/Html.make
include $(MODULE_TOPDIR)/include/Make/Compile.make
include $(MODULE_TOPDIR)/include/Make/Rest.make

dbmi: $(DBDRIVERDIR)/$(PGM)$(EXE) db_html

db_html: $(HTMLDIR)/grass-$(PGM).html $(MANDIR)/grass-$(PGM).$(MANSECT)

restdocs: $(DBDRIVERDIR)/$(PGM)$(EXE) db_rest

db_rest: $(RESTDIR)/grass-$(PGM).txt

$(DBDRIVERDIR)/$(PGM)$(EXE): $(ARCH_OBJS) $(DEPENDENCIES)
	$(call linker)

.PHONY: dbmi db_html
