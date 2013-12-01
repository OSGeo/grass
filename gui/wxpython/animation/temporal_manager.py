"""!
@package animation.temporal_manager

@brief Management of temporal datasets used in animation

Classes:
 - temporal_manager::DataMode
 - temporal_manager::GranularityMode
 - temporal_manager::TemporalManager


(C) 2012 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Anna Kratochvilova <kratochanna gmail.com>
"""

import os
import sys

if __name__ == '__main__':
    sys.path.append(os.path.join(os.environ['GISBASE'], "etc", "gui", "wxpython"))

import grass.script as grass
import grass.temporal as tgis
from core.gcmd import GException
from core.utils import _
from animation.utils import validateTimeseriesName, TemporalType


class DataMode:
    SIMPLE = 1
    MULTIPLE = 2


class GranularityMode:
    ONE_UNIT = 1
    ORIGINAL = 2


class TemporalManager(object):
    """!Class for temporal data processing."""
    def __init__(self):
        self.timeseriesList = []
        self.timeseriesInfo = {}

        self.dataMode = None
        self.temporalType = None

        self.granularityMode = GranularityMode.ORIGINAL

        # Make sure the temporal database exists
        tgis.init()

    def GetTemporalType(self):
        """!Get temporal type (TemporalType.ABSOLUTE, TemporalType.RELATIVE)"""
        return self._temporalType

    def SetTemporalType(self, ttype):
        self._temporalType = ttype

    temporalType = property(fget=GetTemporalType, fset=SetTemporalType)

    def AddTimeSeries(self, timeseries, etype):
        """!Add space time dataset
        and collect basic information about it.

        Raises GException (e.g. with invalid topology).

        @param timeseries name of timeseries (with or without mapset)
        @param etype element type (strds, stvds)
        """
        self._gatherInformation(timeseries, etype, self.timeseriesList, self.timeseriesInfo)

    def EvaluateInputData(self):
        """!Checks if all timeseries are compatible (raises GException).

        Sets internal variables.
        """
        timeseriesCount = len(self.timeseriesList)

        if timeseriesCount == 1:
            self.dataMode = DataMode.SIMPLE
        elif timeseriesCount > 1:
            self.dataMode = DataMode.MULTIPLE
        else:
            self.dataMode = None

        ret, message = self._setTemporalState()
        if not ret:
            raise GException(message)
        if message:  # warning
            return message

        return None

    def _setTemporalState(self):
        # check for absolute x relative
        absolute, relative = 0, 0
        for infoDict in self.timeseriesInfo.values():
            if infoDict['temporal_type'] == 'absolute':
                absolute += 1
            else:
                relative += 1
        if bool(absolute) == bool(relative):
            message = _("It is not allowed to display data with different "
                        "temporal types (absolute and relative).")
            return False, message
        if absolute:
            self.temporalType = TemporalType.ABSOLUTE
        else:
            self.temporalType = TemporalType.RELATIVE

        # check for units for relative type
        if relative:
            units = set()
            for infoDict in self.timeseriesInfo.values():
                units.add(infoDict['unit'])
            if len(units) > 1:
                message = _("It is not allowed to display data with different units (%s).") % ','.join(units)
                return False, message

        # check for interval x point
        interval, point = 0, 0
        for infoDict in self.timeseriesInfo.values():
            if infoDict['map_time'] == 'interval':
                interval += 1
            else:
                point += 1
        if bool(interval) == bool(point):
            message = _("You are going to display data with different "
                        "temporal types of maps (interval and point)."
                        " It is recommended to use data of one temporal type to avoid confusion.")
            return True, message  # warning

        return True, None

    def GetGranularity(self):
        """!Returns temporal granularity of currently loaded timeseries."""
        if self.dataMode == DataMode.SIMPLE:
            gran = self.timeseriesInfo[self.timeseriesList[0]]['granularity']
            if 'unit' in self.timeseriesInfo[self.timeseriesList[0]]:  # relative:
                granNum = gran
                unit = self.timeseriesInfo[self.timeseriesList[0]]['unit']
                if self.granularityMode == GranularityMode.ONE_UNIT:
                    granNum = 1
            else:  # absolute
                granNum, unit = gran.split()
                if self.granularityMode == GranularityMode.ONE_UNIT:
                    granNum = 1

            return (int(granNum), unit)

        if self.dataMode == DataMode.MULTIPLE:
            return self._getCommonGranularity()

    def _getCommonGranularity(self):
        allMaps = []
        for dataset in self.timeseriesList:
            maps = self.timeseriesInfo[dataset]['maps']
            allMaps.extend(maps)

        if self.temporalType == TemporalType.ABSOLUTE:
            gran = tgis.compute_absolute_time_granularity(allMaps)
            granNum, unit = gran.split()
            if self.granularityMode == GranularityMode.ONE_UNIT:
                granNum = 1
            return int(granNum), unit
        if self.temporalType == TemporalType.RELATIVE:
            unit = self.timeseriesInfo[self.timeseriesList[0]]['unit']
            granNum = tgis.compute_relative_time_granularity(allMaps)
            if self.granularityMode == GranularityMode.ONE_UNIT:
                granNum = 1
            return (granNum, unit)

    def GetLabelsAndMaps(self):
        """!Returns time labels and map names.
        """
        mapLists = []
        labelLists = []
        labelListSet = set()
        for dataset in self.timeseriesList:
            grassLabels, listOfMaps = self._getLabelsAndMaps(dataset)
            mapLists.append(listOfMaps)
            labelLists.append(tuple(grassLabels))
            labelListSet.update(grassLabels)
        # combine all timeLabels and fill missing maps with None
        # BUT this does not work properly if the datasets have
        # no temporal overlap! We would need to sample all datasets
        # by a temporary dataset, I don't know how it would work with point data
        if self.temporalType == TemporalType.ABSOLUTE:
            # ('1996-01-01 00:00:00', '1997-01-01 00:00:00', 'year'),
            timestamps = sorted(list(labelListSet), key=lambda x: x[0])
        else:
            # ('15', '16', u'years'),
            timestamps = sorted(list(labelListSet), key=lambda x: float(x[0]))

        newMapLists = []
        for mapList, labelList in zip(mapLists, labelLists):
            newMapList = [None] * len(timestamps)
            i = 0
            # compare start time
            while timestamps[i][0] != labelList[0][0]:  # compare
                i += 1
            newMapList[i:i + len(mapList)] = mapList
            newMapLists.append(newMapList)

        mapDict = {}
        for i, dataset in enumerate(self.timeseriesList):
            mapDict[dataset] = newMapLists[i]

        return timestamps, mapDict

    def _getLabelsAndMaps(self, timeseries):
        """!Returns time labels and map names (done by sampling)
        for both interval and point data.
        """
        sp = tgis.dataset_factory(self.timeseriesInfo[timeseries]['etype'], timeseries)
        if sp.is_in_db() is False:
            raise GException(_("Space time dataset <%s> not found.") % timeseries)
        sp.select()

        listOfMaps = []
        timeLabels = []
        granNum, unit = self.GetGranularity()
        if self.temporalType == TemporalType.ABSOLUTE:
            if self.granularityMode == GranularityMode.ONE_UNIT:
                gran = '%(one)d %(unit)s' % {'one': 1, 'unit': unit}
            else:
                gran = '%(num)d %(unit)s' % {'num': granNum, 'unit': unit}

        elif self.temporalType == TemporalType.RELATIVE:
            unit = self.timeseriesInfo[timeseries]['unit']
            if self.granularityMode == GranularityMode.ONE_UNIT:
                gran = 1
            else:
                gran = granNum
        # start sampling - now it can be used for both interval and point data
        # after instance, there can be a gap or an interval
        # if it is a gap we remove it and put there the previous instance instead
        # however the first gap must be removed to avoid duplication
        maps = sp.get_registered_maps_as_objects_by_granularity(gran=gran)
        if maps and len(maps) > 0:
            lastTimeseries = None
            followsPoint = False  # indicates that we are just after finding a point
            afterPoint = False  # indicates that we are after finding a point
            for mymap in maps:
                if isinstance(mymap, list):
                    if len(mymap) > 0:
                        map = mymap[0]
                else:
                    map = mymap

                series = map.get_id()

                start, end = map.get_temporal_extent_as_tuple()
                if self.timeseriesInfo[timeseries]['map_time'] == 'point':
                    # point data
                    listOfMaps.append(series)
                    afterPoint = True
                    followsPoint = True
                    lastTimeseries = series
                    end = None
                else:
                    end = str(end)
                    # interval data
                    if series:
                        # map exists, stop point mode
                        listOfMaps.append(series)
                        afterPoint = False
                    else:
                        # check point mode
                        if afterPoint:
                            if followsPoint:
                                # skip this one, already there
                                followsPoint = False
                                continue
                            else:
                                # append the last one (of point time)
                                listOfMaps.append(lastTimeseries)
                                end = None
                        else:
                            # append series which is None
                            listOfMaps.append(series)
                timeLabels.append((str(start), end, unit))

        return timeLabels, listOfMaps

    def _pretifyTimeLabels(self, labels):
        """!Convert absolute time labels to grass time and
        leave only datum when time is 0.
        """
        grassLabels = []
        isTime = False
        for start, end, unit in labels:
            start = tgis.string_to_datetime(start)
            start = tgis.datetime_to_grass_datetime_string(start)
            if end is not None:
                end = tgis.string_to_datetime(end)
                end = tgis.datetime_to_grass_datetime_string(end)
            grassLabels.append((start, end, unit))
            if '00:00:00' not in start or (end is not None and '00:00:00' not in end):
                isTime = True
        if not isTime:
            for i, (start, end, unit) in enumerate(grassLabels):
                start = start.replace('00:00:00', '').strip()
                if end is not None:
                    end = end.replace('00:00:00', '').strip()
                grassLabels[i] = (start, end, unit)
        return grassLabels

    def _gatherInformation(self, timeseries, etype, timeseriesList, infoDict):
        """!Get info about timeseries and check topology (raises GException)"""
        id = validateTimeseriesName(timeseries, etype)
        sp = tgis.dataset_factory(etype, id)
        # Insert content from db
        sp.select()
        # Get ordered map list
        maps = sp.get_registered_maps_as_objects()

        if not sp.check_temporal_topology(maps):
            raise GException(_("Topology of Space time dataset %s is invalid." % id))

        timeseriesList.append(id)
        infoDict[id] = {}
        infoDict[id]['etype'] = etype
        infoDict[id]['temporal_type'] = sp.get_temporal_type()
        if sp.is_time_relative():
            infoDict[id]['unit'] = sp.get_relative_time_unit()
        infoDict[id]['granularity'] = sp.get_granularity()
        infoDict[id]['map_time'] = sp.get_map_time()
        infoDict[id]['maps'] = maps


def test():
    from pprint import pprint

    temp = TemporalManager()
#    timeseries = createAbsolutePoint()
#    timeseries = createRelativePoint()
#    timeseries1, timeseries2 = createAbsoluteInterval()
    timeseries1, timeseries2 = createRelativeInterval()

    temp.AddTimeSeries(timeseries1, 'strds')
    temp.AddTimeSeries(timeseries2, 'strds')

    try:
        warn = temp.EvaluateInputData()
        print warn
    except GException, e:
        print e
        return

    print '///////////////////////////'
    gran = temp.GetGranularity()
    print "granularity: " + str(gran)
    pprint (temp.GetLabelsAndMaps())


def createAbsoluteInterval():
    grass.run_command('g.region', s=0, n=80, w=0, e=120, b=0, t=50, res=10, res3=10,
                      flags='p3', quiet=True)

    grass.mapcalc(exp="prec_1 = rand(0, 550)", overwrite=True)
    grass.mapcalc(exp="prec_2 = rand(0, 450)", overwrite=True)
    grass.mapcalc(exp="prec_3 = rand(0, 320)", overwrite=True)
    grass.mapcalc(exp="prec_4 = rand(0, 510)", overwrite=True)
    grass.mapcalc(exp="prec_5 = rand(0, 300)", overwrite=True)
    grass.mapcalc(exp="prec_6 = rand(0, 650)", overwrite=True)

    grass.mapcalc(exp="temp_1 = rand(0, 550)", overwrite=True)
    grass.mapcalc(exp="temp_2 = rand(0, 450)", overwrite=True)
    grass.mapcalc(exp="temp_3 = rand(0, 320)", overwrite=True)
    grass.mapcalc(exp="temp_4 = rand(0, 510)", overwrite=True)
    grass.mapcalc(exp="temp_5 = rand(0, 300)", overwrite=True)
    grass.mapcalc(exp="temp_6 = rand(0, 650)", overwrite=True)

    n1 = grass.read_command("g.tempfile", pid=1, flags='d').strip()
    fd = open(n1, 'w')
    fd.write(
        "prec_1|2001-01-01|2001-02-01\n"
        "prec_2|2001-04-01|2001-05-01\n"
        "prec_3|2001-05-01|2001-09-01\n"
        "prec_4|2001-09-01|2002-01-01\n"
        "prec_5|2002-01-01|2002-05-01\n"
        "prec_6|2002-05-01|2002-07-01\n"
    )
    fd.close()

    n2 = grass.read_command("g.tempfile", pid=2, flags='d').strip()
    fd = open(n2, 'w')
    fd.write(
        "temp_1|2000-10-01|2001-01-01\n"
        "temp_2|2001-04-01|2001-05-01\n"
        "temp_3|2001-05-01|2001-09-01\n"
        "temp_4|2001-09-01|2002-01-01\n"
        "temp_5|2002-01-01|2002-05-01\n"
        "temp_6|2002-05-01|2002-07-01\n"
    )
    fd.close()
    name1 = 'absinterval1'
    name2 = 'absinterval2'
    grass.run_command('t.unregister', type='rast',
                      maps='prec_1,prec_2,prec_3,prec_4,prec_5,prec_6,'
                      'temp_1,temp_2,temp_3,temp_4,temp_5,temp_6')
    for name, fname in zip((name1, name2), (n1, n2)):
        grass.run_command('t.create', overwrite=True, type='strds',
                          temporaltype='absolute', output=name,
                          title="A test with input files", descr="A test with input files")
        grass.run_command('t.register', flags='i', input=name, file=fname, overwrite=True)

    return name1, name2


def createRelativeInterval():
    grass.run_command('g.region', s=0, n=80, w=0, e=120, b=0, t=50, res=10, res3=10,
                      flags='p3', quiet=True)

    grass.mapcalc(exp="prec_1 = rand(0, 550)", overwrite=True)
    grass.mapcalc(exp="prec_2 = rand(0, 450)", overwrite=True)
    grass.mapcalc(exp="prec_3 = rand(0, 320)", overwrite=True)
    grass.mapcalc(exp="prec_4 = rand(0, 510)", overwrite=True)
    grass.mapcalc(exp="prec_5 = rand(0, 300)", overwrite=True)
    grass.mapcalc(exp="prec_6 = rand(0, 650)", overwrite=True)

    grass.mapcalc(exp="temp_1 = rand(0, 550)", overwrite=True)
    grass.mapcalc(exp="temp_2 = rand(0, 450)", overwrite=True)
    grass.mapcalc(exp="temp_3 = rand(0, 320)", overwrite=True)
    grass.mapcalc(exp="temp_4 = rand(0, 510)", overwrite=True)
    grass.mapcalc(exp="temp_5 = rand(0, 300)", overwrite=True)
    grass.mapcalc(exp="temp_6 = rand(0, 650)", overwrite=True)

    n1 = grass.read_command("g.tempfile", pid=1, flags='d').strip()
    fd = open(n1, 'w')
    fd.write(
        "prec_1|1|4\n"
        "prec_2|6|7\n"
        "prec_3|7|10\n"
        "prec_4|10|11\n"
        "prec_5|11|14\n"
        "prec_6|14|17\n"
    )
    fd.close()

    n2 = grass.read_command("g.tempfile", pid=2, flags='d').strip()
    fd = open(n2, 'w')
    fd.write(
        "temp_1|5|6\n"
        "temp_2|6|7\n"
        "temp_3|7|10\n"
        "temp_4|10|11\n"
        "temp_5|11|18\n"
        "temp_6|19|22\n"
    )
    fd.close()
    name1 = 'relinterval1'
    name2 = 'relinterval2'
    grass.run_command('t.unregister', type='rast',
                      maps='prec_1,prec_2,prec_3,prec_4,prec_5,prec_6,'
                      'temp_1,temp_2,temp_3,temp_4,temp_5,temp_6')
    for name, fname in zip((name1, name2), (n1, n2)):
        grass.run_command('t.create', overwrite=True, type='strds',
                          temporaltype='relative', output=name,
                          title="A test with input files", descr="A test with input files")
        grass.run_command('t.register', flags='i', input=name, file=fname, unit="years", overwrite=True)
    return name1, name2


def createAbsolutePoint():
    grass.run_command('g.region', s=0, n=80, w=0, e=120, b=0, t=50, res=10, res3=10,
                      flags='p3', quiet=True)

    grass.mapcalc(exp="prec_1 = rand(0, 550)", overwrite=True)
    grass.mapcalc(exp="prec_2 = rand(0, 450)", overwrite=True)
    grass.mapcalc(exp="prec_3 = rand(0, 320)", overwrite=True)
    grass.mapcalc(exp="prec_4 = rand(0, 510)", overwrite=True)
    grass.mapcalc(exp="prec_5 = rand(0, 300)", overwrite=True)
    grass.mapcalc(exp="prec_6 = rand(0, 650)", overwrite=True)

    n1 = grass.read_command("g.tempfile", pid=1, flags='d').strip()
    fd = open(n1, 'w')
    fd.write(
        "prec_1|2001-01-01\n"
        "prec_2|2001-03-01\n"
        "prec_3|2001-04-01\n"
        "prec_4|2001-05-01\n"
        "prec_5|2001-08-01\n"
        "prec_6|2001-09-01\n"
    )
    fd.close()
    name = 'abspoint'
    grass.run_command('t.create', overwrite=True, type='strds',
                      temporaltype='absolute', output=name,
                      title="A test with input files", descr="A test with input files")

    grass.run_command('t.register', flags='i', input=name, file=n1, overwrite=True)
    return name


def createRelativePoint():
    grass.run_command('g.region', s=0, n=80, w=0, e=120, b=0, t=50, res=10, res3=10,
                      flags='p3', quiet=True)

    grass.mapcalc(exp="prec_1 = rand(0, 550)", overwrite=True)
    grass.mapcalc(exp="prec_2 = rand(0, 450)", overwrite=True)
    grass.mapcalc(exp="prec_3 = rand(0, 320)", overwrite=True)
    grass.mapcalc(exp="prec_4 = rand(0, 510)", overwrite=True)
    grass.mapcalc(exp="prec_5 = rand(0, 300)", overwrite=True)
    grass.mapcalc(exp="prec_6 = rand(0, 650)", overwrite=True)

    n1 = grass.read_command("g.tempfile", pid=1, flags='d').strip()
    fd = open(n1, 'w')
    fd.write(
        "prec_1|1\n"
        "prec_2|3\n"
        "prec_3|5\n"
        "prec_4|7\n"
        "prec_5|11\n"
        "prec_6|13\n"
    )
    fd.close()
    name = 'relpoint'
    grass.run_command('t.create', overwrite=True, type='strds',
                      temporaltype='relative', output=name,
                      title="A test with input files", descr="A test with input files")

    grass.run_command('t.register', unit="day", input=name, file=n1, overwrite=True)
    return name

if __name__ == '__main__':

    test()
