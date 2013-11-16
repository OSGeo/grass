"""!
@package iscatt.iscatt_core

@brief Non GUI functions.

Classes:
 - iscatt_core::Core
 - iscatt_core::CatRastUpdater
 - iscatt_core::AnalyzedData
 - iscatt_core::ScattPlotsCondsData
 - iscatt_core::ScattPlotsData

(C) 2013 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Stepan Turek <stepan.turek seznam.cz> (mentor: Martin Landa)
"""
import os
import sys

import time

import numpy as np

# used iclass perimeters algorithm instead of convolve2d
#from matplotlib.path import Path 
#from scipy.signal import convolve2d

from math import sqrt, ceil, floor
from copy import deepcopy

from core.gcmd import GException, GError, RunCommand

import grass.script as grass

from core_c import CreateCatRast, ComputeScatts, UpdateCatRast, \
                   Rasterize, SC_SCATT_DATA, SC_SCATT_CONDITIONS

MAX_SCATT_SIZE = 4100 * 4100
WARN_SCATT_SIZE = 2000 * 2000
MAX_NCELLS = 65536 * 65536
WARN_NCELLS = 12000 * 12000

class Core:
    """!Represents scatter plot backend.
    """
    def __init__(self):
        
        self.an_data = AnalyzedData()

        self.scatts_dt = ScattPlotsData(self.an_data)
        self.scatt_conds_dt = ScattPlotsCondsData(self.an_data)

        self.cat_rast_updater = CatRastUpdater(self.scatts_dt, self.an_data, self)

    def SetData(self, bands):
        """Set bands for analysis.
        """
        ret = self.an_data.Create(bands)
        if not ret:
            return False

        n_bands = len(self.GetBands())

        self.scatts_dt.Create(n_bands)
        self.scatt_conds_dt.Create(n_bands)

        return True

    def AddCategory(self, cat_id):
        self.scatts_dt.AddCategory(cat_id)
        return self.scatt_conds_dt.AddCategory(cat_id)

    def DeleteCategory(self, cat_id):
        self.scatts_dt.DeleteCategory(cat_id)
        self.scatt_conds_dt.DeleteCategory(cat_id)

    def CleanUp(self):
        self.scatts_dt.CleanUp()
        self.scatt_conds_dt.CleanUp()

    def GetBands(self):
        return self.an_data.GetBands()

    def GetScattsData(self):
        return self.scatts_dt, self.scatt_conds_dt;

    def GetRegion(self):
        return self.an_data.GetRegion()

    def GetCatRast(self, cat_id):
        return self.scatts_dt.GetCatRast(cat_id)

    def AddScattPlots(self, scatt_ids):
    
        for s_id in scatt_ids:
            self.scatts_dt.AddScattPlot(scatt_id = s_id)

        cats_ids = self.scatts_dt.GetCategories()
        self.ComputeCatsScatts(cats_ids)

    def SetEditCatData(self, cat_id, scatt_id, bbox, value):

        if cat_id not in self.scatts_dt.GetCategories():
            raise GException(_("Select category for editing."))

        if self.scatt_conds_dt.AddScattPlot(cat_id, scatt_id) < 0:
            return None

        arr = self.scatt_conds_dt.GetValuesArr(cat_id, scatt_id)

        for k, v in bbox.iteritems():
            bbox[k] = self._validExtend(v)

        arr[bbox['btm_y'] : bbox['up_y'], bbox['btm_x'] : bbox['up_x']] = value

        self.ComputeCatsScatts([cat_id])
        return cat_id

    def ComputeCatsScatts(self, cats_ids):

        requested_dt = {}
        requested_dt_conds = {}

        for c in cats_ids:
            requested_dt_conds[c] = self.scatt_conds_dt.GetCatScatts(c)
            requested_dt[c] = self.scatts_dt.GetCatScatts(c)

        scatt_conds = self.scatt_conds_dt.GetData(requested_dt_conds)
        scatts = self.scatts_dt.GetData(requested_dt)

        bands = self.an_data.GetBands()

        cats_rasts = self.scatts_dt.GetCatsRasts()
        cats_rasts_conds = self.scatts_dt.GetCatsRastsConds()

        returncode, scatts = ComputeScatts(self.an_data.GetRegion(), 
                                           scatt_conds, 
                                           bands,
                                           len(self.GetBands()), 
                                           scatts, 
                                           cats_rasts_conds, 
                                           cats_rasts)

        if returncode < 0:
            GException(_("Computing of scatter plots failed."))

    def CatRastUpdater(self):
        return self.cat_rast_updater
    
    def UpdateCategoryWithPolygons(self, cat_id, scatts_pols, value):
        start_time = time.clock()

        if cat_id not in self.scatts_dt.GetCategories():
            raise GException(_("Select category for editing."))

        for scatt_id, coords in scatts_pols.iteritems():

            if self.scatt_conds_dt.AddScattPlot(cat_id, scatt_id) < 0:
                return False

            b1, b2 = idScattToidBands(scatt_id, len(self.an_data.GetBands()))    
            b = self.scatts_dt.GetBandsInfo(scatt_id)

            region = {}
            region['s'] = b['b2']['min'] - 0.5
            region['n'] = b['b2']['max'] + 0.5

            region['w'] = b['b1']['min'] - 0.5
            region['e'] = b['b1']['max'] + 0.5

            arr = self.scatt_conds_dt.GetValuesArr(cat_id, scatt_id)
            arr = Rasterize(polygon=coords, 
                            rast=arr, 
                            region=region, 
                            value=value)

            # previous way of rasterization / used scipy
            #raster_pol = RasterizePolygon(coords, b['b1']['range'], b['b1']['min'], 
            #                                      b['b2']['range'], b['b2']['min'])

            #raster_ind = np.where(raster_pol > 0) 
            #arr = self.scatt_conds_dt.GetValuesArr(cat_id, scatt_id)

            #arr[raster_ind] = value
            #arr.flush()
        
        self.ComputeCatsScatts([cat_id])
        return cat_id

    def ExportCatRast(self, cat_id, rast_name):

        cat_rast = self.scatts_dt.GetCatRast(cat_id);
        if not cat_rast:
            return 1
        
        return RunCommand("g.copy", 
                          rast=cat_rast + ',' + rast_name,
                          getErrorMsg=True,
                          overwrite=True)

    def _validExtend(self, val):
        #TODO do it general
        if  val > 255:
            val = 255
        elif val < 0:
            val = 0

        return val

class CatRastUpdater:
    """!Update backend data structures according to selected areas in mapwindow.
    """
    def __init__(self, scatts_dt, an_data, core):
        self.scatts_dt = scatts_dt
        self.an_data = an_data # TODO may be confusing
        self.core = core
        self.vectMap = None

    def SetVectMap(self, vectMap):
        self.vectMap = vectMap

    def SyncWithMap(self):
        #TODO possible optimization - bbox only of vertex and its two neighbours

        region = self.an_data.GetRegion()

        bbox = {}
        bbox['maxx'] = region['e']
        bbox['minx'] = region['w']
        bbox['maxy'] = region['n']
        bbox['miny'] = region['s']

        updated_cats = []

        for cat_id in self.scatts_dt.GetCategories():
            if cat_id == 0:
                continue
            
            cat = [{1 : [cat_id]}]
            self._updateCatRast(bbox, cat, updated_cats)

        return updated_cats

    def EditedFeature(self, new_bboxs, new_areas_cats, old_bboxs, old_areas_cats):
        #TODO possible optimization - bbox only of vertex and its two neighbours

        bboxs = old_bboxs + new_bboxs
        areas_cats = old_areas_cats + new_areas_cats 

        updated_cats = []

        for i in range(len(areas_cats)):
            self._updateCatRast(bboxs[i], areas_cats[i], updated_cats)
        
        return updated_cats

    def _updateCatRast(self, bbox, areas_cats, updated_cats):

        rasterized_cats = []
        for c in range(len(areas_cats)):

            if not areas_cats[c]:
                continue

            layer = areas_cats[c].keys()[0]
            cat =  areas_cats[c][layer][0]

            if cat in rasterized_cats:
                continue

            rasterized_cats.append(cat)
            updated_cats.append(cat)

            grass_region = self._create_grass_region_env(bbox)

            #TODO hack check if raster exists?
            patch_rast = "temp_scatt_patch_%d" % (os.getpid())
            self._rasterize(grass_region, layer, cat, patch_rast)

            region = self.an_data.GetRegion()
            ret = UpdateCatRast(patch_rast, region, self.scatts_dt.GetCatRastCond(cat))
            if ret < 0:
                GException(_("Patching category raster conditions file failed."))            
            RunCommand("g.remove",
                      rast = patch_rast)

    def _rasterize(self, grass_region, layer, cat, out_rast):

        #TODO different thread may be problem when user edits map
        environs = os.environ.copy()
        environs['GRASS_VECTOR_TEMPORARY'] = '1'

        ret, text, msg = RunCommand("v.category",
                                    input=self.vectMap,
                                    getErrorMsg = True,
                                    option='report',
                                    read=True,
                                    env=environs)

        ret, text, msg = RunCommand("v.build",
                                    map = self.vectMap,
                                    getErrorMsg = True,
                                    read = True,
                                    env = environs)

        if ret != 0:
            GException(_("v.build failed:\n%s" % msg))

        environs = os.environ.copy()
        environs["GRASS_REGION"] = grass_region["GRASS_REGION"]
        environs['GRASS_VECTOR_TEMPORARY'] = '1'

        ret, text, msg = RunCommand("v.to.rast",
                                    input = self.vectMap,
                                    use = "cat",
                                    layer = str(layer),
                                    cat = str(cat),
                                    output = out_rast,
                                    getErrorMsg = True,
                                    read = True,
                                    overwrite = True,
                                    env = environs)

        if ret != 0:
            GException(_("v.to.rast failed:\n%s" % msg))

    def _create_grass_region_env(self, bbox):

        r = self.an_data.GetRegion()
        new_r = {}

        if bbox["maxy"] <= r["s"]:
            return 0
        elif bbox["maxy"] >= r["n"]:
            new_r["n"] = bbox["maxy"]
        else:
            new_r["n"] = ceil((bbox["maxy"] - r["s"]) / r["nsres"]) * r["nsres"] + r["s"]

        if bbox["miny"] >= r["n"]:
            return 0
        elif bbox["miny"] <= r["s"]:
            new_r["s"] = bbox["miny"]
        else:
            new_r["s"] = floor((bbox["miny"] - r["s"]) / r["nsres"]) * r["nsres"] + r["s"]

        if bbox["maxx"] <= r["w"]:
            return 0
        elif bbox["maxx"] >= r["e"]:
            new_r["e"] = bbox["maxx"]
        else:
            new_r["e"] = ceil((bbox["maxx"] - r["w"]) / r["ewres"]) * r["ewres"] + r["w"]

        if bbox["minx"] >= r["e"]:
            return 0
        elif bbox["minx"] <= r["w"]:
            new_r["w"] = bbox["minx"]
        else:
            new_r["w"] = floor((bbox["minx"] - r["w"]) / r["ewres"]) * r["ewres"] + r["w"]

        #TODO check regions resolution
        new_r["nsres"] = r["nsres"]
        new_r["ewres"] = r["ewres"]

        return {"GRASS_REGION" :  grass.region_env(**new_r)}

class AnalyzedData:
    """!Represents analyzed data (bands, region).
    """
    def __init__(self):
        
        self.bands = []
        self.bands_info = {}

        self.region = None

    def GetRegion(self):
        return self.region

    def Create(self, bands):
        self.bands = bands[:]
        self.region = None

        self.region = GetRegion()
        if self.region["rows"] * self.region["cols"] > MAX_NCELLS:
            GException("too big region")

        self.bands_info = {}

        for b in self.bands[:]:
            i = GetRasterInfo(b)

            if i is None:
                GException("raster %s is not CELL type" % (b))
    
            self.bands_info[b] = i
            #TODO check size of raster

        return True

    def GetBands(self):
        return self.bands

    def GetBandInfo(self, band_id):
        band = self.bands[band_id]
        return self.bands_info[band]

class ScattPlotsCondsData:
    """!Data structure for selected areas in scatter plot(condtions).
    """
    def __init__(self, an_data):

        self.an_data = an_data

        #TODO
        self.max_n_cats = 10
    
        self.dtype = 'uint8'
        self.type = 1;
        self.CleanUp()

    def CleanUp(self):
    
        self.cats = {}

        self.n_scatts = -1
        self.n_bands = -1

        for cat_id in self.cats.keys():
            self.DeleteCategory(cat_id)

    def Create(self, n_bands):

        self.CleanUp()

        self.n_scatts =  (n_bands - 1) * n_bands / 2;
        self.n_bands = n_bands

        self.AddCategory(cat_id = 0)

    def AddCategory(self, cat_id):

        if cat_id not in self.cats.keys():
            self.cats[cat_id] = {}
            return cat_id
        return -1

    def DeleteCategory(self, cat_id):

        if cat_id not in self.cats.keys():
            return False

        for scatt in self.cats[cat_id].itervalues():
            grass.try_remove(scatt['np_vals'])
            del scatt['np_vals']

        del self.cats[cat_id]

        return True

    def GetCategories(self):
        return self.cats.keys()

    def GetCatScatts(self, cat_id):

        if not self.cats.has_key(cat_id):
            return False

        return self.cats[cat_id].keys()


    def AddScattPlot(self, cat_id, scatt_id):

        if not self.cats.has_key(cat_id):
            return -1

        if self.cats[cat_id].has_key(scatt_id):
            return 0

        b_i = self.GetBandsInfo(scatt_id)

        shape = (b_i['b2']['max'] - b_i['b2']['min'] + 1, b_i['b1']['max'] - b_i['b1']['min'] + 1)

        np_vals = np.memmap(grass.tempfile(), dtype=self.dtype, mode='w+', shape = shape)

        self.cats[cat_id][scatt_id] = {'np_vals' : np_vals}

        return 1

    def GetBandsInfo(self, scatt_id):
        b1, b2 = idScattToidBands(scatt_id, len(self.an_data.GetBands()))

        b1_info = self.an_data.GetBandInfo(b1)
        b2_info = self.an_data.GetBandInfo(b2)

        bands_info = {'b1' : b1_info,
                      'b2' : b2_info}

        return bands_info

    def DeleScattPlot(self, cat_id, scatt_id):

        if not self.cats.has_key(cat_id):
            return False

        if not self.cats[cat_id].has_key(scatt_id):
            return False

        del self.cats[cat_id][scatt_id]
        return True

    def GetValuesArr(self, cat_id, scatt_id):

        if not self.cats.has_key(cat_id):
            return None

        if not self.cats[cat_id].has_key(scatt_id):
            return None

        return self.cats[cat_id][scatt_id]['np_vals']

    def GetData(self, requested_dt):
        
        cats = {}
        for cat_id, scatt_ids in requested_dt.iteritems():
            if not cats.has_key(cat_id):
                cats[cat_id] = {}
            for scatt_id in scatt_ids:
                # if key is missing condition is always True (full scatter plor is computed)
                if self.cats[cat_id].has_key(scatt_id):
                    cats[cat_id][scatt_id] = {'np_vals' : self.cats[cat_id][scatt_id]['np_vals'],
                                              'bands_info' : self.GetBandsInfo(scatt_id)}
                        
        return cats

    def SetData(self, cats):
        
        for cat_id, scatt_ids in cats.iteritems():            
            for scatt_id in scatt_ids:
                # if key is missing condition is always True (full scatter plor is computed)
                if self.cats[cat_id].has_key(scatt_id):
                    self.cats[cat_id][scatt_id]['np_vals'] = cats[cat_id][scatt_id]['np_vals']

    def GetScatt(self, scatt_id, cats_ids = None):
        scatts = {}
        for cat_id in self.cats.iterkeys():
            if cats_ids and cat_id not in cats_ids:
                continue
            if not self.cats[cat_id].has_key(scatt_id):
                continue

            scatts[cat_id] = {'np_vals' : self.cats[cat_id][scatt_id]['np_vals'],
                              'bands_info' : self.GetBandsInfo(scatt_id)}
        return scatts

                   
class ScattPlotsData(ScattPlotsCondsData):
    """!Data structure for computed points (classes) in scatter plots.\
    """
    def __init__(self, an_data):

        self.cats_rasts = {}
        self.cats_rasts_conds = {}    
        self.scatts_ids = []    

        ScattPlotsCondsData.__init__(self, an_data)

        self.dtype = 'uint32'

        #TODO
        self.type = 0

    def AddCategory(self, cat_id):
        cat_id = ScattPlotsCondsData.AddCategory(self, cat_id)
        if cat_id < 0:
            return cat_id

        for scatt_id in self.scatts_ids:
            ScattPlotsCondsData.AddScattPlot(self, cat_id, scatt_id)

        if cat_id == 0:
            self.cats_rasts_conds[cat_id] = None
            self.cats_rasts[cat_id] = None
        else:
            self.cats_rasts_conds[cat_id] = grass.tempfile()
            self.cats_rasts[cat_id] = "temp_cat_rast_%d_%d" % (cat_id, os.getpid())
            region = self.an_data.GetRegion()
            CreateCatRast(region, self.cats_rasts_conds[cat_id])

        return cat_id

    def DeleteCategory(self, cat_id):

        ScattPlotsCondsData.DeleteCategory(self, cat_id)
        
        grass.try_remove(self.cats_rasts_conds[cat_id])
        del self.cats_rasts_conds[cat_id]

        RunCommand("g.remove",
                   rast=self.cats_rasts[cat_id])
        del self.cats_rasts[cat_id]

        return True

    def AddScattPlot(self, scatt_id):
        
        if scatt_id in self.scatts_ids:
            return False

        self.scatts_ids.append(scatt_id)
        for cat_id in self.cats.iterkeys():
                ScattPlotsCondsData.AddScattPlot(self, cat_id, scatt_id)
                self.cats[cat_id][scatt_id]['ellipse'] = None

        return True

    def DeleteScatterPlot(self, scatt_id):
        
        if scatt_id not in self.scatts_ids:
            return False

        self.scatts_ids.remove(scatt_id)

        for cat_id in self.cats.iterkeys():
                ScattPlotsCondsData.DeleteScattPlot(self, cat_id, scatt_id)

        return True

    def GetEllipses(self, scatt_id, styles):
        if scatt_id not in self.scatts_ids:
            return False

        scatts = {}
        for cat_id in self.cats.iterkeys():
            if cat_id == 0:
                continue
            nstd = styles[cat_id]['nstd']
            scatts[cat_id] = self._getEllipse(cat_id, scatt_id, nstd)

        return scatts

    def _getEllipse(self, cat_id, scatt_id, nstd):
        # Joe Kington
        # http://stackoverflow.com/questions/12301071/multidimensional-confidence-intervals

        data = np.copy(self.cats[cat_id][scatt_id]['np_vals'])

        b = self.GetBandsInfo(scatt_id)
        sel_pts = np.where(data > 0)

        x = sel_pts[1]
        y = sel_pts[0]

        flatten_data = data.reshape([-1])
        flatten_sel_pts = np.nonzero(flatten_data)
        weights = flatten_data[flatten_sel_pts]
        if len(weights) == 0:
            return None

        x_avg = np.average(x, weights=weights)
        y_avg = np.average(y, weights=weights)
        pos = np.array([x_avg + b['b1']['min'], y_avg + b['b2']['min']])

        x_diff = (x - x_avg)
        y_diff = (y - y_avg)
        
        x_diff = (x - x_avg) 
        y_diff = (y - y_avg) 

        diffs = x_diff * y_diff.T
        cov = np.dot(diffs, weights) / (np.sum(weights) - 1)

        diffs = x_diff * x_diff.T
        var_x = np.dot(diffs, weights) /  (np.sum(weights) - 1)
        
        diffs = y_diff * y_diff.T
        var_y = np.dot(diffs, weights) /  (np.sum(weights) - 1)

        cov = np.array([[var_x, cov],[cov, var_y]])

        def eigsorted(cov):
            vals, vecs = np.linalg.eigh(cov)
            order = vals.argsort()[::-1]
            return vals[order], vecs[:,order]

        vals, vecs = eigsorted(cov)
        theta = np.degrees(np.arctan2(*vecs[:,0][::-1]))

        # Width and height are "full" widths, not radius
        width, height = 2 * nstd * np.sqrt(vals)

        ellipse = {'pos' : pos, 
                   'width' : width,
                   'height' : height,
                   'theta' : theta}

        del data
        del flatten_data
        del flatten_sel_pts
        del weights
        del sel_pts
        return ellipse

    def CleanUp(self):

        ScattPlotsCondsData.CleanUp(self)        
        for tmp in self.cats_rasts_conds.itervalues():
            grass.try_remove(tmp) 
        for tmp in self.cats_rasts.itervalues():
            RunCommand("g.remove",
                       rast=tmp,
                       getErrorMsg=True)

        self.cats_rasts = {}
        self.cats_rasts_conds = {}

    def GetCatRast(self, cat_id):
        if self.cats_rasts.has_key(cat_id):
            return self.cats_rasts[cat_id]
        return None

    def GetCatRastCond(self, cat_id):
        return self.cats_rasts_conds[cat_id]

    def GetCatsRastsConds(self):
        max_cat_id = max(self.cats_rasts_conds.keys())

        cats_rasts_conds = [''] * (max_cat_id + 1)
        for i_cat_id, i_rast in self.cats_rasts_conds.iteritems():
            cats_rasts_conds[i_cat_id] = i_rast

        return cats_rasts_conds

    def GetCatsRasts(self):
        max_cat_id = max(self.cats_rasts.keys())

        cats_rasts = [''] * (max_cat_id + 1)
        for i_cat_id, i_rast in self.cats_rasts.iteritems():
            cats_rasts[i_cat_id] = i_rast

        return cats_rasts


# not used,  using iclass_perimeter algorithm instead of scipy convolve2d
"""
def RasterizePolygon(pol, height, min_h, width, min_w):

    # Joe Kington
    # http://stackoverflow.com/questions/3654289/scipy-create-2d-polygon-mask

    #poly_verts = [(1,1), (1,4), (4,4),(4,1), (1,1)]

    nx = width
    ny = height

    x, y =  np.meshgrid(np.arange(-0.5 + min_w, nx + 0.5 + min_w, dtype=float), 
                        np.arange(-0.5 + min_h, ny + 0.5 + min_h, dtype=float))
    x, y = x.flatten(), y.flatten()

    points = np.vstack((x,y)).T

    p = Path(pol)
    grid = p.contains_points(points)
    grid = grid.reshape((ny + 1, nx + 1))
    raster = np.zeros((height, width), dtype=np.uint8)#TODO bool

    #TODO shift by 0.5
    B = np.ones((2,2))/4
    raster = convolve2d(grid, B, 'valid')

    return raster
"""

def idScattToidBands(scatt_id, n_bands):
    """!Get bands ids from scatter plot id.""" 
    n_b1 = n_bands - 1

    band_1 = (int) ((2 * n_b1 + 1 - sqrt(((2 * n_b1 + 1) * (2 * n_b1 + 1) - 8 * scatt_id))) / 2)

    band_2 = scatt_id - (band_1 * (2 * n_b1 + 1) - band_1 * band_1) / 2 + band_1 + 1

    return band_1, band_2

def idBandsToidScatt(band_1_id, band_2_id, n_bands):
    """!Get scatter plot id from band ids."""
    if band_2_id <  band_1_id:
        tmp = band_1_id
        band_1_id = band_2_id
        band_2_id = tmp

    n_b1 = n_bands - 1

    scatt_id = (band_1_id * (2 * n_b1 + 1) - band_1_id * band_1_id) / 2 + band_2_id - band_1_id - 1

    return scatt_id

def GetRegion():
    ret, region, msg = RunCommand("g.region",
                                  flags = "gp",
                                  getErrorMsg = True,
                                  read = True)

    if ret != 0:
        raise GException("g.region failed:\n%s" % msg)

    return _parseRegion(region)

def _parseRegion(region_str):

    region = {}
    region_str = region_str.splitlines()

    for param in region_str:
        k, v = param.split("=")
        if k in ["rows", "cols", "cells"]:
            v = int(v)
        else:
            v = float(v)
        region[k] = v

    return region

def GetRasterInfo(rast):
    ret, out, msg = RunCommand("r.info",
                                map = rast,
                                flags = "rg",
                                getErrorMsg = True,
                                read = True)

    if  ret != 0:
        raise GException("r.info failed:\n%s" % msg)

    out = out.split("\n")
    raster_info = {} 

    for b in out:
        if not b.strip():
            continue
        k, v = b.split("=")
        if k == "datatype":
            if v != "CELL":
                return None
            pass
        elif k in ['rows', 'cols', 'cells', 'min', 'max']:
            v = int(v)
        else:
            v = float(v)

        raster_info[k] = v

    raster_info['range'] = raster_info['max'] - raster_info['min'] + 1
    return raster_info