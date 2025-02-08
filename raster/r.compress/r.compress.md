## DESCRIPTION

*r.compress* can be used to compress or decompress raster maps.
Additionally, it prints information about the compression method and
data type of the input raster map(s).

All raster maps (those imported for the first time and those newly
generated) are compressed by default using the ZSTD compression method
if available, otherwise ZLIB compression is used (see below). Related no
data files (i.e.: NULL files), if present, are compressed by default
unless a specific environment variable is set to explicitly disable NULL
file compression (`GRASS_COMPRESS_NULLS`, see below).

During compression or re-compression, *r.compress* compresses raster
maps using the method specified by means of the environment variable
`GRASS_COMPRESSOR`. The default compression method is ZSTD if available,
otherwise ZLIB's "deflate" algorithm (LZ77-based). Raster maps that
contain very little information (such as boundary, geology, soils and
land use maps) can be greatly reduced in size. Some raster maps are
shrunk to roughly 1% of their original sizes. All newly generated raster
maps are automatically stored as compressed data with varying methods
depending on the raster format (i.e., CELL: integer; FCELL: single
precision; DCELL: double precision; see below). All GRASS GIS modules
are able to read both compressed and uncompressed raster maps.

Raster maps that are already compressed might be compressed again,
either by setting a different method with `GRASS_COMPRESSOR` (supported
methods: RLE, ZLIB, LZ4, BZIP2, ZSTD) or, for the case of ZLIB
compression, by changing the compression level with the environment
variable `GRASS_ZLIB_LEVEL`.

Compressed raster maps may be decompressed using *r.compress* with the
**-u** flag. If a raster map was already decompressed and the **-u**
flag is set, the module simply informs the user that the map is already
decompressed and exits.

Information about the compression method and data type of the input
raster map(s) can be printed in shell style with the **-g** flag. In
this case, the module prints to `stdout` one line per input map with the
fields "input map name", "data type", "name of data compression method",
"NULL file compression" separated by the pipe character. NULL file
compression is indicated with "YES" or "NO".

### TERMINOLOGY

- INTEGER map (CELL data type): a raster map of INTEGER type (whole
  numbers only)
- FLOAT map (FCELL data type): a raster map of FLOAT type (4 bytes, 7-9
  digits precision)
- DOUBLE map (DCELL data type): a raster map of DOUBLE type (8 bytes,
  15-17 digits precision)
- NULL: represents "no data" in raster maps; to be distinguished from 0
  (zero) data value

### OVERVIEW OF AVAILABLE COMPRESSION ALGORITHMS

The following compression methods are available (set by
`export GRASS_COMPRESSOR=`*`method`*):

- `NONE` (uncompressed)
- `RLE` (generic Run-Length Encoding of single bytes; deprecated)
- `ZLIB` (DEFLATE, good speed and compression)
  - with zlib compression levels (`export GRASS_ZLIB_LEVEL=X`): -1..9
    (-1 is default which corresponds to ZLIB level 6)
  - note: `export GRASS_ZLIB_LEVEL=0` is equal to copying the data as-is
    from source to destination
- `LZ4` (fastest, low compression)
- `BZIP2` (slowest, high compression)
- `ZSTD` (compared to ZLIB, faster and higher compression, much faster
  decompression - **default compression**)

Important: the NULL file compression can be turned off with
`export GRASS_COMPRESS_NULLS=0`. Raster maps with NULL file compression
can only be opened with GRASS GIS 7.2.0 or later. NULL file compression
for a particular raster map can be managed with **r.null -z**. The NULL
file compression is using the LZ4 method as being the best compromise
between speed and compression rate.

### COMPRESSION ALGORITHM DETAILS

All GRASS GIS raster map types are by default ZSTD compressed if
available, otherwise ZLIB compressed. Through the environment variable
`GRASS_COMPRESSOR` the compression method can be set to RLE, ZLIB, LZ4,
BZIP2, or ZSTD.

Integer (CELL type) raster maps can be compressed with RLE if the
environment variable `GRASS_COMPRESSOR` exists and is set to RLE.
However, this is not recommended.

Floating point (FCELL, DCELL) raster maps never use RLE compression;
they are either compressed with ZLIB, LZ4, BZIP2, ZSTD or are
uncompressed.

**RLE**  
**DEPRECATED** Run-Length Encoding, poor compression ratio but fast. It
is kept for backwards compatibility to read raster maps created with
GRASS 6. It is only used for raster maps of type CELL. FCELL and DCELL
maps are never and have never been compressed with RLE.

**ZLIB**  
ZLIB's deflate is the default compression method for all raster maps, if
ZSTD is not available. GRASS GIS 8 uses by default 1 as ZLIB compression
level which is the best compromise between speed and compression ratio,
also when compared to other available compression methods. Valid levels
are in the range \[1, 9\] and can be set with the environment variable
`GRASS_ZLIB_LEVEL`.

**LZ4**  
LZ4 is a very fast compression method, about as fast as no compression.
Decompression is also very fast. The compression ratio is generally
higher than for RLE but worse than for ZLIB. LZ4 is recommended if disk
space is not a limiting factor.

**BZIP2**  
BZIP2 can provide compression ratios much higher than the other methods,
but only for large raster maps (\> 10000 columns). For large raster
maps, disk space consumption can be reduced by 30 - 50% when using BZIP2
instead of ZLIB's deflate. BZIP2 is the slowest compression and
decompression method. However, if reading from / writing to a storage
device is the limiting factor, BZIP2 compression can speed up raster map
processing. Be aware that for smaller raster maps, BZIP2 compression
ratio can be worse than other compression methods.

**ZSTD**  
ZSTD (Zstandard) provides compression ratios higher than ZLIB but lower
than BZIP2 (for large data). ZSTD compresses up to 4x faster than ZLIB,
and usually decompresses 6x faster than ZLIB. ZSTD is the default
compression method if available.

## NOTES

### Compression method number scheme

The used compression method is encoded with numbers. In the internal
`cellhd` file, the value for "compressed" is 1 for RLE, 2 for ZLIB, 3
for LZ4, 4 for BZIP2, and 5 for ZSTD.

Obviously, decompression is controlled by the raster map's compression,
not by the environment variable.

### Formats

Conceptually, a raster data file consists of rows of cells, with each
row containing the same number of cells. A cell consists of one or more
bytes. For CELL maps, the number of bytes per cell depends on the
category values stored in the cell. Category values in the range 0-255
require 1 byte per cell, while category values in the range 256-65535
require 2 bytes, and category values in the range above 65535 require 3
(or more) bytes per cell.

FCELL maps always have 4 bytes per cell and DCELL maps always have 8
bytes per cell.

Since GRASS GIS 7.0.0, the default compression method for Integer (CELL)
raster maps is ZLIB and no longer RLE.

### ZLIB compression levels

If the environment variable `GRASS_ZLIB_LEVEL` exists and its value can
be parsed as an integer, it determines the compression level used when
newly generated raster maps are compressed using ZLIB compression. This
applies to all raster map types (CELL, FCELL, DCELL).

If the variable does not exist, or the value cannot be parsed as an
integer, ZLIB's compression level 1 will be used.

## EXAMPLES

### Printing of current compression state

Example for an uncompressed raster map:

```sh
r.compress compressed_no -p
  <compressed_no> (method 0: NONE). Data type: <CELL>
```

### Applying ZLIB compression

Applying ZLIB compression to a copy of the uncompressed map from above:

```sh
# compression of map using ZLIB compression
g.copy raster=compressed_no,compressed_ZLIB

export GRASS_COMPRESSOR=ZLIB # ZLIB
r.compress compressed_ZLIB
r.compress compressed_ZLIB -p
  <compressed_ZLIB> is compressed (method 2: ZLIB). Data type: <CELL>
unset GRASS_COMPRESSOR # switch back to default
```

### Applying BZIP2 compression

Applying BZIP2 compression to a copy of the ZLIB-compressed map from
above:

```sh
# compression of map using BZIP2 compression
g.copy raster=compressed_ZLIB,compressed_BZIP2

export GRASS_COMPRESSOR=BZIP2 # BZIP2
r.compress compressed_BZIP2
r.compress compressed_BZIP2 -p
  <compressed_BZIP2> is compressed (method 4: BZIP2). Data type: <CELL>
unset GRASS_COMPRESSOR # switch back to default
```

### Applying ZSTD compression

Applying ZSTD compression to a copy of the BZIP2-compressed map from
above:

```sh
# compression of map using ZSTD compression
g.copy raster=compressed_BZIP2,compressed_ZSTD

export GRASS_COMPRESSOR=ZSTD # ZSTD
r.compress compressed_ZSTD
r.compress compressed_ZSTD -p
  <compressed_ZSTD> is compressed (method 5: ZSTD). Data type: <CELL>
unset GRASS_COMPRESSOR
```

## SEE ALSO

*[r.info](r.info.md), [r.null](r.null.md), [r.support](r.support.md)*

Compression algorithms: [bzip2](http://www.bzip.org/),
[LZ4](https://lz4.org/), [zlib](https://zlib.net/),
[zstd](https://facebook.github.io/zstd)

## AUTHORS

James Westervelt and Michael Shapiro, U.S. Army Construction Engineering
Research Laboratory

Markus Metz
