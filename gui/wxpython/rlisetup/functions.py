# -*- coding: utf-8 -*-
"""
Created on Mon Nov 26 11:48:03 2012

@author: lucadelu
"""
import wx
import os
import sys
from grass.script import core as grass
from core.gcmd import GError


class SamplingType:
    """"
    KMVWINC = samplingtype=moving, regionbox=keyboard, shape=circle
    KMVWINR = samplingtype moving, regionbox=keyboard, shape=rectangle
    MMVWINC = samplingtype=moving, regionbox=mouse, shape=circle
    MMVWINR = samplingtype moving, regionbox=mouse, shape=rectangle

    KUNITSC = samplingtype=units, regionbox=keyboard, shape=cirlce
    KUNITSR = samplingtype=units, regionbox=keyboard, shape=rectangle
    MUNITSC = samplingtype=units, regionbox=mouse, shape=cirlce
    MUNITSR = samplingtype=units, regionbox=mouse, shape=rectangle
    """

    WHOLE = 'whole'
    REGIONS = 'regions'
    UNITS = 'units'
    VECT = 'vector'
    MVWIN = 'moving'

    KMVWINC = 'kmvwin_circle'
    KMVWINR = 'kmvwin_rectangle'
    MMVWINC = 'mmvwin_circle'
    MMVWINR = 'mmvwin_rectangle'

    KUNITSC = 'kunits_circle'
    KUNITSR = 'kunits_rectangle'
    MUNITSC = 'munits_circle'
    MUNITSR = 'munits_rectangle'


def checkValue(value):
    if value == '':
        wx.FindWindowById(wx.ID_FORWARD).Enable(False)
    else:
        wx.FindWindowById(wx.ID_FORWARD).Enable(True)


def retRLiPath():
    """Return the directory of configuration files for r.li"""
    if sys.platform == 'win32':
        grass_config_dirname = "GRASS7"
        grass_config_dir = os.path.join(os.getenv('APPDATA'),
                                        grass_config_dirname)
    else:
        grass_config_dirname = ".grass7"
        grass_config_dir = os.path.join(os.getenv('HOME'),
                                        grass_config_dirname)

    rlipath = os.path.join(grass_config_dir, 'r.li')
    if os.path.exists(rlipath):
        return rlipath
    else:
        os.mkdir(rlipath)
        return rlipath


def checkMapExists(name, typ='raster'):
    """Check if a map already exist in the working mapset"""
    env = grass.gisenv()
    mapset = env['MAPSET']
    mapp = grass.find_file(name, typ, mapset)
    if mapp.name != '':
        return True
    else:
        return False


def convertFeature(vect, outrast, cat, origrast):
    """Convert a single feature to a raster"""
    tmp_vect = "tmp_{rast}".format(rast=outrast)
    grass.run_command('v.extract', input=vect, output=tmp_vect, cats=cat,
                      overwrite=True, quiet=True)
    grass.run_command('g.region', vector=tmp_vect, align=origrast)
    grass.run_command('v.to.rast', input=vect, output=outrast, use='cat',
                      cats=cat, overwrite=True, quiet=True)
    grass.run_command('g.remove', flags='f', type='vector',
                      name=tmp_vect, quiet=True)

def obtainAreaVector(outrast):
    """Create the string for configuration file"""
    reg = grass.region()
    return "MASKEDOVERLAYAREA {name}|{n}|{s}|{e}|{w}\n".format(name=outrast,
                                                             n=reg['n'],
                                                             s=reg['s'],
                                                             e=reg['e'],
                                                             w=reg['w'])


def sampleAreaVector(vect, rast, vect_cats, progDialog=None):
    """Create the strings to add to the configuration file using vector"""
    areanum = len(vect_cats)
    output = []
    #TODO if areanum == 0 exit from the program
    if areanum == 0:
        GError(message=_("The polygon seems to have 0 areas"))
        return None
    for n in range(areanum):
        cat = vect_cats[n]
        rast_name = "{name}_{cat}".format(name=vect.split('@')[0], cat=cat)
        convertFeature(vect, rast_name, cat, rast)
        output.append(obtainAreaVector(rast_name))
        if progDialog:
            progDialog.Update(n)
    return output
