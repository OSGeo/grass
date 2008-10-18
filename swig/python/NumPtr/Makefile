# build NumPtr

all: numptr

numptr:
	python setup.py build --build-lib build/lib
	# build bytecode .pyc file
	python build/lib/test.py >& /dev/null

clean:
	-rm -rf build/
	-rm -f setup.pyc
	$(MAKE) -C src clean
