"""!
@package gmodeler.giface

@brief wxGUI Graphical Modeler GRASS interface

Classes:
 - giface::GraphicalModelerGrassInterface

(C) 2013 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com>
"""

class GraphicalModelerGrassInterface(object):
    """!@implements core::giface::GrassInterface"""
    def __init__(self, model):
        self._model = model
        
    def __getattr__(self, name):
        return getattr(self._giface, name)
    
    def GetLayerTree(self):
        return None
    
    def GetLayerList(self, prompt):
        return self._model.GetMaps(prompt)
