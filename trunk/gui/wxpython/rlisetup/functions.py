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


def convertFeature(vect, outrast, cat, origrast, layer='1', overwrite=False):
    """Convert a single feature to a raster"""
    tmp_vect = "tmp_{rast}".format(rast=outrast)
    grass.run_command('v.extract', input=vect, cats=cat, type='area',
                      layer=layer, output=tmp_vect, flags='d',
                      overwrite=overwrite, quiet=True)
    grass.run_command('g.region', raster=origrast)
    grass.run_command('g.region', vector=tmp_vect)
    grass.run_command('g.region', align=origrast)
    grass.run_command('v.to.rast', input=tmp_vect, type='area',
                      layer=layer, use='val', value=cat, output=outrast,
                      overwrite=overwrite, quiet=True)
    grass.run_command('g.remove', flags='f', type='vector',
                      name=tmp_vect, quiet=True)


def obtainCategories(vector, layer='1'):
    """This function returns a list of categories for all areas in
    the given layer"""
    vect_cats = []
    vc = grass.read_command('v.category', input=vector, layer=layer,
                            option='print', type='centroid')
    for lc in vc.splitlines():
        for cat in lc.split('/'):
            vect_cats.append(int(cat))

    return sorted(set(vect_cats))


def obtainAreaVector(outrast):
    """Create the string for configuration file"""
    reg = grass.region()
    return "MASKEDOVERLAYAREA {name}|{n}|{s}|{e}|{w}\n".format(name=outrast,
                                                               n=reg['n'],
                                                               s=reg['s'],
                                                               e=reg['e'],
                                                               w=reg['w'])


def sampleAreaVector(vect, rast, vect_cats, layer='1', overwrite=False,
                     progDialog=None):
    """Create the strings to add to the configuration file using vector"""
    areanum = len(vect_cats)
    output = []
    # TODO if areanum == 0 exit from the program
    if areanum == 0:
        GError(message=_("The polygon seems to have 0 areas"))
        return None
    for n in range(areanum):
        cat = str(vect_cats[n])
        outpref = "{rast}_{vect}_".format(vect=vect.split('@')[0],
                                          rast=rast.split('@')[0])
        rast_name = "{pref}{cat}".format(pref=outpref, cat=cat)
        # check if raster already axist

        if len(grass.list_strings('raster', pattern=rast_name,
                                  mapset='.')) == 1 and not overwrite:
            GError(message=_("The raster map <%s> already exists."
                             " Please remove or rename the maps "
                             "with the prefix '%s' or select the "
                             "option to overwrite existing maps"
                             % (rast_name, outpref)))
            return None
        convertFeature(vect, rast_name, cat, rast, layer, overwrite)
        output.append(obtainAreaVector(rast_name))
        if progDialog:
            progDialog.Update(n)
    return output
