"""!
@package iscatt.core_c

@brief Wrappers for scatter plot C backend.

(C) 2013 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Stepan Turek <stepan.turek seznam.cz> (mentor: Martin Landa)
"""

import sys
import numpy as np
from multiprocessing import Process, Queue

from ctypes import *
try:
    from grass.lib.imagery import *
    from grass.lib.gis import Cell_head, G_get_window
except ImportError, e:
    sys.stderr.write(_("Loading ctypes libs failed"))

from core.gcmd import GException

def Rasterize(polygon, rast, region, value):
    rows, cols = rast.shape 

    #TODO creating of region is on many places
    region['rows'] = rows
    region['cols'] = cols

    region['nsres'] = 1.0
    region['ewres'] = 1.0

    q = Queue()
    p = Process(target=_rasterize, args=(polygon, rast, region, value, q))
    p.start()
    rast = q.get()
    p.join()

    return rast

def ApplyColormap(vals, vals_mask, colmap, out_vals):
    
    c_uint8_p = POINTER(c_uint8)

    vals_p = vals.ctypes.data_as(c_uint8_p)


    if hasattr(vals_mask, "ctypes"):
        vals_mask_p = vals_mask.ctypes.data_as(c_uint8_p)
    else: #vals mask is empty (all data are selected)
        vals_mask_p = None
    colmap_p = colmap.ctypes.data_as(c_uint8_p)
    out_vals_p = out_vals.ctypes.data_as(c_uint8_p)

    vals_size = vals.reshape((-1)).shape[0]
    I_apply_colormap(vals_p, vals_mask_p, vals_size, colmap_p, out_vals_p)

def MergeArrays(merged_arr, overlay_arr, alpha):
    if merged_arr.shape != overlay_arr.shape:
        GException("MergeArrays: merged_arr.shape != overlay_arr.shape")

    c_uint8_p = POINTER(c_uint8)
    merged_p = merged_arr.ctypes.data_as(c_uint8_p)
    overlay_p = overlay_arr.ctypes.data_as(c_uint8_p)

    I_merge_arrays(merged_p, overlay_p, merged_arr.shape[0], merged_arr.shape[1], alpha)

def MergeArrays(merged_arr, overlay_arr, alpha):
    if merged_arr.shape != overlay_arr.shape:
        GException("MergeArrays: merged_arr.shape != overlay_arr.shape")

    c_uint8_p = POINTER(c_uint8)
    merged_p = merged_arr.ctypes.data_as(c_uint8_p)
    overlay_p = overlay_arr.ctypes.data_as(c_uint8_p)

    I_merge_arrays(merged_p, overlay_p, merged_arr.shape[0], merged_arr.shape[1], alpha)

def ComputeScatts(region, scatt_conds, bands, n_bands, scatts, cats_rasts_conds, cats_rasts):

    q = Queue()
    p = Process(target=_computeScattsProcess, args=(region, scatt_conds, bands, 
                                                    n_bands, scatts, cats_rasts_conds, cats_rasts, q))
    p.start()
    ret = q.get()
    p.join()
    
    return ret[0], ret[1]

def UpdateCatRast(patch_rast, region, cat_rast):
    q = Queue()
    p = Process(target=_updateCatRastProcess, args=(patch_rast, region, cat_rast, q))
    p.start()
    ret = q.get()
    p.join()

    return ret

def CreateCatRast(region, cat_rast):
    cell_head = _regionToCellHead(region)
    I_create_cat_rast(pointer(cell_head), cat_rast)   

def _computeScattsProcess(region, scatt_conds, bands, n_bands, scatts, 
                          cats_rasts_conds, cats_rasts, output_queue):

    #TODO names for types not 0 and 1?
    sccats_c, cats_rasts_c, refs = _getComputationStruct(scatts, cats_rasts, 
                                                         SC_SCATT_DATA, n_bands)
    scatt_conds_c, cats_rasts_conds_c, refs2 = _getComputationStruct(scatt_conds, cats_rasts_conds, 
                                                                     SC_SCATT_CONDITIONS, n_bands)

    char_bands = _stringListToCharArr(bands)
   
    cell_head = _regionToCellHead(region)

    ret = I_compute_scatts(pointer(cell_head),
                           pointer(scatt_conds_c),
                           pointer(cats_rasts_conds_c),
                           pointer(char_bands),
                           n_bands,
                           pointer(sccats_c),
                           pointer(cats_rasts_c))

    I_sc_free_cats(pointer(sccats_c))
    I_sc_free_cats(pointer(scatt_conds_c))

    output_queue.put((ret, scatts))

def _getBandcRange( band_info):
    band_c_range = struct_Range()

    band_c_range.max = band_info['max']
    band_c_range.min = band_info['min']

    return band_c_range

def _regionToCellHead(region):
    cell_head = struct_Cell_head()
    G_get_window(pointer(cell_head))

    convert_dict = {'n' : 'north', 'e' : 'east', 
                    'w' : 'west',  's' : 'south', 
                    'nsres' : 'ns_res',
                    'ewres' : 'ew_res'}

    for k, v in region.iteritems():
        if k in ["rows", "cols", "cells"]:
            v = int(v)
        else:
            v = float(v)

        if convert_dict.has_key(k):
            k = convert_dict[k]
           
        setattr(cell_head, k, v)

    return cell_head

def _stringListToCharArr(str_list):

    arr = c_char_p * len(str_list)
    char_arr = arr()
    for i, st in enumerate(str_list):
        if st:
            char_arr[i] = st
        else:
            char_arr[i] = None

    return char_arr

def _getComputationStruct(cats, cats_rasts, cats_type, n_bands):

    sccats = struct_scCats()
    I_sc_init_cats(pointer(sccats), c_int(n_bands), c_int(cats_type));

    refs = []        
    cats_rasts_core = []
    
    for cat_id, scatt_ids in cats.iteritems():
        cat_c_id = I_sc_add_cat(pointer(sccats))
        cats_rasts_core.append(cats_rasts[cat_id])

        for scatt_id, dt in scatt_ids.iteritems():
            # if key is missing condition is always True (full scatter plor is computed)
                vals = dt['np_vals']

                scatt_vals = scdScattData()

                c_void_p = ctypes.POINTER(ctypes.c_void_p)

                if cats_type == SC_SCATT_DATA:
                    vals[:] = 0
                elif cats_type == SC_SCATT_CONDITIONS:
                    pass
                else:
                    return None
                data_p = vals.ctypes.data_as(c_void_p)
                I_scd_init_scatt_data(pointer(scatt_vals), cats_type, len(vals), data_p)

                refs.append(scatt_vals)

                I_sc_insert_scatt_data(pointer(sccats),  
                                       pointer(scatt_vals),
                                       cat_c_id, scatt_id)

    cats_rasts_c = _stringListToCharArr(cats_rasts_core)

    return sccats, cats_rasts_c, refs

def _updateCatRastProcess(patch_rast, region, cat_rast, output_queue):
    cell_head = _regionToCellHead(region)
    
    
    ret = I_insert_patch_to_cat_rast(patch_rast, 
                                     pointer(cell_head), 
                                     cat_rast)

    output_queue.put(ret)

def _rasterize(polygon, rast, region, value, output_queue):    
    pol_size = len(polygon) * 2
    pol = np.array(polygon, dtype=float)

    c_uint8_p = POINTER(c_uint8)
    c_double_p = POINTER(c_double)

    pol_p = pol.ctypes.data_as(c_double_p)
    rast_p = rast.ctypes.data_as(c_uint8_p)

    cell_h = _regionToCellHead(region)
    I_rasterize(pol_p, 
                len(polygon), 
                value, 
                pointer(cell_h), rast_p)

    output_queue.put(rast)
