#!/usr/bin/env bash

# Print versions, esp. versions of dependencies.

# fail on non-zero return code from a subprocess
set -e

grass --version
grass --tmp-location XY --exec g.version
python --version
