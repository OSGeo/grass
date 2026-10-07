## GIS library

### Documentation

The programmer's manual pages of the library are the `*.dox` and `*.dox.md`
files in this directory. Doxygen builds them with `make htmldox`.

The figures of the page on random numbers, `random_streams.dox.md`, are the
`random_streams*.svg` files. The script `random_streams_figures.py` writes
them and prints the numbers quoted on the page:

```sh
python3 random_streams_figures.py
python3 random_streams_figures.py --numbers
```
