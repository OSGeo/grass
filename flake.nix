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
          inputsFrom = [ self'.packages.grass ];

          # additional packages
          buildInputs = with pkgs.python3Packages; [
            pytest
          ];

          shellHook = ''
            function dev-help {
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
              echo
              echo "Run 'dev-help' to see this message again."
            }

            dev-help
          '';
        };
      };

      flake = { };
    };
}
