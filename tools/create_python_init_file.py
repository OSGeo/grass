#!/usr/bin/env python
"""
Script for creating an __init__.py file for a package.

Adds all modules (*.py files) in a directory into an __init__.py file.
Overwrites existing __init__.py file in a directory.

(C) 2011-2013 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com>
@author Vaclav Petras <wenzeslaus gmail.com>
"""


import os
import sys
import glob


def main(path):
    if not os.path.exists(path) or not os.path.isdir(path):
        print >> sys.stderr, "'%s' is not a directory" % path
        return 1

    modules = []
    pattern = os.path.join(path, '*.py')
    for f in glob.glob(pattern):
        if f[-5:-3] == '__':
            continue
        modules.append(os.path.splitext(os.path.basename(f))[0])

    fd = open(os.path.join(path, '__init__.py'), 'w')
    try:
        fd.write('all = [%s' % os.linesep)
        for m in modules:
            fd.write("    '%s',%s" % (m, os.linesep))
        fd.write('    ]%s' % os.linesep)
    finally:
        fd.close()

    return 0

if __name__ == "__main__":
    if len(sys.argv) < 2:
        sys.exit("usage: %s path/to/package/directory" % sys.argv[0])

    sys.exit(main(sys.argv[1]))
