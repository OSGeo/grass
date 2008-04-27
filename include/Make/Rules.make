# lexical analyzer and default options
LEXFLAGS= -d -i -s -t

# parser generator and default options
YACCFLAGS = -d -v -t

ifndef LOCAL_HEADERS
LOCAL_HEADERS = $(wildcard *.h)
endif

# for i18N support
DEFS=-DPACKAGE=\"$(PACKAGE)\"
NLS_CFLAGS=$(ZLIBINCPATH) $(PICFLAGS) $(DEFS)

ifndef MOD_OBJS
MOD_OBJS := $(subst .c,.o,$(wildcard *.c))
endif

ifndef CMD_OBJS
CMD_OBJS := $(MOD_OBJS)
endif

ifndef ARCH_CMD_OBJS
ARCH_CMD_OBJS := $(foreach obj,$(CMD_OBJS),OBJ.$(ARCH)/$(obj))
endif

$(OBJDIR):
	-test -d $(OBJDIR) || $(MKDIR) $(OBJDIR)

ifndef BROKEN_MAKE
ifneq ($(MAKE_VERSION),3.81)
BROKEN_MAKE=1
endif
endif

# default cc rules
ifeq ($(BROKEN_MAKE),)
$(OBJDIR)/%.o : %.c $(LOCAL_HEADERS) $(EXTRA_HEADERS) | $(OBJDIR)
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) $(NLS_CFLAGS) $(EXTRA_INC) $(INC) -o $@ -c $<

$(OBJDIR)/%.o : %.cc $(LOCAL_HEADERS) $(EXTRA_HEADERS) | $(OBJDIR)
	$(CXX) $(CXXFLAGS) $(EXTRA_CFLAGS) $(NLS_CFLAGS) $(EXTRA_INC) $(INC) -o $@ -c $<

$(OBJDIR)/%.o : %.cpp $(LOCAL_HEADERS) $(EXTRA_HEADERS) | $(OBJDIR)
	$(CXX) $(CXXFLAGS) $(EXTRA_CFLAGS) $(NLS_CFLAGS) $(EXTRA_INC) $(INC) -o $@ -c $<
else
$(OBJDIR)/%.o : %.c $(LOCAL_HEADERS) $(EXTRA_HEADERS)
	$(MAKE) $(OBJDIR)
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) $(NLS_CFLAGS) $(EXTRA_INC) $(INC) -o $@ -c $<

$(OBJDIR)/%.o : %.cc $(LOCAL_HEADERS) $(EXTRA_HEADERS)
	$(MAKE) $(OBJDIR)
	$(CXX) $(CXXFLAGS) $(EXTRA_CFLAGS) $(NLS_CFLAGS) $(EXTRA_INC) $(INC) -o $@ -c $<

$(OBJDIR)/%.o : %.cpp $(LOCAL_HEADERS) $(EXTRA_HEADERS)
	$(MAKE) $(OBJDIR)
	$(CXX) $(CXXFLAGS) $(EXTRA_CFLAGS) $(NLS_CFLAGS) $(EXTRA_INC) $(INC) -o $@ -c $<
endif

# default parser generation rules, include prefix for files/vars
%.yy.c: %.l
	$(LEX) -P$* $(LEXFLAGS) $*.l | \
	$(SED) -e 's/unistd.h/limits.h/g' \
	> $@

%.tab.h %.tab.c: %.y
	$(YACC) -b$* -p$* $(YACCFLAGS) $<


# default clean rules
clean:
	-rm -rf $(OBJDIR) $(EXTRA_CLEAN_DIRS)
	-rm -f $(EXTRA_CLEAN_FILES) *.tmp.html
	-if [ "$(CLEAN_SUBDIRS)" != "" ] ; then \
		for dir in $(CLEAN_SUBDIRS) ; do \
			$(MAKE) -C $$dir clean ; \
		done ; \
	fi

# HTML page rules:
include $(MODULE_TOPDIR)/include/Make/Html.make
