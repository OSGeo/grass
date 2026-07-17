# Building GRASS with CMake

This guide covers developer workflows for building GRASS with CMake:
configuring and compiling, running GRASS directly from the build
directory, recompiling only modified code, and compiling addons. For
dependencies and general installation instructions, see
[INSTALL.md](../../INSTALL.md). If you are coming from the Autotools
build, see the [comparison table](#comparison-with-autotools) at the
end.

## Configuring

Create and configure a build directory with:

```bash
cmake -B build
```

The build is fully out-of-source: all generated files go into the build
directory (here `build`), the source tree stays clean, and you can keep
several independently configured build directories side by side
(e.g. `build-debug`, `build-clang`).

Optional dependencies are controlled with `WITH_*` options:

```bash
cmake -B build -DWITH_MYSQL=ON -DWITH_FFTW=OFF
```

See the `option(WITH_...)` calls in the top-level `CMakeLists.txt` for
the full list. Note that options enabled by default (PDAL, GEOS, FFTW,
etc.) are treated as required: on a system missing one, configure fails
with a "Could not find ..." error. Either install the dependency or
disable the option explicitly.

Other frequently used settings:

```bash
# Custom compiler flags
cmake -B build -DCMAKE_C_FLAGS="-g -Wall"

# Optimization/debug presets (Debug, Release, RelWithDebInfo)
cmake -B build -DCMAKE_BUILD_TYPE=Debug

# Installation directory (default /usr/local)
cmake -B build -DCMAKE_INSTALL_PREFIX=/opt/grass
```

The install prefix is recorded in the build at configure time, so set it
with `-DCMAKE_INSTALL_PREFIX` here rather than at install time.

Settings are sticky: they are cached in the build directory, so a later
plain `cmake -B build` or rebuild keeps using them. To change a setting,
pass the new `-D...` value; to reset everything, delete the build
directory.

If `ccache` is installed, the build uses it automatically to cache
compiler output, so recompiling files that have not changed (e.g. after
deleting the build directory or in a second build directory) is nearly
instant. Pass `-DUSE_CCACHE=OFF` to opt out.

## Compiling

```bash
cmake --build build -j$(nproc)
```

`cmake --build` drives `make` under the hood; it compiles serially
unless you pass `-j`.

There is no separate reconfigure step after pulling changes: the build
re-runs CMake automatically when any `CMakeLists.txt` changed.

## Running GRASS from the build directory

The runnable tree is staged under `build/output`, so GRASS can be run
without installing:

```bash
./build/output/bin/grass
```

For running tests against the build tree, put `build/output/bin` on
`PATH`:

```bash
export PATH="$(pwd)/build/output/bin:${PATH}"
```

## Recompiling only what you modified

Every tool is a CMake target named exactly like the tool, and building a
target also rebuilds any libraries it depends on and re-links the result
into `build/output`. So after editing, e.g., `raster/r.slope.aspect/main.c`
or a library it uses:

```bash
cmake --build build --target r.slope.aspect
```

and the updated tool is immediately runnable from `build/output`.
Targets are always built this way from the top level; there is no
per-directory build.

Other useful target names:

- C libraries: `grass_<name>` (e.g. `grass_gis`, `grass_raster`,
  `grass_vector`)
- Python packages, staged into `build/output`: `python_<name>`
  (e.g. `python_script`, `python_tools`, `python_temporal`,
  `python_pygrass`); an edited file under `python/grass/` is not visible
  in `build/output` until its target is rebuilt
- Python script tools: the tool name, same as C tools (e.g. `r.mask`)

To list all available targets (with the default Makefile generator):

```bash
cmake --build build --target help
```

## Compiling addons

Addons are installed with *g.extension*, which downloads the addon
source code, compiles it, and installs it into your user addon
directory. With a CMake-built GRASS, *g.extension* compiles the addon
using CMake automatically; you do not run any CMake commands yourself.

The requirement is that the GRASS you run *g.extension* from is an
installed one (`cmake --install build`), not the `build/output` tree,
which lacks the addon build machinery. No root is needed: install to a
prefix in your home directory. In the GRASS source directory:

```bash
cmake -B build -DCMAKE_INSTALL_PREFIX=$HOME/grass-install
cmake --build build -j$(nproc)
cmake --install build
```

Then run *g.extension* from the installed GRASS as usual, in a GRASS
session or on the command line with `--exec`:

```bash
$HOME/grass-install/bin/grass --tmp-project XY --exec \
    g.extension extension=r.example
```

## Comparison with Autotools

For developers used to the Autotools build (`./configure && make`):

| Task | Autotools | CMake |
| ---- | --------- | ----- |
| Configure | `./configure [options]` | `cmake -B build [options]` |
| Configure with debug flags | `CFLAGS="-g -Wall" ./configure` | `cmake -B build -DCMAKE_C_FLAGS="-g -Wall"` |
| Set install directory | `./configure --prefix=...` | `cmake -B build -DCMAKE_INSTALL_PREFIX=...` |
| Compile everything | `make -j$(nproc)` | `cmake --build build -j$(nproc)` |
| Install | `make install` | `cmake --install build` |
| Run without installing | `bin.$ARCH/grass` | `build/output/bin/grass` |
| Recompile one tool | `cd raster/r.slope.aspect && make` | `cmake --build build --target r.slope.aspect` |
| Clean compiled files | `make clean` | `cmake --build build --target clean` |
| Clean everything incl. configuration | `make distclean` | `rm -rf build` |
| Use ccache | `CC="ccache gcc" ./configure` | automatic if installed |
