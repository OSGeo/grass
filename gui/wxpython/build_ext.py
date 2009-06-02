# Build wxGUI extensions (vdigit and nviz)

import os
import sys

def update_opts(line, macros, inc_dirs, lib_dirs, libs, extras):
    """!Update Extension options"""
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
