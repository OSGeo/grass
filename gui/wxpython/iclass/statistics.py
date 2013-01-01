"""!
@package iclass.statistics

@brief wxIClass classes for storing statistics about cells in training areas.

Classes:
 - statistics::Statistics
 - statistics::BandStatistics

(C) 2006-2011 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Vaclav Petras <wenzeslaus gmail.com>
@author Anna Kratochvilova <kratochanna gmail.com>
"""

import os
from ctypes import *

import grass.script as grass

try:
    from grass.lib.imagery import *
except ImportError, e:
    sys.stderr.write(_("Loading imagery lib failed"))

class Statistics:
    """! Statistis conected to one class (category).
    
    It is Python counterpart of similar C structure.
    But it adds some attributes or features used in wxIClass.
    It is not interface to C structure (it copies values).
    """
    def __init__(self):
        self.category = -1
        self.name = ""
        self.rasterName = ""
        self.color = "0:0:0"
        self.nbands = 0
        self.ncells = 0
        self.nstd = 1.5
        self.bands = []
        self.ready = False
        
        
    def SetReady(self, ready = True):
        self.ready = ready
        
    def IsReady(self):
        return self.ready
        
    def SetBaseStatistics(self, cat, name, color):
        """! Sets basic (non-statistical) values.
        
        @todo Later self.name is changed but self.rasterName is not.
        self.rasterName should not be set by user. It can remains the same.
        But it should be done more explicitly.
        Currently it looks like unintentional feature or bug.
        """
        self.category = cat
        self.name = name
        self.color = color
        
        rasterPath = grass.tempfile(create = False)
        name = name.replace(' ', '_')
        self.rasterName = name + '_' + os.path.basename(rasterPath)
        
    def SetStatistics(self, cStatistics):
        """! Sets all statistical values.
        
        Copies all statistic values from \a cStattistics.
        
        @param cStatistics pointer to C statistics structure
        """
        cat = c_int()
        I_iclass_statistics_get_cat(cStatistics, byref(cat))
        self.category = cat.value
        
        name = c_char_p()
        I_iclass_statistics_get_name(cStatistics, byref(name))
        self.name = name.value
        
        color = c_char_p()
        I_iclass_statistics_get_color(cStatistics, byref(color))
        self.color = color.value
        
        nbands = c_int()
        I_iclass_statistics_get_nbands(cStatistics, byref(nbands))
        self.nbands = nbands.value
        
        ncells = c_int()
        I_iclass_statistics_get_ncells(cStatistics, byref(ncells))
        self.ncells = ncells.value
        
        nstd = c_float()
        I_iclass_statistics_get_nstd(cStatistics, byref(nstd))
        self.nstd = nstd.value
        
        self.SetBandStatistics(cStatistics)
        
    def SetBandStatistics(self, cStatistics):
        """! Sets all band statistics.
        
        @param cStatistics pointer to C statistics structure
        """
        self.bands = []
        for i in range(self.nbands):
            band = BandStatistics()
            band.SetStatistics(cStatistics, index = i)
            self.bands.append(band)
        
class BandStatistics:
    """! Statistis conected to one band within class (category).
    
    @see Statistics
    """
    def __init__(self):
        self.min = self.max = None
        self.rangeMin = self.rangeMax = None
        self.mean = None
        self.stddev = None
        self.histo = [0] * 256 # max categories
        
        
    def SetStatistics(self, cStatistics, index):
        """! Sets statistics for one band by given index.
        
        @param cStatistics pointer to C statistics structure
        @param index index of band in C statistics structure
        """
        min, max = c_int(), c_int()
        I_iclass_statistics_get_min(cStatistics, index, byref(min))
        I_iclass_statistics_get_max(cStatistics, index, byref(max))
        self.min, self.max = min.value, max.value
        
        rangeMin, rangeMax = c_int(), c_int()
        I_iclass_statistics_get_range_min(cStatistics, index, byref(rangeMin))
        I_iclass_statistics_get_range_max(cStatistics, index, byref(rangeMax))
        self.rangeMin, self.rangeMax = rangeMin.value, rangeMax.value
        
        mean, stddev = c_float(), c_float()
        I_iclass_statistics_get_mean(cStatistics, index, byref(mean))
        I_iclass_statistics_get_stddev(cStatistics, index, byref(stddev))
        self.mean, self.stddev = mean.value, stddev.value
        
        histo = c_int()
        for i in range(len(self.histo)):
            I_iclass_statistics_get_histo(cStatistics, index, i, byref(histo))
            self.histo[i] = histo.value
        
