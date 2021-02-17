# -*- coding: utf-8 -*-
"""
Created on Sun Jun 23 19:58:54 2013

@author: pietro
"""
import io


def raw_figure(figpath):
    with io.OpenWrapper(figpath, mode='rb') as data:
        res = data.read()
    return res
