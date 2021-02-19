# -*- coding: utf-8 -*-
"""
Created on Thu May 28 17:41:32 2015

@author: pietro
"""
from __future__ import print_function
import os
import sys


def get_env():
    """Parse the GISRC file and return the GRASS variales"""
    gisrc = os.environ.get('GISRC')
    if gisrc is None:
        raise RuntimeError('You are not in a GRASS session, GISRC not found.')
    with open(gisrc, mode='r') as grc:
        env = dict([(k.strip(), v.strip())
                    for k, v in [row.split(':',1) for row in grc if row]])
    return env


def get_debug_level():
    """Return the debug level"""
    debug = get_env().get('DEBUG')
    return int(debug) if debug else 0


def G_debug(level, *msg):
    """Print or write a debug message, this is a pure python implementation
    of the G_debug function in the C API."""
    debug_level = get_debug_level()
    if debug_level >= level:
        dfile = os.environ.get("GRASS_DEBUG_FILE")
        fd = sys.stderr if dfile is None else open(dfile, mode='a')
        print("D%d/%d: " % (level, debug_level), *msg, end='\n', file=fd)
