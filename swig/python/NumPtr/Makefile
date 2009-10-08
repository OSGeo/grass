# build NumPtr

all: numptr

numptr:
	$(PYTHON) setup.py build --build-lib build/lib
	# build bytecode .pyc file
	$(PYTHON) build/lib/test.py >& /dev/null

clean:
	-rm -rf build/
	-rm -f setup.pyc
	$(MAKE) -C src clean
