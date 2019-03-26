"""
Functions to compute the temporal granularity of a map list

Usage:

.. code-block:: python

    import grass.temporal as tgis

    tgis.compute_relative_time_granularity(maps)


(C) 2012-2013 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

:authors: Soeren Gebbert
"""
from __future__ import print_function
from .datetime_math import *
from .core import get_tgis_message_interface
from functools import reduce
from collections import OrderedDict
import ast

SINGULAR_GRAN = ["second", "minute", "hour", "day", "week", "month", "year"]
PLURAL_GRAN = ["seconds", "minutes", "hours", "days", "weeks", "months",
               "years"]
SUPPORTED_GRAN = SINGULAR_GRAN + PLURAL_GRAN
CONVERT_GRAN = OrderedDict()
CONVERT_GRAN['year'] = '12 month'
CONVERT_GRAN['month'] = '30.436875 day'
CONVERT_GRAN['day'] = '24 hour'
CONVERT_GRAN['hour'] = '60 minute'
CONVERT_GRAN['minute'] = '60 second'
###############################################################################


def check_granularity_string(granularity, temporal_type):
    """Check if the granularity string is valid

        :param granularity: The granularity string
        :param temporal_type: The temporal type of the granularity relative or
                              absolute
        :return: True if valid, False if invalid

        .. code-block:: python

            >>> check_granularity_string("1 year", "absolute")
            True
            >>> check_granularity_string("1 month", "absolute")
            True
            >>> check_granularity_string("1 day", "absolute")
            True
            >>> check_granularity_string("1 minute", "absolute")
            True
            >>> check_granularity_string("1 hour", "absolute")
            True
            >>> check_granularity_string("1 second", "absolute")
            True
            >>> check_granularity_string("5 months", "absolute")
            True
            >>> check_granularity_string("5 days", "absolute")
            True
            >>> check_granularity_string("5 minutes", "absolute")
            True
            >>> check_granularity_string("5 years", "absolute")
            True
            >>> check_granularity_string("5 hours", "absolute")
            True
            >>> check_granularity_string("2 seconds", "absolute")
            True
            >>> check_granularity_string("1 secondo", "absolute")
            False
            >>> check_granularity_string("bla second", "absolute")
            False
            >>> check_granularity_string("bla", "absolute")
            False
            >>> check_granularity_string(1, "relative")
            True
            >>> check_granularity_string("bla", "relative")
            False

    """
    temporal_type

    if granularity is None:
        return False

    if temporal_type == "absolute":
        try:
            num, unit = granularity.split(" ")
        except:
            return False
        if unit not in SUPPORTED_GRAN:
            return False

        try:
            integer = int(num)
        except:
            return False
    elif temporal_type == "relative":
        try:
            integer = int(granularity)
        except:
            return False
    else:
        return False

    return True

###############################################################################


def compute_relative_time_granularity(maps):
    """Compute the relative time granularity

        Attention: The computation of the granularity
        is only correct in case of not overlapping intervals.
        Hence a correct temporal topology is required for computation.

        :param maps: a ordered by start_time list of map objects
        :return: An integer


        .. code-block:: python

            >>> import grass.temporal as tgis
            >>> tgis.init()
            >>> maps = []
            >>> for i in range(5):
            ...   map = tgis.RasterDataset("a%i@P"%i)
            ...   check = map.set_relative_time(i,i + 1,"seconds")
            ...   if check:
            ...     maps.append(map)
            >>> tgis.compute_relative_time_granularity(maps)
            1

            >>> maps = []
            >>> count = 0
            >>> timelist = ((0,3), (3,6), (6,9))
            >>> for t in timelist:
            ...   map = tgis.RasterDataset("a%i@P"%count)
            ...   check = map.set_relative_time(t[0],t[1],"years")
            ...   if check:
            ...     maps.append(map)
            ...   count += 1
            >>> tgis.compute_relative_time_granularity(maps)
            3

            >>> maps = []
            >>> count = 0
            >>> timelist = ((0,3), (4,6), (8,11))
            >>> for t in timelist:
            ...   map = tgis.RasterDataset("a%i@P"%count)
            ...   check = map.set_relative_time(t[0],t[1],"years")
            ...   if check:
            ...     maps.append(map)
            ...   count += 1
            >>> tgis.compute_relative_time_granularity(maps)
            1

            >>> maps = []
            >>> count = 0
            >>> timelist = ((0,8), (2,6), (5,9))
            >>> for t in timelist:
            ...   map = tgis.RasterDataset("a%i@P"%count)
            ...   check = map.set_relative_time(t[0],t[1],"months")
            ...   if check:
            ...     maps.append(map)
            ...   count += 1
            >>> tgis.compute_relative_time_granularity(maps)
            4

            >>> maps = []
            >>> count = 0
            >>> timelist = ((0,8), (8,12), (12,18))
            >>> for t in timelist:
            ...   map = tgis.RasterDataset("a%i@P"%count)
            ...   check = map.set_relative_time(t[0],t[1],"days")
            ...   if check:
            ...     maps.append(map)
            ...   count += 1
            >>> tgis.compute_relative_time_granularity(maps)
            2

            >>> maps = []
            >>> count = 0
            >>> timelist = ((0,None), (8,None), (12,None), (24,None))
            >>> for t in timelist:
            ...   map = tgis.RasterDataset("a%i@P"%count)
            ...   check = map.set_relative_time(t[0],t[1],"minutes")
            ...   if check:
            ...     maps.append(map)
            ...   count += 1
            >>> tgis.compute_relative_time_granularity(maps)
            4

            >>> maps = []
            >>> count = 0
            >>> timelist = ((0,None), (8,14), (18,None), (24,None))
            >>> for t in timelist:
            ...   map = tgis.RasterDataset("a%i@P"%count)
            ...   check = map.set_relative_time(t[0],t[1],"hours")
            ...   if check:
            ...     maps.append(map)
            ...   count += 1
            >>> tgis.compute_relative_time_granularity(maps)
            2

            >>> maps = []
            >>> count = 0
            >>> timelist = ((0,21),)
            >>> for t in timelist:
            ...   map = tgis.RasterDataset("a%i@P"%count)
            ...   check = map.set_relative_time(t[0],t[1],"hours")
            ...   if check:
            ...     maps.append(map)
            ...   count += 1
            >>> tgis.compute_relative_time_granularity(maps)
            21

    """

    # The interval time must be scaled to days resolution
    granularity = None
    delta = []
    # First we compute the timedelta of the intervals
    for map in maps:
        start, end = map.get_temporal_extent_as_tuple()
        if (start == 0 or start) and end:
            t = abs(end - start)
            delta.append(int(t))

    # Compute the timedelta of the gaps
    for i in range(len(maps)):
        if i < len(maps) - 1:
            relation = maps[i + 1].temporal_relation(maps[i])
            if relation == "after":
                start1, end1 = maps[i].get_temporal_extent_as_tuple()
                start2, end2 = maps[i + 1].get_temporal_extent_as_tuple()
                # Gaps are between intervals, intervals and
                # points, points and points
                if end1 and start2:
                    t = abs(end1 - start2)
                    delta.append(int(t))
                if not end1 and start2:
                    t = abs(start1 - start2)
                    delta.append(int(t))

    delta.sort()
    ulist = list(set(delta))
    if len(ulist) > 1:
        # Find greatest common divisor
        granularity = gcd_list(ulist)
    elif len(ulist) == 1:
        granularity = ulist[0]
    else:
        granularity = 0

    return granularity

###############################################################################


def compute_absolute_time_granularity(maps):
    """Compute the absolute time granularity

        Attention: The computation of the granularity
        is only correct in case of not overlapping intervals.
        Hence a correct temporal topology is required for computation.

        The computed granularity is returned as number of seconds or minutes
        or hours or days or months or years.

        :param maps: a ordered by start_time list of map objects
        :return: The temporal topology as string "integer unit"

        .. code-block:: python

            >>> import grass.temporal as tgis
            >>> import datetime
            >>> dt = datetime.datetime
            >>> tgis.init()
            >>> maps = []
            >>> count = 0
            >>> timelist = ((dt(2000,01,01),None), (dt(2000,02,01),None))
            >>> for t in timelist:
            ...   map = tgis.RasterDataset("a%i@P"%count)
            ...   check = map.set_absolute_time(t[0],t[1])
            ...   if check:
            ...     maps.append(map)
            ...   count += 1
            >>> tgis.compute_absolute_time_granularity(maps)
            '1 month'

            >>> maps = []
            >>> count = 0
            >>> timelist = ((dt(2000,01,01),None), (dt(2000,01,02),None), (dt(2000,01,03),None))
            >>> for t in timelist:
            ...   map = tgis.RasterDataset("a%i@P"%count)
            ...   check = map.set_absolute_time(t[0],t[1])
            ...   if check:
            ...     maps.append(map)
            ...   count += 1
            >>> tgis.compute_absolute_time_granularity(maps)
            '1 day'

            >>> maps = []
            >>> count = 0
            >>> timelist = ((dt(2000,01,01),None), (dt(2000,01,02),None), (dt(2000,05,04,0,5,30),None))
            >>> for t in timelist:
            ...   map = tgis.RasterDataset("a%i@P"%count)
            ...   check = map.set_absolute_time(t[0],t[1])
            ...   if check:
            ...     maps.append(map)
            ...   count += 1
            >>> tgis.compute_absolute_time_granularity(maps)
            '30 seconds'

            >>> maps = []
            >>> count = 0
            >>> timelist = ((dt(2000,01,01),dt(2000,05,02)), (dt(2000,05,04,2),None))
            >>> for t in timelist:
            ...   map = tgis.RasterDataset("a%i@P"%count)
            ...   check = map.set_absolute_time(t[0],t[1])
            ...   if check:
            ...     maps.append(map)
            ...   count += 1
            >>> tgis.compute_absolute_time_granularity(maps)
            '2 hours'

            >>> maps = []
            >>> count = 0
            >>> timelist = ((dt(2000,01,01),dt(2000,02,01)), (dt(2005,05,04,12),dt(2007,05,20,6)))
            >>> for t in timelist:
            ...   map = tgis.RasterDataset("a%i@P"%count)
            ...   check = map.set_absolute_time(t[0],t[1])
            ...   if check:
            ...     maps.append(map)
            ...   count += 1
            >>> tgis.compute_absolute_time_granularity(maps)
            '6 hours'

    """

    has_seconds = False
    has_minutes = False
    has_hours = False
    has_days = False
    has_months = False
    has_years = False

    use_seconds = False
    use_minutes = False
    use_hours = False
    use_days = False
    use_months = False
    use_years = False

    delta = []
    datetime_delta = []
    # First we compute the timedelta of the intervals
    for map in maps:
        start, end = map.get_temporal_extent_as_tuple()
        if start and end:
            delta.append(end - start)
            datetime_delta.append(compute_datetime_delta(start, end))

    # Compute the timedelta of the gaps
    for i in range(len(maps)):
        if i < len(maps) - 1:
            relation = maps[i + 1].temporal_relation(maps[i])
            if relation == "after":
                start1, end1 = maps[i].get_temporal_extent_as_tuple()
                start2, end2 = maps[i + 1].get_temporal_extent_as_tuple()
                # Gaps are between intervals, intervals and
                # points, points and points
                if end1 and start2:
                    delta.append(end1 - start2)
                    datetime_delta.append(compute_datetime_delta(end1, start2))
                if not end1 and start2:
                    delta.append(start2 - start1)
                    datetime_delta.append(compute_datetime_delta(
                        start1, start2))
    # Check what changed
    dlist = []
    for d in datetime_delta:
        if "second" in d and d["second"] > 0:
            has_seconds = True
            #print "has second"
        if "minute" in d and d["minute"] > 0:
            has_minutes = True
            #print "has minute"
        if "hour" in d and d["hour"] > 0:
            has_hours = True
            #print "has hour"
        if "day" in d and d["day"] > 0:
            has_days = True
            #print "has day"
        if "month" in d and d["month"] > 0:
            has_months = True
            #print "has month"
        if "year" in d and d["year"] > 0:
            has_years = True
            #print "has year"

    # Create a list with a single time unit only
    if has_seconds:
        for d in datetime_delta:
            if "second" in d and d["second"] > 0:
                dlist.append(d["second"])
            elif "minute" in d and d["minute"] > 0:
                dlist.append(d["minute"] * 60)
            elif "hour" in d and d["hour"] > 0:
                dlist.append(d["hour"] * 3600)
            elif "day" in d and d["day"] > 0:
                dlist.append(d["day"] * 24 * 3600)
            else:
                dlist.append(d["max_days"] * 24 * 3600)
        use_seconds = True
    elif has_minutes:
        for d in datetime_delta:
            if "minute" in d and d["minute"] > 0:
                dlist.append(d["minute"])
            elif "hour" in d and d["hour"] > 0:
                dlist.append(d["hour"] * 60)
            elif "day" in d:
                dlist.append(d["day"] * 24 * 60)
            else:
                dlist.append(d["max_days"] * 24 * 60)
        use_minutes = True
    elif has_hours:
        for d in datetime_delta:
            if "hour" in d and d["hour"] > 0:
                dlist.append(d["hour"])
            elif "day" in d and d["day"] > 0:
                dlist.append(d["day"] * 24)
            else:
                dlist.append(d["max_days"] * 24)
        use_hours = True
    elif has_days:
        for d in datetime_delta:
            if "day" in d and d["day"] > 0:
                dlist.append(d["day"])
            else:
                dlist.append(d["max_days"])
        use_days = True
    elif has_months:
        for d in datetime_delta:
            if "month" in d and d["month"] > 0:
                dlist.append(d["month"])
            elif "year" in d and d["year"] > 0:
                dlist.append(d["year"] * 12)
        use_months = True
    elif has_years:
        for d in datetime_delta:
            if "year" in d:
                dlist.append(d["year"])
        use_years = True

    dlist.sort()
    ulist = list(set(dlist))

    if len(ulist) == 0:
        return None

    if len(ulist) > 1:
        # Find greatest common divisor
        granularity = gcd_list(ulist)
    else:
        granularity = ulist[0]

    if use_seconds:
        if granularity == 1:
            return "%i second" % granularity
        else:
            return "%i seconds" % granularity
    elif use_minutes:
        if granularity == 1:
            return "%i minute" % granularity
        else:
            return "%i minutes" % granularity
    elif use_hours:
        if granularity == 1:
            return "%i hour" % granularity
        else:
            return "%i hours" % granularity
    elif use_days:
        if granularity == 1:
            return "%i day" % granularity
        else:
            return "%i days" % granularity
    elif use_months:
        if granularity == 1:
            return "%i month" % granularity
        else:
            return "%i months" % granularity
    elif use_years:
        if granularity == 1:
            return "%i year" % granularity
        else:
            return "%i years" % granularity

    return None

###############################################################################

def compute_common_relative_time_granularity(gran_list):
	"""Compute the greatest common granule from a list of relative time granules

        .. code-block:: python

            >>> import grass.temporal as tgis
            >>> tgis.init()
            >>> grans = [1,2,30]
            >>> tgis.compute_common_relative_time_granularity(grans)
            1

            >>> import grass.temporal as tgis
            >>> tgis.init()
            >>> grans = [10,20,30]
            >>> tgis.compute_common_relative_time_granularity(grans)
            10
    """
	return gcd_list(gran_list)

###############################################################################

def compute_common_absolute_time_granularity(gran_list,
                                             start_date_list = None):
    """ Compute the greatest common granule from a list of absolute time granules,
        considering the start times of the related space time datasets in the
        common granularity computation.

        The list of start dates is optional. If you use this function to compute a common
        granularity between space time datasets, then you should provide their start times
        to avoid wrong synchronization.

        :param gran_list: List of granularities
        :param start_date_list: List of the start times of related space time datasets
        :return: The common granularity

        .. code-block:: python

            >>> from datetime import datetime
            >>> import grass.temporal as tgis
            >>> tgis.init()
            >>> grans = ["20 second", "10 minutes", "2 hours"]
            >>> dates = [datetime(2001,1,1,0,0,0),
            ...          datetime(2001,1,1,0,0,0),
            ...          datetime(2001,1,1,0,0,0),]
            >>> tgis.compute_common_absolute_time_granularity(grans, dates)
            '20 seconds'

            >>> grans = ["20 second", "10 minutes", "2 hours"]
            >>> dates = [datetime(2001,1,1,0,0,20),
            ...          datetime(2001,1,1,0,0,0),
            ...          datetime(2001,1,1,0,0,0),]
            >>> tgis.compute_common_absolute_time_granularity(grans, dates)
            '1 second'

            >>> grans = ["7200 second", "240 minutes", "1 year"]
            >>> dates = [datetime(2001,1,1,0,0,10),
            ...          datetime(2001,1,1,0,0,0),
            ...          datetime(2001,1,1,0,0,0),]
            >>> tgis.compute_common_absolute_time_granularity(grans, dates)
            '1 second'

            >>> grans = ["7200 second", "89 minutes", "1 year"]
            >>> dates = [datetime(2001,1,1,0,0,0),
            ...          datetime(2001,1,1,0,0,0),
            ...          datetime(2001,1,1,0,0,0),]
            >>> tgis.compute_common_absolute_time_granularity(grans, dates)
            '60 seconds'

            >>> grans = ["120 minutes", "2 hours"]
            >>> dates = [datetime(2001,1,1,0,0,0),
            ...          datetime(2001,1,1,0,0,0),]
            >>> tgis.compute_common_absolute_time_granularity(grans, dates)
            '60 minutes'

            >>> grans = ["120 minutes", "2 hours"]
            >>> dates = [datetime(2001,1,1,0,30,0),
            ...          datetime(2001,1,1,0,0,0),]
            >>> tgis.compute_common_absolute_time_granularity(grans, dates)
            '1 minute'

            >>> grans = ["360 minutes", "3 hours"]
            >>> dates = [datetime(2001,1,1,0,0,0),
            ...          datetime(2001,1,1,0,0,0),]
            >>> tgis.compute_common_absolute_time_granularity(grans, dates)
            '60 minutes'

            >>> grans = ["2 hours", "4 hours", "8 hours"]
            >>> dates = [datetime(2001,1,1,0,0,0),
            ...          datetime(2001,1,1,0,0,0),
            ...          datetime(2001,1,1,0,0,0),]
            >>> tgis.compute_common_absolute_time_granularity(grans, dates)
            '2 hours'

            >>> grans = ["2 hours", "4 hours", "8 hours"]
            >>> dates = [datetime(2001,1,1,2,0,0),
            ...          datetime(2001,1,1,4,0,0),
            ...          datetime(2001,1,1,8,0,0),]
            >>> tgis.compute_common_absolute_time_granularity(grans, dates)
            '1 hour'

            >>> grans = ["8 hours", "2 days"]
            >>> dates = [datetime(2001,1,1,0,0,0),
            ...          datetime(2001,1,1,0,0,0),]
            >>> tgis.compute_common_absolute_time_granularity(grans, dates)
            '8 hours'

            >>> grans = ["8 hours", "2 days"]
            >>> dates = [datetime(2001,1,1,10,0,0),
            ...          datetime(2001,1,1,0,0,0),]
            >>> tgis.compute_common_absolute_time_granularity(grans, dates)
            '1 hour'

            >>> grans = ["120 months", "360 months", "4 years"]
            >>> dates = [datetime(2001,1,1,0,0,0),
            ...          datetime(2001,1,1,0,0,0),
            ...          datetime(2001,1,1,0,0,0),]
            >>> tgis.compute_common_absolute_time_granularity(grans, dates)
            '12 months'

            >>> grans = ["30 days", "10 days", "5 days"]
            >>> dates = [datetime(2001,2,1,0,0,0),
            ...          datetime(2001,1,1,0,0,0),
            ...          datetime(2001,1,1,0,0,0),]
            >>> tgis.compute_common_absolute_time_granularity(grans, dates)
            '5 days'

            >>> grans = ["30 days", "10 days", "5 days"]
            >>> dates = [datetime(2001,2,2,0,0,0),
            ...          datetime(2001,1,1,0,0,0),
            ...          datetime(2001,1,1,0,0,0),]
            >>> tgis.compute_common_absolute_time_granularity(grans, dates)
            '1 day'

            >>> grans = ["2 days", "360 months", "4 years"]
            >>> dates = [datetime(2001,1,1,0,0,0),
            ...          datetime(2001,1,1,0,0,0),
            ...          datetime(2001,1,1,0,0,0),]
            >>> tgis.compute_common_absolute_time_granularity(grans, dates)
            '2 days'

            >>> grans = ["2 days", "360 months", "4 years"]
            >>> dates = [datetime(2001,1,2,0,0,0),
            ...          datetime(2001,1,1,0,0,0),
            ...          datetime(2001,1,1,0,0,0),]
            >>> tgis.compute_common_absolute_time_granularity(grans, dates)
            '1 day'

            >>> grans = ["120 months", "360 months", "4 years"]
            >>> dates = [datetime(2001,2,1,0,0,0),
            ...          datetime(2001,1,1,0,0,0),
            ...          datetime(2001,1,1,0,0,0),]
            >>> tgis.compute_common_absolute_time_granularity(grans, dates)
            '1 month'

            >>> grans = ["120 months", "361 months", "4 years"]
            >>> dates = [datetime(2001,1,1,0,0,0),
            ...          datetime(2001,1,1,0,0,0),
            ...          datetime(2001,1,1,0,0,0),]
            >>> tgis.compute_common_absolute_time_granularity(grans, dates)
            '1 month'

            >>> grans = ["120 months", "360 months", "4 years"]
            >>> dates = [datetime(2001,1,1,0,0,0),
            ...          datetime(2001,1,1,0,0,0),
            ...          datetime(2001,1,1,0,0,0),]
            >>> tgis.compute_common_absolute_time_granularity(grans, dates)
            '12 months'

        ..

    """

    common_granule = compute_common_absolute_time_granularity_simple(gran_list)

    if start_date_list is None:
        return common_granule

    num, granule = common_granule.split()

    if granule in ["seconds",  "second"]:
        # If the start seconds are different between the start dates
        # set the granularity to one second
        for start_time in start_date_list:
            if start_time.second != start_date_list[0].second:
                return "1 second"
        # Make sure the granule does not exceed the hierarchy limit
        if int(num) > 60:
            if int(num)%60 == 0:
                return "60 seconds"
            else:
                return "1 second"

    if granule in ["minutes",  "minute"]:
        # If the start minutes are different between the start dates
        # set the granularity to one minute
        for start_time in start_date_list:
            if start_time.minute != start_date_list[0].minute:
                return "1 minute"
        # Make sure the granule does not exceed the hierarchy limit
        if int(num) > 60:
            if int(num)%60 == 0:
                return "60 minutes"
            else:
                return "1 minute"

    if granule in ["hours",  "hour"]:
        # If the start hours are different between the start dates
        # set the granularity to one hour
        for start_time in start_date_list:
            if start_time.hour != start_date_list[0].hour:
                return "1 hour"
        # Make sure the granule does not exceed the hierarchy limit
        if int(num) > 24:
            if int(num)%24 == 0:
                return "24 hours"
            else:
                return "1 hour"

    if granule in ["days",  "day"]:
        # If the start days are different between the start dates
        # set the granularity to one day
        for start_time in start_date_list:
            if start_time.day != start_date_list[0].day:
                return "1 day"
        # Make sure the granule does not exceed the hierarchy limit
        if int(num) > 365:
            if int(num)%365 == 0:
                return "365 days"
            else:
                return "1 day"

    if granule in ["months",  "month"]:
        # If the start months are different between the start dates
        # set the granularity to one month
        for start_time in start_date_list:
            if start_time.month != start_date_list[0].month:
                return "1 month"
        # Make sure the granule does not exceed the hierarchy limit
        if int(num) > 12:
            if int(num)%12 == 0:
                return "12 months"
            else:
                return "1 month"

    return common_granule

###############################################################################

def compute_common_absolute_time_granularity_simple(gran_list):
    """ Compute the greatest common granule from a list of absolute time granules

        :param gran_list: List of granularities
        :return: The common granularity

        .. code-block:: python

            >>> import grass.temporal as tgis
            >>> tgis.init()
            >>> grans = ["1 second", "2 seconds", "30 seconds"]
            >>> tgis.compute_common_absolute_time_granularity(grans)
            '1 second'

            >>> grans = ["3 second", "6 seconds", "30 seconds"]
            >>> tgis.compute_common_absolute_time_granularity(grans)
            '3 seconds'

            >>> grans = ["12 second", "18 seconds", "30 seconds", "10 minutes"]
            >>> tgis.compute_common_absolute_time_granularity(grans)
            '6 seconds'

            >>> grans = ["20 second", "10 minutes", "2 hours"]
            >>> tgis.compute_common_absolute_time_granularity(grans)
            '20 seconds'

            >>> grans = ["7200 second", "240 minutes", "1 year"]
            >>> tgis.compute_common_absolute_time_granularity(grans)
            '7200 seconds'

            >>> grans = ["7200 second", "89 minutes", "1 year"]
            >>> tgis.compute_common_absolute_time_granularity(grans)
            '60 seconds'

            >>> grans = ["10 minutes", "20 minutes", "30 minutes", "40 minutes", "2 hours"]
            >>> tgis.compute_common_absolute_time_granularity(grans)
            '10 minutes'

            >>> grans = ["120 minutes", "2 hours"]
            >>> tgis.compute_common_absolute_time_granularity(grans)
            '120 minutes'

            >>> grans = ["360 minutes", "3 hours"]
            >>> tgis.compute_common_absolute_time_granularity(grans)
            '180 minutes'

            >>> grans = ["2 hours", "4 hours", "8 hours"]
            >>> tgis.compute_common_absolute_time_granularity(grans)
            '2 hours'

            >>> grans = ["8 hours", "2 days"]
            >>> tgis.compute_common_absolute_time_granularity(grans)
            '8 hours'

            >>> grans = ["48 hours", "1 month"]
            >>> tgis.compute_common_absolute_time_granularity(grans)
            '24 hours'

            >>> grans = ["48 hours", "1 year"]
            >>> tgis.compute_common_absolute_time_granularity(grans)
            '24 hours'

            >>> grans = ["2 months", "4 months", "1 year"]
            >>> tgis.compute_common_absolute_time_granularity(grans)
            '2 months'

            >>> grans = ["120 months", "360 months", "4 years"]
            >>> tgis.compute_common_absolute_time_granularity(grans)
            '24 months'

            >>> grans = ["120 months", "361 months", "4 years"]
            >>> tgis.compute_common_absolute_time_granularity(grans)
            '1 month'

            >>> grans = ["2 years", "3 years", "4 years"]
            >>> tgis.compute_common_absolute_time_granularity(grans)
            '1 year'
    """

    has_seconds = False # 0
    has_minutes = False # 1
    has_hours = False     # 2
    has_days = False      # 3
    has_months = False  # 4
    has_years = False     # 5

    seconds = []
    minutes = []
    hours = []
    days = []
    months = []
    years = []

    min_gran = 6
    max_gran = -1

    for entry in gran_list:
        if not check_granularity_string(entry, "absolute"):
            return False

        num,  gran = entry.split()

        if gran in ["seconds",  "second"]:
            has_seconds = True
            if min_gran > 0:
                min_gran = 0
            if max_gran < 0:
                max_gran = 0

            seconds.append(int(num))

        if gran in ["minutes",  "minute"]:
            has_minutes = True
            if min_gran > 1:
                min_gran = 1
            if max_gran < 1:
                max_gran = 1

            minutes.append(int(num))

        if gran in ["hours",  "hour"]:
            has_hours = True
            if min_gran > 2:
                min_gran = 2
            if max_gran < 2:
                max_gran = 2

            hours.append(int(num))

        if gran in ["days",  "day"]:
            has_days = True
            if min_gran > 3:
                min_gran = 3
            if max_gran < 3:
                max_gran = 3

            days.append(int(num))

        if gran in ["months",  "month"]:
            has_months = True
            if min_gran > 4:
                min_gran = 4
            if max_gran < 4:
                max_gran = 4

            months.append(int(num))

        if gran in ["years",  "year"]:
            has_years = True
            if min_gran > 5:
                min_gran = 5
            if max_gran < 5:
                max_gran = 5

            years.append(int(num))

    if has_seconds:
        if has_minutes:
            minutes.sort()
            seconds.append(minutes[0]*60)
        if has_hours:
            hours.sort()
            seconds.append(hours[0]*60*60)
        if has_days:
            days.sort()
            seconds.append(days[0]*60*60*24)
        if has_months:
            months.sort()
            seconds.append(months[0]*60*60*24*28)
            seconds.append(months[0]*60*60*24*29)
            seconds.append(months[0]*60*60*24*30)
            seconds.append(months[0]*60*60*24*31)
        if has_years:
            years.sort()
            seconds.append(years[0]*60*60*24*365)
            seconds.append(years[0]*60*60*24*366)

        num = gcd_list(seconds)
        gran = "second"
        if num > 1:
            gran += "s"
        return "%i %s"%(num,  gran)

    elif has_minutes:
        if has_hours:
            hours.sort()
            minutes.append(hours[0]*60)
        if has_days:
            days.sort()
            minutes.append(days[0]*60*24)
        if has_months:
            months.sort()
            minutes.append(months[0]*60*24*28)
            minutes.append(months[0]*60*24*29)
            minutes.append(months[0]*60*24*30)
            minutes.append(months[0]*60*24*31)
        if has_years:
            years.sort()
            minutes.append(years[0]*60*24*365)
            minutes.append(years[0]*60*24*366)
        num = gcd_list(minutes)
        gran = "minute"
        if num > 1:
            gran += "s"
        return "%i %s"%(num,  gran)

    elif has_hours:
        if has_days:
            days.sort()
            hours.append(days[0]*24)
        if has_months:
            months.sort()
            hours.append(months[0]*24*28)
            hours.append(months[0]*24*29)
            hours.append(months[0]*24*30)
            hours.append(months[0]*24*31)
        if has_years:
            years.sort()
            hours.append(years[0]*24*365)
            hours.append(years[0]*24*366)
        num = gcd_list(hours)
        gran = "hour"
        if num > 1:
            gran += "s"
        return "%i %s"%(num,  gran)

    elif has_days:
        if has_months:
            months.sort()
            days.append(months[0]*28)
            days.append(months[0]*29)
            days.append(months[0]*30)
            days.append(months[0]*31)
        if has_years:
            years.sort()
            days.append(years[0]*365)
            days.append(years[0]*366)
        num = gcd_list(days)
        gran = "day"
        if num > 1:
            gran += "s"
        return "%i %s"%(num,  gran)

    elif has_months:
        if has_years:
            years.sort()
            months.append(years[0]*12)
        num = gcd_list(months)
        gran = "month"
        if num > 1:
            gran += "s"
        return "%i %s"%(num,  gran)

    elif has_years:
        num = gcd_list(years)
        gran = "year"
        if num > 1:
            gran += "s"
        return "%i %s"%(num,  gran)

#######################################################################

def gran_singular_unit(gran):
    """Return the absolute granularity unit in its singular term

    :param gran: input granularity
    :return: granularity unit

    .. code-block:: python

        >>> import grass.temporal as tgis
        >>> tgis.init()
        >>> tgis.gran_singular_unit('1 month')
        'month'

        >>> tgis.gran_singular_unit('2 months')
        'month'

        >>> tgis.gran_singular_unit('6 seconds')
        'second'

        >>> tgis.gran_singular_unit('1 year')
        'year'
    """
    if check_granularity_string(gran, 'absolute'):
        output, unit = gran.split(" ")
        if unit in PLURAL_GRAN:
            return unit[:-1]
        elif unit in SINGULAR_GRAN:
            return unit
        else:
            lists = "{gr}".format(gr=SUPPORTED_GRAN).replace('[',
                                                             '').replace(']',
                                                                         '')
            print(_("Output granularity seems not to be valid. Please use "
                    "one of the following values : {gr}".format(gr=lists)))
            return False
    else:
        print(_("Invalid absolute granularity"))
        return False


#######################################################################

def gran_plural_unit(gran):
    """Return the absolute granularity unit in its singular term

    :param gran: input granularity
    :return: granularity unit

    .. code-block:: python

        >>> import grass.temporal as tgis
        >>> tgis.init()
        >>> tgis.gran_singular_unit('1 month')
        'month'

        >>> tgis.gran_singular_unit('2 months')
        'month'

        >>> tgis.gran_singular_unit('6 seconds')
        'second'

        >>> tgis.gran_singular_unit('1 year')
        'year'
    """
    if check_granularity_string(gran, 'absolute'):
        output, unit = gran.split(" ")
        if unit in PLURAL_GRAN:
            return unit
        elif unit in SINGULAR_GRAN:
            return "{gr}s".format(gr=unit)
        else:
            lists = "{gr}".format(gr=SUPPORTED_GRAN).replace('[',
                                                             '').replace(']',
                                                                         '')
            print(_("Output granularity seems not to be valid. Please use "
                    "one of the following values : {gr}".format(gr=lists)))
    else:
        print(_("Invalid absolute granularity"))
        return False

########################################################################

def gran_to_gran(from_gran, to_gran="days", shell=False):
    """Converts the computed absolute granularity of a STDS to a smaller
       granularity based on the Gregorian calendar hierarchy that 1 year
       equals 12 months or 365.2425 days or 24 * 365.2425 hours or 86400 *
       365.2425 seconds.

       :param from_gran: input granularity, this should be bigger than to_gran
       :param to_gran: output granularity
       :return: The output granularity

       .. code-block:: python

           >>> import grass.temporal as tgis
           >>> tgis.init()
           >>> tgis.gran_to_gran('1 month', '1 day')
           '30.436875 days'

           >>> tgis.gran_to_gran('1 month', '1 day', True)
           30.436875

           >>> tgis.gran_to_gran('10 year', '1 hour')
           '87658.2 hours'

           >>> tgis.gran_to_gran('10 year', '1 minute')
           '5259492.0 minutes'

           >>> tgis.gran_to_gran('6 months', '1 day')
           '182.62125 days'

           >>> tgis.gran_to_gran('1 months', '1 second')
           '2629746.0 seconds'

           >>> tgis.gran_to_gran('1 month', '1 second', True)
           2629746.0

           >>> tgis.gran_to_gran('30 month', '1 month', True)
           30
    """
    def _return(output, tounit, shell):
        """Fuction to return the output"""
        if shell:
            return output
        else:
            if output == 1:
                return "{val} {unit}".format(val=output, unit=tounit)
            else:
                return "{val} {unit}s".format(val=output, unit=tounit)

    #TODO check the leap second
    if check_granularity_string(from_gran, 'absolute'):
        output, unit = from_gran.split(" ")
        if unit in PLURAL_GRAN:
            unit = unit[:-1]
        myunit = unit
        tounit = gran_singular_unit(to_gran)

        output = ast.literal_eval(output)
        for k, v in CONVERT_GRAN.items():
            if myunit == tounit:
                return _return(output, tounit, shell)
            if k == myunit:
                num, myunit = v.split(" ")
                output = output * ast.literal_eval(num)
            if tounit == 'second' and myunit == tounit:
                return _return(output, tounit, shell)
        print(_("Probably you need to invert 'from_gran' and 'to_gran'"))
        return False
    else:
        print(_("Invalid absolute granularity"))
        return False


###############################################################################
# http://akiscode.com/articles/gcd_of_a_list.shtml
# Copyright (c) 2010 Stephen Akiki
# MIT License (Means you can do whatever you want with this)
#  See http://www.opensource.org/licenses/mit-license.php
# Error Codes:
#   None


def gcd(a, b):
    """The Euclidean Algorithm """
    a = abs(a)
    b = abs(b)
    while a:
        a, b = b % a, a
    return b

###############################################################################


def gcd_list(list):
    """Finds the GCD of numbers in a list.

    :param list: List of numbers you want to find the GCD of
                 E.g. [8, 24, 12]
    :return: GCD of all numbers

    """
    return reduce(gcd, list)

###############################################################################

if __name__ == "__main__":
    import doctest
    doctest.testmod()
