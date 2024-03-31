#!/usr/bin/env bash

# Print versions, esp. versions of dependencies.

# fail on non-zero return code from a subprocess
set -e

grass --version
grass --tmp-location XY --exec g.version -e
# Detailed Python version info (in one line thanks to echo)
grass --tmp-location XY --exec bash -c "echo Python: \$(\$GRASS_PYTHON -c 'import sys; print(sys.version)')"
python3 --version
python --version
