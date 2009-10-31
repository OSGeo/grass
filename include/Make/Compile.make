
# for i18N support
NLS_CFLAGS = -DPACKAGE=\"$(PACKAGE)\"

LINK = $(CC)

linker_x = $(1) $(LDFLAGS) $(EXTRA_LDFLAGS) -o $@ $(filter %.o,$^) $(FMODE_OBJ) $(LIBES) $(EXTRA_LIBS) $(MATHLIB) $(XDRLIB)
linker_c = $(call linker_x,$(CC))
linker_cxx = $(call linker_x,$(CXX))
linker = $(call linker_x,$(LINK))

compiler_x = $(1) $(2) $(EXTRA_CFLAGS) $(NLS_CFLAGS) $(DEFS) $(EXTRA_INC) $(INC) -o $@ -c $<
compiler_c = $(call compiler_x,$(CC),$(CFLAGS) $($*_c_FLAGS))
compiler_cxx = $(call compiler_x,$(CXX),$(CXXFLAGS) $($*_cc_FLAGS) $($*_cpp_FLAGS))
compiler = $(call compiler_x,$(CC))

# default cc rules
$(OBJDIR)/%.o : %.c $(LOCAL_HEADERS) $(EXTRA_HEADERS) | $(OBJDIR)
	$(call compiler_c)

$(OBJDIR)/%.o : %.cc $(LOCAL_HEADERS) $(EXTRA_HEADERS) | $(OBJDIR)
	$(call compiler_cxx)

$(OBJDIR)/%.o : %.cpp $(LOCAL_HEADERS) $(EXTRA_HEADERS) | $(OBJDIR)
	$(call compiler_cxx)

# default parser generation rules, include prefix for files/vars
%.yy.c: %.l
	$(LEX) $(LFLAGS) -t $< > $@

%.output %.tab.h %.tab.c: %.y
	$(YACC) -b$* $(YFLAGS) $<
