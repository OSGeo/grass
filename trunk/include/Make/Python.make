
PY_SOURCES := $(wildcard *.py)

%.pyc: %.py
	$(PYTHON) -t -m py_compile $<
