
# first found target
first: pre default

# create platform dirs 
ARCH_DIRS = $(ARCH_DISTDIR) $(ARCH_BINDIR) $(ARCH_INCDIR) $(ARCH_LIBDIR) \
	$(BIN) $(ETC) \
	$(DRIVERDIR) $(DBDRIVERDIR) $(FONTDIR) $(DOCSDIR) $(HTMLDIR) \
	$(MANBASEDIR) $(MANDIR)

pre: $(ARCH_DIRS)

$(ARCH_DIRS):
	$(MKDIR) $@

$(OBJDIR):
	-test -d $(OBJDIR) || $(MKDIR) $(OBJDIR)

# default clean rules
clean:
	-rm -rf $(OBJDIR) $(EXTRA_CLEAN_DIRS)
	-rm -f $(EXTRA_CLEAN_FILES) *.tab.[ch] *.yy.c *.output *.backup *.tmp.html
	-if [ "$(CLEAN_SUBDIRS)" != "" ] ; then \
		for dir in $(CLEAN_SUBDIRS) ; do \
			$(MAKE) -C $$dir clean ; \
		done ; \
	fi

