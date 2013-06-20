
PY_SOURCES := $(wildcard *.py)

%.pyc: %.py
	$(PYTHON) -t -3 -m py_compile $<
