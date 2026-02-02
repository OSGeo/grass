#!/usr/bin/env bash

# Print versions, esp. versions of dependencies.

python --version
python3 --version

# We use Git during build.
git --version

# This will fail if the build failed.
command -v grass >/dev/null 2>&1 && grass --version || echo "GRASS not installed yet"
grass --tmp-project XY --exec g.version -ergb
# Detailed Python version info (in one line thanks to echo)
grass --tmp-project XY --exec bash -c "echo Python: \$(\$GRASS_PYTHON -c 'import sys; print(sys.version)')"
