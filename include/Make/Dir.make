
# common dependencies and rules for building subdirs

include $(MODULE_TOPDIR)/include/Make/Platform.make
include $(MODULE_TOPDIR)/include/Make/Grass.make
include $(MODULE_TOPDIR)/include/Make/Rules.make

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

.PHONY: subdirs cleansubdirs parsubdirs $(SUBDIRS)

parsubdirs: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@ || echo $(CURDIR)/$@ >> $(ERRORLOG)

