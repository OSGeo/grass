{ lib
, stdenv
, makeWrapper
, wrapGAppsHook

, withOpenGL ? true

, bison
, blas
, cairo
, ffmpeg
, fftw
, flex
, freetype
, gdal
, geos
, lapack
, libGLU
, libiconv
, libpng
, libsvm
, libtiff
, libxml2
, netcdf
, pdal
, pkg-config
, postgresql
, proj
, python312Packages
, readline
, sqlite
, wxGTK32
, zlib
, zstd
}:


let
  pyPackages = python312Packages;

in
stdenv.mkDerivation (finalAttrs: {
  pname = "grass";
  version = "dev";

  src = lib.cleanSourceWith {
    src = ./.;
    filter = (
      path: type: (builtins.all (x: x != baseNameOf path) [
        ".git"
        ".github"
        "flake.nix"
        "package.nix"
      ])
    );
  };

  nativeBuildInputs = [
    makeWrapper
    wrapGAppsHook

    bison
    flex
    gdal # for `gdal-config`
    geos # for `geos-config`
    netcdf # for `nc-config`
    pkg-config
  ] ++ (with pyPackages; [ python-dateutil numpy wxpython ]);

  buildInputs = [
    blas
    cairo
    ffmpeg
    fftw
    freetype
    gdal
    geos
    lapack
    libpng
    libsvm
    libtiff
    (libxml2.override { enableHttp = true; })
    netcdf
    pdal
    postgresql
    proj
    readline
    sqlite
    wxGTK32
    zlib
    zstd
  ] ++ lib.optionals withOpenGL [ libGLU ]
  ++ lib.optionals stdenv.isDarwin [ libiconv ];

  strictDeps = true;

  configureFlags = [
    "--with-blas"
    "--with-cairo-ldflags=-lfontconfig"
    "--with-cxx"
    "--with-fftw"
    "--with-freetype"
    "--with-gdal"
    "--with-geos"
    "--with-lapack"
    "--with-libsvm"
    "--with-nls"
    "--with-openmp"
    "--with-pdal"
    "--with-postgres"
    "--with-postgres-libs=${postgresql.lib}/lib/"
    "--with-proj-includes=${proj.dev}/include"
    "--with-proj-libs=${proj}/lib"
    "--with-proj-share=${proj}/share/proj"
    "--with-sqlite"
    "--with-zstd"
    "--without-bzlib"
    "--without-mysql"
    "--without-odbc"
  ] ++ lib.optionals (! withOpenGL) [
    "--without-opengl"
  ] ++ lib.optionals stdenv.isDarwin [
    "--without-cairo"
    "--without-freetype"
    "--without-x"
  ];

  # Otherwise a very confusing "Can't load GDAL library" error
  makeFlags = lib.optional stdenv.isDarwin "GDAL_DYNAMIC=";

  # Ensures that the python scripts running at build time are actually
  # executable; otherwise, patchShebangs ignores them.
  postConfigure = ''
    for f in $(find . -name '*.py'); do
      chmod +x $f
    done

    patchShebangs */
  '';

  postInstall = ''
    wrapProgram $out/bin/grass \
    --set PYTHONPATH $PYTHONPATH \
    --set GRASS_PYTHON ${pyPackages.python.interpreter} \
    --suffix LD_LIBRARY_PATH ':' '${gdal}/lib'
    ln -s $out/grass*/lib $out/lib
    ln -s $out/grass*/include $out/include
  '';

  enableParallelBuilding = true;

  meta = with lib; {
    description = "GIS software suite used for geospatial data management and analysis, image processing, graphics and maps production, spatial modeling, and visualization";
    homepage = "https://grass.osgeo.org/";
    license = licenses.gpl2Plus;
    maintainers = with maintainers; teams.geospatial.members;
    platforms = platforms.all;
    mainProgram = "grass";
  };
})
