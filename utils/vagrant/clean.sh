#!/bin/sh

# abort install if any errors occur and enable tracing
set -o errexit
set -o xtrace

cd /vagrant

if [ -f "include/Make/Platform.make" ] ; then
    make cleandistdirs
    make distclean
fi

exit 0
