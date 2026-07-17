"""
@package gmodeler.giface

@brief wxGUI Graphical Modeler GRASS interface

Classes:
 - giface::GraphicalModelerGrassInterface

SPDX-FileCopyrightText: 2013-2018 Other GRASS authors
SPDX-License-Identifier: GPL-2.0-or-later

@author Martin Landa <landa.martin gmail.com>
"""

from grass.pydispatch.signal import Signal


class GraphicalModelerGrassInterface:
    """@implements core::giface::GrassInterface"""

    def __init__(self, model, giface):
        self._model = model
        self._giface = giface

        # Signal emitted to request updating of map (TODO)
        self.updateMap = Signal("GraphicalModelerGrassInterface.updateMap")

    def GetLayerList(self, prompt):
        return self._model.GetMaps(prompt)

    def __getattr__(self, name):
        return getattr(self._giface, name)
