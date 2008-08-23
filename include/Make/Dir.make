
# common dependencies and rules for building subdirs

include $(MODULE_TOPDIR)/include/Make/Vars.make
include $(MODULE_TOPDIR)/include/Make/Rules.make
include $(MODULE_TOPDIR)/include/Make/Html.make

subdirs:
	@list='$(SUBDIRS)'; \
	for subdir in $$list; do \
	    echo $$subdir ; \
	    $(MAKE) -C $$subdir || echo $(CURDIR)/$$subdir >> $(ERRORLOG) ; \
	done

cleansubdirs:
	@list='$(SUBDIRS)'; \
	for subdir in $$list; do \
	    echo $$subdir ; \
	    $(MAKE) -C $$subdir clean; \
	done

clean: cleansubdirs

htmldir: html

.PHONY: subdirs cleansubdirs parsubdirs htmldir $(SUBDIRS)

parsubdirs: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@ || echo $(CURDIR)/$@ >> $(ERRORLOG)

