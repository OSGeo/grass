"""
@package iclass.statistics

@brief wxIClass classes for storing statistics about cells in training areas.

Classes:
 - statistics::StatisticsData
 - statistics::Statistics
 - statistics::BandStatistics

(C) 2006-2011, 2013 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Vaclav Petras <wenzeslaus gmail.com>
@author Anna Kratochvilova <kratochanna gmail.com>
"""

import os
import six
from ctypes import *

import grass.script as grass

try:
    from grass.lib.imagery import *
except ImportError as e:
    sys.stderr.write(_("Loading imagery lib failed"))

from grass.pydispatch.signal import Signal


class StatisticsData:
    """Stores all statistics."""

    def __init__(self):
        self.statisticsDict = {}
        self.statisticsList = []

        self.statisticsAdded = Signal("StatisticsData.statisticsAdded")
        self.statisticsDeleted = Signal("StatisticsData.statisticsDeleted")
        self.allStatisticsDeleted = Signal("StatisticsData.allStatisticsDeleted")

        self.statisticsSet = Signal("StatisticsData.statisticsSet")

    def GetStatistics(self, cat):
        return self.statisticsDict[cat]

    def AddStatistics(self, cat, name, color):
        st = Statistics()
        st.SetBaseStatistics(cat=cat, name=name, color=color)
        st.statisticsSet.connect(
            lambda stats: self.statisticsSet.emit(cat=cat, stats=stats)
        )

        self.statisticsDict[cat] = st
        self.statisticsList.append(cat)

        self.statisticsAdded.emit(cat=cat, name=name, color=color)

    def DeleteStatistics(self, cat):
        del self.statisticsDict[cat]
        self.statisticsList.remove(cat)

        self.statisticsDeleted.emit(cat=cat)

    def GetCategories(self):
        return self.statisticsList[:]

    def DeleteAllStatistics(self):
        self.statisticsDict.clear()
        del self.statisticsList[:]  # not ...=[] !

        self.allStatisticsDeleted.emit()


class Statistics:
    """Statistis connected to one class (category).

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

        self.statisticsSet = Signal("Statistics.statisticsSet")

    def SetReady(self, ready=True):
        self.ready = ready

    def IsReady(self):
        return self.ready

    def SetBaseStatistics(self, cat, name, color):
        """Sets basic (non-statistical) values.

        .. todo::
            Later self.name is changed but self.rasterName is not.
            self.rasterName should not be set by user. It can remains
            the same. But it should be done more explicitly. Currently
            it looks like unintentional feature or bug.
        """
        self.category = cat
        self.name = name
        self.color = color

        rasterPath = grass.tempfile(create=False)
        name = name.replace(" ", "_")
        self.rasterName = name + "_" + os.path.basename(rasterPath)

    def SetFromcStatistics(self, cStatistics):
        """Sets all statistical values.

        Copies all statistic values from \a cStattistics.

        :param cStatistics: pointer to C statistics structure
        """
        cat = c_int()

        set_stats = {}
        I_iclass_statistics_get_cat(cStatistics, byref(cat))
        if self.category != cat.value:
            set_stats["category"] = cat.value

        name = c_char_p()
        I_iclass_statistics_get_name(cStatistics, byref(name))
        if self.name != name.value:
            set_stats["name"] = grass.decode(name.value)

        color = c_char_p()
        I_iclass_statistics_get_color(cStatistics, byref(color))
        if self.color != color.value:
            set_stats["color"] = grass.decode(color.value)

        nbands = c_int()
        I_iclass_statistics_get_nbands(cStatistics, byref(nbands))
        if self.nbands != nbands.value:
            set_stats["nbands"] = nbands.value

        ncells = c_int()
        I_iclass_statistics_get_ncells(cStatistics, byref(ncells))
        if self.ncells != ncells.value:
            set_stats["ncells"] = ncells.value

        nstd = c_float()
        I_iclass_statistics_get_nstd(cStatistics, byref(nstd))
        if self.nstd != nstd.value:
            set_stats["nstd"] = nstd.value

        self.SetStatistics(set_stats)
        self.SetBandStatistics(cStatistics)

    def SetBandStatistics(self, cStatistics):
        """Sets all band statistics.

        :param cStatistics: pointer to C statistics structure
        """
        self.bands = []
        for i in range(self.nbands):
            band = BandStatistics()
            band.SetFromcStatistics(cStatistics, index=i)
            self.bands.append(band)

    def SetStatistics(self, stats):

        for st, val in six.iteritems(stats):
            setattr(self, st, val)

        self.statisticsSet.emit(stats=stats)


class BandStatistics:
    """Statistis connected to one band within class (category).

    :class:`Statistics`
    """

    def __init__(self):
        self.min = self.max = None
        self.rangeMin = self.rangeMax = None
        self.mean = None
        self.stddev = None
        self.histo = [0] * 256  # max categories

    def SetFromcStatistics(self, cStatistics, index):
        """Sets statistics for one band by given index.

        :param cStatistics: pointer to C statistics structure
        :param index: index of band in C statistics structure
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
