"""
@package gmodeler.giface

@brief wxGUI Graphical Modeler GRASS interface

Classes:
 - giface::GraphicalModelerGrassInterface

(C) 2013-2018 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com>
"""

from grass.pydispatch.signal import Signal


class GraphicalModelerGrassInterface():
    """@implements core::giface::GrassInterface"""

    def __init__(self, model):
        self._model = model

        # Signal emitted to request updating of map (TODO)
        self.updateMap = Signal('GraphicalModelerGrassInterface.updateMap')

    def GetLayerTree(self):
        return None

    def GetLayerList(self, prompt):
        return self._model.GetMaps(prompt)

    def GetMapDisplay(self):
        """
        ..todo::
            implement connection with mapdisplay
        """
        return None
