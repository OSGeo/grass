# Test suite

Tests are in directories `tests` and `testsuite` under each directory which has
tests. This directory contains additional scripts and information to test
functionality without a focus on a specific part of the code. Currently, that
is `raster_md5test.sh`, a shell test which runs in a GRASS session and is
collected by _grass.gunittest_ like any other test in a `testsuite` directory.

See the [testing documentation](../doc/development/testing.md) for how GRASS
tests are written and run.

## CI

Most tests run in the CI. See the `.github` directory for details and
use it as a reference if needed.
