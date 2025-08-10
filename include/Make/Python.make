
PY_SOURCES := $(wildcard *.py)

%.pyc: %.py
	$(PYTHON) -m py_compile $<
