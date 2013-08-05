"""!@package grass.temporal

@brief GRASS Python scripting module (temporal GIS functions)

Temporal GIS related functions to be used in temporal GIS Python library package.

Usage:

@code
import grass.temporal as tgis

tgis.compute_relative_time_granularity(maps)
...
@endcode

(C) 2008-2011 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Soeren Gebbert
"""
from abstract_dataset import *
from datetime_math import *

###############################################################################

def check_granularity_string(granularity, temporal_type):
    """!Check if the granularity string is valid

        @param granularity The granularity string
        @param temporal_type The temporal type of the granularity relative or absolute
        @return True if valid, False if invalid

        @code

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

        @endcode
    """
    temporal_type

    if granularity is None:
        return False

    if temporal_type == "absolute":
        try:
            num, unit = granularity.split(" ")
        except:
            return False
        if unit not in ["second", "seconds", "minute", "minutes", "hour",
                        "hours", "day", "days", "week", "weeks", "month",
                        "months", "year", "years"]:
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
    """!Compute the relative time granularity

        Attention: The computation of the granularity
        is only correct in case of not overlapping intervals.
        Hence a correct temporal topology is required for computation.

        @param maps a ordered by start_time list of map objects
        @return An integer


        @code

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

        @endcode
    """

    # The interval time must be scaled to days resolution
    granularity = None

    delta = []
    # First we compute the timedelta of the intervals
    for map in maps:
        start, end = map.get_temporal_extent_as_tuple()
        if start and end:
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
    """!Compute the absolute time granularity

        Attention: The computation of the granularity
        is only correct in case of not overlapping intervals.
        Hence a correct temporal topology is required for computation.

        The computed granularity is returned as number of seconds or minutes or hours
        or days or months or years.

        @param maps a ordered by start_time list of map objects
        @return The temporal topology as string "integer unit"

        @code

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

        @endcode
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
# http://akiscode.com/articles/gcd_of_a_list.shtml
# Copyright (c) 2010 Stephen Akiki
# MIT License (Means you can do whatever you want with this)
#  See http://www.opensource.org/licenses/mit-license.php
# Error Codes:
#   None


def gcd(a, b):
    """!The Euclidean Algorithm """
    a = abs(a)
    b = abs(b)
    while a:
        a, b = b % a, a
    return b

###############################################################################


def gcd_list(list):
    """!Finds the GCD of numbers in a list.
    Input: List of numbers you want to find the GCD of
            E.g. [8, 24, 12]
    Returns: GCD of all numbers
    """
    return reduce(gcd, list)

###############################################################################

if __name__ == "__main__":
    import doctest
    doctest.testmod()
