#!/bin/sh
set -eu

###############################################################################
# Format source code according to GRASS GIS submitting rules
#
# Dependencies:
#    clang-format
#      (most easily available with e.g.:
#      `python -m pip install 'clang-format==15.0.6’`)
#
# Author(s):
#    Nicklas Larsson
#
# Usage:
#    If you have "clang-format" in PATH, execute for complete source formatting:
#      ./utils/grass_clang_format.sh
#
#    Setting 'GRASS_CLANG_FORMAT' to explicitly set clang-format version:
#      GRASS_CLANG_FORMAT="clang-format-15" ./utils/grass_clang_format.sh
#
#    It is also possible to format the content in a (one) given directory:
#      ./utils/grass_clang_format.sh ./lib/raster
#
# COPYRIGHT: (C) 2023-2024 by the GRASS Development Team
#
#            This program is free software under the GNU General Public
#            License (>=v2). Read the file COPYING that comes with GRASS
#            for details.
#
###############################################################################

# Check if variable is integer (POSIX compliant)
# based on https://unix.stackexchange.com/a/598047
is_integer ()
{
    case "${1#[+-]}" in
        (*[!0123456789]*) echo 1 ;;
        ('')              echo 1 ;;
        (*)               echo 0 ;;
    esac
}

# Required clang-format version
req_cf_v="18"

# No need to continue if the .clang-format file isn't found
if [ ! -f .clang-format ]; then
    echo "Error: could not find the .clang-format file. Is the GRASS source"
    echo  "  top directory your working directory?"
    exit 1
fi

# If set, use env variable GRASS_CLANG_FORMAT for clang-format
if [ -z ${GRASS_CLANG_FORMAT+x} ]; then
    fmt="clang-format"
else
    fmt="${GRASS_CLANG_FORMAT}"
fi

if ! (${fmt} --version >/dev/null); then
    echo "clang-format not available."
    exit 1
fi

# Try extract ClangFormat version from pre-commit config file
pre_commit_config_file=".pre-commit-config.yaml"
pre_commit_version=$(grep -A 1 --regex "repo:.*clang-format" \
    "${pre_commit_config_file}" | sed -En 's/.*rev: v([0-9]+)\..*/\1/p')
if [ "$(is_integer "$pre_commit_version")" -eq "1" ]; then
    echo "Warning: failed to retrieve ClangFormat version number from"
    echo "  ${pre_commit_config_file}. Falling back to version {$req_cf_v}."
else
    req_cf_v="${pre_commit_version}"
fi

clang_version_full=$(${fmt} --version)
clang_version=$(echo "${clang_version_full}" | \
    sed -En 's/.*version ([0-9]+)\.[0-9]+\.[0-9]+.*/\1/p')
if [ "${clang_version}" -ne "${req_cf_v}" ]; then
    echo "Error: ${clang_version_full}"
    echo "  is used, but version ${req_cf_v} is required."
    echo "  Consider setting the global variable GRASS_CLANG_FORMAT to"
    echo "  the clang-format version needed."
    exit 1
fi

# One argument, a directory path is allowed
if [ "$#" -eq 1 ] && [ -d "$1" ]; then
    dir="$1"
else
    dir="."
fi

find "${dir}" -type f \
    '(' -iname '*.c' -o -iname '*.cpp' -o -iname '*.h' -o -iname '*.hpp' ')' \
        -exec "${fmt}" -i '{}' +
