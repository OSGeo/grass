# Build GRASS with Anaconda

This is a script package for automated build of GRASS as a macOS
application bundle (GRASS-x.x.app).

The building script `build_grass_app.bash` will do all the steps – creating App
bundle, installing Conda dependencies (using the package manager Miniforge),
to patching, compiling and installing GRASS – to end up with an
installed GRASS.app in `/Applications`. It can also create a compressed dmg
file if so wished.

Usage:

```text
./build_grass_app.bash [arguments]

Arguments:
  -s
  --sdk         [path]  MacOS SDK - full path, spaces in path not allowed.
  -t
  --target    [target]  Set deployment target version (MACOSX_DEPLOYMENT_TARGET),
                        e.g. "10.14", optional, default is set from SDK.
  -o
  --dmg-out-dir [path]  Output directory path for DMG file creation
                        This is a requirement for creating .dmg files.
  -c
  --conda-file  [path]  Conda package requirement file, optional.
  --with-liblas         Include libLAS support, optional, default is no support.
  -u
  --update-conda-stable Update the stable explicit conda requirement file. This
                        is only allowed if conda-requirements-dev-[arm64|x86_64].txt
                        is used (with --conda-file), to keep the two files in sync.
  -r
  --repackage           Recreate dmg file from previously built app,
                        setting [-o | --dmg-out-dir] is a requirement.
  --notarize            Code sign and notarize app and dmg for distribution
                        Setting Apple developer ID, keychain profile, and
                        provisionprofile is a prerequisite.
  -h
  --help                Usage information.

```

## Requirements

- Apple's Command Line Tools

You need to install Apple's Command Line Tools (CLT), with or without Xcode.
Installing CLT is possible with following terminal command:

```sh
xcode-select --install
```

Xcode is available for download at Apple's App Store.

CLT will typically install SDKs in `/Library/Developer/CommandLineTools/SDKs/`,
while finding Xcode's default SDK can be achieved with e.g.
`xcrun --show-sdk-path`. (See `man xcrun` for more functions.)

**Note**: Compiling GRASS (C/C++ based) addon extensions with the resulting
GRASS.app requires CLT installation too.

## Instructions

```sh
cd [grass-source-dir]

./macos/build_grass_app.bash
```

There is one required variable: full path to MacOS SDK. By default
this is attempted to be set by `xcrun --show-sdk-path`. Alternatively,
it can be set either through editing the
`$HOME/.config/grass/configure-build-[arm64|x86_64].sh` file,
or by giving it as argument to the main script: `./build_grass_app.bash`.

```sh
./macos/build_grass_app.bash \
  --sdk /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk
```

Argument given to `./build_grass_app.bash` will override settings in
`configure-build-[arm64|x86_64].sh`. You can also do
`./build_grass_app.bash --help` for info on possible configurations.

Required settings:

- SDK full path to the SDK that will be set to -isysroot (path may **not**
  contain spaces)

Example by executing with arguments:

```sh
./macos/build_grass_app.bash \
  --sdk /Library/Developer/CommandLineTools/SDKs/MacOSX10.14.sdk \
  --target 10.14
```

Example of building and creating dmg by executing with arguments:

```sh
./macos/build_grass_app.bash \
  --sdk /Library/Developer/CommandLineTools/SDKs/MacOSX10.14.sdk \
  --conda-file ./Desktop/requirement.txt \
  --dmg-out-dir ~/Desktop
```

## Build Target Architecture

Building GRASS on a x86_64 (Intel) machine can create a binary *only* for the
x86_64 architecture. On an Apple silicon based machine, a binary can be created
for *either* x86_64 or arm64 (creating Universal Binary is at the moment *not*
possible).

The building target architecture depends ultimately on the result of `uname -m`
in the Terminal running the `build_grass_app.bash` script. Building on Apple silicon
machines by opening the Terminal in Rosetta mode, creates a x86_64 binary.

## Settings

By default a conda environment will be created by an explicit conda requirement
file (`files/conda-requirements-stable-[arm64|x86_64].txt`). It was created by
executing `conda list --explicit` on an environment created by the file
`files/conda-requirements-dev-[arm64|x86_64].txt`. This enables reproducibility
and stability. It is also possible to use a customized conda requirement file,
set as an argument (or in `configure-build-[arm64|x86_64].sh`).

To be able to bump dependency versions and/or add/remove dependencies for the
`files/conda-requirements-stable-[arm64|x86_64].txt` file the command flag
`--update-conda-stable` can be added. A requirement for this is that
`files/conda-requirements-dev-[arm64|x86_64].txt` is used for `--conda-file`.
This function is primarily intended to be used for updating this git repo.

```sh
./macos/build_grass_app.bash --update-conda-stable
```

GRASS build configure settings can be set in configure files located in
`$HOME/.config/grass` (or `$XDG_CONFIG_HOME/grass` if set), e.g.:

```sh
mkdir -p $HOME/.config/grass
cp macos/files/configure-build.sh.in $HOME/.config/grass/configure-build-arm64.sh
cp macos/files/configure-build.sh.in $HOME/.config/grass/configure-build-x86_64.sh
```

Edit the configure file(s) to your needs.
