"""
Created on Thu May 28 17:41:32 2015

@author: pietro
"""

import os
import sys


def get_env():
    """Parse the GISRC file and return the GRASS variables"""
    gisrc = os.environ.get("GISRC")
    if gisrc is None:
        msg = "You are not in a GRASS session, GISRC not found."
        raise RuntimeError(msg)
    with open(gisrc) as grc:
        return {
            k.strip(): v.strip() for k, v in [row.split(":", 1) for row in grc if row]
        }


def get_debug_level():
    """Return the debug level"""
    debug = get_env().get("DEBUG")
    return int(debug) if debug else 0


def G_debug(level, *msg):
    """Print or write a debug message, this is a pure python implementation
    of the G_debug function in the C API."""
    debug_level = get_debug_level()
    if debug_level >= level:
        dfile = os.environ.get("GRASS_DEBUG_FILE")
        fd = sys.stderr if dfile is None else open(dfile, mode="a")
        print("D%d/%d: " % (level, debug_level), *msg, end="\n", file=fd)
