"""
Created on Sun Jun 23 19:58:54 2013

@author: pietro
"""

from pathlib import Path


def raw_figure(figpath):
    return Path(figpath).read_bytes()
