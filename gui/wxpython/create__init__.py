#!/usr/bin/env python

import os
import sys
import glob

def main(path):
    if not os.path.exists(path) or not os.path.isdir(path):
        print >> sys.stderr, "'%s' is not a directory" % path
        return 1
    
    modules = []
    for f in glob.glob(os.path.join(os.path.basename(path), '*.py')):
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
        sys.exit("usage: %s path/to/gui_modules" % sys.argv[0])
    
    sys.exit(main(sys.argv[1]))
