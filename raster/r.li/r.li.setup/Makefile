TARGET= $(ETC)/r.li.setup

MODULE_TOPDIR = ../../..

PGM = r.li.setup

include $(MODULE_TOPDIR)/include/Make/Script.make

SCRIPT_SRC = \
	area_query \
	masked_area_selection.sh \
	r.li.setup.main \
	r.li.setup.procedures.tcl \
	r.li.windows.tcl \
	sample_area_vector.sh \
	square_mouse_selection.sh \
	square_query

SCRIPT_DST := $(patsubst %,$(TARGET)/%,$(SCRIPT_SRC))

DATA_SRC = circle.txt polygon.txt

DATA_DST := $(patsubst %,$(TARGET)/%,$(DATA_SRC))

default: script $(SCRIPT_DST) $(DATA_DST)

$(TARGET)/%.txt: %.txt | $(TARGET)
	$(INSTALL_DATA) $< $@

$(TARGET)/%: % | $(TARGET)
	$(INSTALL) $< $@

$(TARGET):
	$(MKDIR) $@

