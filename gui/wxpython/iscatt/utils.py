"""
@package iscatt.utils

@brief Misc utilities for iscatt

(C) 2020 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Nicklas Larsson <n_larsson yahoo com>
"""

import numpy as np

# Code originate from matplotlib
# https://matplotlib.org/3.0.0/_modules/matplotlib/mlab.html#dist
def dist(x, y):
    """
    Return the distance between two points.
    """
    d = x - y
    return np.sqrt(np.dot(d, d))

# Code originate from matplotlib
# https://matplotlib.org/3.0.0/_modules/matplotlib/mlab.html#dist_point_to_segment
def dist_point_to_segment(p, s0, s1):
    """
    Get the distance of a point to a segment.

      *p*, *s0*, *s1* are *xy* sequences

    This algorithm from
    http://geomalgorithms.com/a02-_lines.html
    """
    p = np.asarray(p, float)
    s0 = np.asarray(s0, float)
    s1 = np.asarray(s1, float)
    v = s1 - s0
    w = p - s0

    c1 = np.dot(w, v)
    if c1 <= 0:
        return dist(p, s0)

    c2 = np.dot(v, v)
    if c2 <= c1:
        return dist(p, s1)

    b = c1 / c2
    pb = s0 + b * v
    return dist(p, pb)
