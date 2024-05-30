"""
Created on Sun Jun 23 19:58:54 2013

@author: pietro
"""


def raw_figure(figpath):
    with open(figpath, mode="rb") as data:
        res = data.read()
    return res
