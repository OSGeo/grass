# How to use Nix

## What is Nix

[Nix](https://nixos.org/) is a powerful package manager and system configuration
tool that aims to make software deployment fully reproducible.

## Nix installation

- Install Nix
  [(learn more about this installer)](https://zero-to-nix.com/start/install)

  ```bash
  curl --proto '=https' --tlsv1.2 -sSf \
     -L https://install.determinate.systems/nix \
     | sh -s -- install
  ```

## Create GRASS GIS development environment

Nix provides a development environment containing all required dependencies.

- Launch development environment

  ```bash
  nix develop
  ```

- Optionally, use [direnv](https://direnv.net) to activate environment
  automatically when entering the source code directory

  ```bash
  echo "use flake" > .envrc
  direnv allow
  ```

## Launch GRASS GIS directly from the source code

Nix allows to run a program directly from git source code repository using
following command:

```bash
nix run \
  github:<OWNER>/<REPO>/<REVISION|BRANCH|TAG>#<PACKAGE-NAME> -- <PROGRAM-ARGUMENTS>
```

- Launch latest version of GRASS from `main` branch

  ```bash
  nix run github:OSGeo/grass#grass
  ```

- Launch GRASS from specific Git revision, branch or tag

  ```bash
  nix run github:OSGeo/grass/<REVISION|BRANCH|TAG>#grass
  ```

- Launch GRASS from pull request

  ```bash
  nix run github:<PR-OWNER>/grass/<PR-BRANCH>#grass
  ```

## Install GRASS GIS directly from the source code

To install a program permanently, use following command:

```bash
nix profile install \
  github:<OWNER>/<REPO>/<REVISION|TAG|BRANCH>#<PACKAGE-NAME> -- <PROGRAM-ARGUMENTS>
```

- Install latest version of GRASS from `main` branch

  ```bash
  nix profile install github:OSGeo/grass#grass
  ```

- Install GRASS from specific Git revision, branch or tag

  ```bash
  nix profile install github:OSGeo/grass/<REVISION|BRANCH|TAG>#grass
  ```

## Uninstall GRASS GIS

- List installed programs

  ```bash
  nix profile list
  ```

- Uninstall a program

  ```bash
  nix profile remove <INDEX-NUMBER>
  ```

## Nix documentation

- [nix.dev](https://nix.dev)
- [Zero to Nix](https://zero-to-nix.com)
