#!/usr/bin/env bash

set -eu

this_script=$(basename "$0")
version_file=include/VERSION
version_git_file=include/VERSION_GIT

if [ ! -f "$version_file" ]; then
    echo "Error: execute ${this_script} from repository top level."
    exit 1
fi

if ! (git log -1 >/dev/null); then
    echo "Error: git not available."
    exit 1
fi

grass_version_git=$(git rev-parse --short HEAD)
gitdate_utc_local=$(TZ=UTC0 git log -1 --date=iso-local --pretty=format:"%cd" -- include)
grass_headers_git_date=$(echo "$gitdate_utc_local" \
    | sed 's/ /T/' | sed 's/ //' | sed 's/\(..\)$/:\1/')

cat << EOF > "$version_git_file"
$grass_version_git
$grass_headers_git_date
EOF
