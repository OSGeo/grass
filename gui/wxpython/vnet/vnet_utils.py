"""!
@package vnet.vnet_utils

@brief Vector network analysis utilities.

Classes:
 - vnet_core::VNETTmpVectMaps
 - vnet_core::VectMap
 - vnet_core::History

(C) 2013 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Stepan Turek <stepan.turek seznam.cz> (GSoC 2012, mentor: Martin Landa)
@author Lukas Bocan <silent_bob centrum.cz> (turn costs support)
@author Eliska Kyzlikova <eliska.kyzlikova gmail.com> (turn costs support)
"""

import  math
from grass.script     import core as grass

try:
    import grass.lib.vector as vectlib
    from ctypes import pointer, byref, c_char_p, c_int, c_double, POINTER
    haveCtypes = True
except ImportError:
    haveCtypes = False

def ParseMapStr(mapStr):
    """!Create full map name (add current mapset if it is not present in name)"""
    mapValSpl = mapStr.strip().split("@")
    if len(mapValSpl) > 1:
        mapSet = mapValSpl[1]
    else:
        mapSet = grass.gisenv()['MAPSET']
    mapName = mapValSpl[0] 
        
    return mapName, mapSet

def DegreesToRadians(degrees):
    return degrees * math.pi / 180

def RadiansToDegrees(radians):
    return radians * 180 / math.pi   

def SnapToNode(e, n, tresh, vectMap):
    """!Find nearest node to click coordinates (within given threshold)"""
    if not haveCtypes:
        return None

    vectMap, mapSet = ParseMapStr(vectMap)

    openedMap = pointer(vectlib.Map_info())
    ret = vectlib.Vect_open_old(openedMap, 
                                     c_char_p(vectMap),
                                     c_char_p(mapSet))
    if ret == 1:
        vectlib.Vect_close(openedMap)
    if ret != 2: 
        return None

    nodeNum =  vectlib.Vect_find_node(openedMap,     
                                      c_double(e), 
                                      c_double(n), 
                                      c_double(0), 
                                      c_double(tresh),
                                      vectlib.WITHOUT_Z)

    if nodeNum > 0:
        e = c_double(0)
        n = c_double(0)
        vectlib.Vect_get_node_coor(openedMap, 
                                   nodeNum, 
                                   byref(e), 
                                   byref(n), 
                                   None); # z
        e = e.value
        n = n.value
    else:
        vectlib.Vect_close(openedMap)
        return False

    return e, n

def GetNearestNodeCat(e, n, layer, tresh, vectMap):

    if  not haveCtypes:
        return -2

    vectMapName, mapSet = ParseMapStr(vectMap)

    openedMap = pointer(vectlib.Map_info())
    ret = vectlib.Vect_open_old(openedMap, 
                                     c_char_p(vectMapName),
                                     c_char_p(mapSet))
    if ret == 1:
        vectlib.Vect_close(openedMap)
    if ret != 2: 
        return -1

    nodeNum = vectlib.Vect_find_node(openedMap,     
                                     c_double(e), 
                                     c_double(n), 
                                     c_double(0), 
                                     c_double(tresh),
                                     vectlib.WITHOUT_Z)

    if nodeNum > 0:
        e = c_double(0)
        n = c_double(0)
        vectlib.Vect_get_node_coor(openedMap, 
                                   nodeNum, 
                                   byref(e), 
                                   byref(n), 
                                   None); # z
        e = e.value
        n = n.value
    else:
        vectlib.Vect_close(openedMap)
        return -1

    box = vectlib.bound_box();
    List = POINTER(vectlib.boxlist);
    List = vectlib.Vect_new_boxlist(c_int(0));

    box.E = box.W = e;
    box.N = box.S = n;
    box.T = box.B = 0;
    vectlib.Vect_select_lines_by_box(openedMap, byref(box), vectlib.GV_POINT, List);

    found = 0;
    dcost = 0;

    Cats = POINTER(vectlib.line_cats)
    Cats = vectlib.Vect_new_cats_struct()
 
    cat = c_int(0)

    for j in range(List.contents.n_values): 
        line = List.contents.id[j]
        type = vectlib.Vect_read_line(openedMap, None, Cats, line)
        if type != vectlib.GV_POINT:
            continue

        if vectlib.Vect_cat_get(Cats, c_int(layer), byref(cat)): 
            found = 1
            break
    if found:
        return cat.value

    return -1