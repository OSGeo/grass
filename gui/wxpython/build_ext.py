# Build wxGUI extensions (vdigit and nviz)

import os
import sys

def __read_variables(file, dict={}):
    """!Read variables from file (e.g. Platform.make)
    
    @param file file descriptor
    @param dict dictionary to store (variable, value)
    """
    for line in file.readlines():
        if len(line) < 1:
            continue # skip empty lines
        if line[0] == '#':
            continue # skip comments
        try:
            var, val = line.split('=', 1)
        except ValueError:
            continue
        
        dict[var.strip()] = val.strip()
        
def update_opts(flag, macros, inc_dirs, lib_dirs, libs, extras):
    """!Update Extension options"""
    global variables
    line = variables[flag]
    fw_next = False
    for val in line.split(' '):
        key = val[:2]
        if fw_next:
            extras.append(val)
            fw_next = False
        elif key == '-I': # includes
            inc_dirs.append(val[2:])
        elif key == '-D': # macros
            if '=' in val[2:]:
                macros.append(tuple(val[2:].split('=')))
            else:
                macros.append((val[2:], None))
        elif key == '-L': # libs dir
            lib_dirs.append(val[2:])
        elif key == '-l':
            libs.append(val[2:])
        elif key == '-F': # frameworks dir
            extras.append(val)
        elif val == '-framework':
            extras.append(val)
            fw_next = True

try:
    Platform_make = open(os.path.join('..', '..', '..',
                                      'include', 'Make', 'Platform.make'))
    Grass_make = open(os.path.join('..', '..', '..',
                                   'include', 'Make', 'Grass.make'))
except IOError, e:
    print 'Unable to compile wxGUI vdigit extension.\n\n', e
    sys.exit(1)

variables = {}
__read_variables(Platform_make, variables)
__read_variables(Grass_make, variables)
