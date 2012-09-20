
# common dependencies and rules for building subdirs

include $(MODULE_TOPDIR)/include/Make/Vars.make
include $(MODULE_TOPDIR)/include/Make/Rules.make
include $(MODULE_TOPDIR)/include/Make/Html.make

# don't install *.png, *.jpg for directories
# to prevent problems with r.out.png etc
IMGSRC := 

subdirs:
	@list='$(SUBDIRS)'; \
	for subdir in $$list; do \
	    $(MAKE) -C $$subdir || echo $(CURDIR)/$$subdir >> $(ERRORLOG) ; \
	done

installsubdirs:
	@list='$(SUBDIRS)'; \
	for subdir in $$list; do \
	    $(MAKE) -C $$subdir install ; \
	done

cleansubdirs:
	@list='$(SUBDIRS)'; \
	for subdir in $$list; do \
	    echo $$subdir ; \
	    $(MAKE) -C $$subdir clean; \
	done

%-recursive:
	@list='$(SUBDIRS)'; \
	for subdir in $$list; do \
	    $(MAKE) -C $$subdir $*; \
	done

clean: clean-recursive

depend: depend-recursive

htmldir: html

.PHONY: subdirs parsubdirs htmldir $(SUBDIRS)

parsubdirs: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@ || echo $(CURDIR)/$@ >> $(ERRORLOG)
