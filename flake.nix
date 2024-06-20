{
  description = "GRASS GIS";

  nixConfig = {
    bash-prompt = "\\[\\033[1m\\][grass-dev]\\[\\033\[m\\]\\040\\w >\\040";
  };

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
  };

  outputs = inputs@{ flake-parts, ... }:
    flake-parts.lib.mkFlake { inherit inputs; } {

      systems = [ "x86_64-linux" ];

      perSystem = { config, self', inputs', pkgs, system, ... }: {

        packages.grass = pkgs.callPackage ./package.nix { };

        devShells.default = pkgs.mkShell {
          buildInputs =
            # from package nativeBuildInputs
            with pkgs; [
              bison
              flex
              gdal # for `gdal-config`
              geos # for `geos-config`
              netcdf # for `nc-config`
              pkg-config
            ] ++ (with python3Packages; [ pytest python-dateutil numpy wxPython_4_2 ])

            # from package buildInputs
            ++ [
              blas
              cairo
              ffmpeg
              fftw
              freetype
              gdal
              geos
              lapack
              libGLU
              libpng
              libsvm
              libtiff
              libxml2
              netcdf
              pdal
              postgresql
              proj
              readline
              sqlite
              wxGTK32
              zlib
              zstd
            ] ++ lib.optionals stdenv.isDarwin [ libiconv ];

          shellHook = ''
            echo -e "\nWelcome to a GRASS development environment !"
            echo "Build GRASS using following commands:"
            echo
            echo " 1.  ./configure --prefix=\$(pwd)/app"
            echo " 2.  make -j$(nproc)"
            echo " 3.  make install"
            echo
            echo "Run tests:"
            echo
            echo " 1.  export PYTHONPATH=\$(app/bin/grass --config python_path):\$PYTHONPATH"
            echo " 2.  export LD_LIBRARY_PATH=\$(app/bin/grass --config path)/lib:\$LD_LIBRARY_PATH"
            echo " 3.  pytest"
            echo
            echo "Note: run 'nix flake update' from time to time to update dependencies."
          '';
        };
      };

      flake = { };
    };
}
