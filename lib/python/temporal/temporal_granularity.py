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

def compute_relative_time_granularity(maps):            
    granularity = None

    delta = []
    # First we compute the timedelta of the intervals
    for map in maps:
        start, end = map.get_valid_time()
        if start and end:
            t =  relative_time_to_time_delta(abs(end - start))
            full_seconds = t.days * 86400 + t.seconds
            delta.append(full_seconds)

    # Compute the timedelta of the gaps
    for i in range(len(maps)):
        if i < len(maps) - 1:
            relation = maps[i + 1].temporal_relation(maps[i])
            if relation == "after":
                start1, end1 = maps[i].get_valid_time()
                start2, end2 = maps[i + 1].get_valid_time()
                # Gaps are between intervals, intervals and points, points and points
                if end1 and start2:
                    t =  relative_time_to_time_delta(abs(end1 - start2))
                    full_seconds = t.days * 86400 + t.seconds
                    delta.append(full_seconds)
                if  not end1 and start2:
                    t =  relative_time_to_time_delta(abs(start1 - start2))
                    full_seconds = t.days * 86400 + t.seconds
                    delta.append(full_seconds)

    delta.sort()
    ulist = list(set(delta))
    if len(ulist) > 1:
        # Find greatest common divisor
        granularity = gcd_list(ulist)
    else:
        granularity = ulist[0]

    return float(granularity / 86400.0)

###############################################################################

def compute_absolute_time_granularity(maps):            

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
        start, end = map.get_valid_time()
        if start and end:
            delta.append(end - start)
            datetime_delta.append(compute_datetime_delta(start, end))

    # Compute the timedelta of the gaps
    for i in range(len(maps)):
        if i < len(maps) - 1:
            relation = maps[i + 1].temporal_relation(maps[i])
            if relation == "after":
                start1, end1 = maps[i].get_valid_time()
                start2, end2 = maps[i + 1].get_valid_time()
                # Gaps are between intervals, intervals and points, points and points
                if end1 and start2:
                    delta.append(end1 - start2)
                    datetime_delta.append(compute_datetime_delta(end1, start2))
                if  not end1 and start2:
                    delta.append(start2 - start1)
                    datetime_delta.append(compute_datetime_delta(start1, start2))

    # Check what changed
    dlist = []
    for d in datetime_delta:
        if d.has_key("second") and d["second"] > 0:
            has_seconds = True
        if d.has_key("minute") and d["minute"] > 0:
            has_minutes = True
        if d.has_key("hour") and d["hour"] > 0:
            has_hours = True
        if d.has_key("day") and d["day"] > 0:
            has_days = True
        if d.has_key("month") and d["month"] > 0:
            has_months = True
        if d.has_key("year") and d["year"] > 0:
            has_years = True

    # Create a list with a single time unit only
    if has_seconds:
        for d in datetime_delta:
            if d.has_key("second"):
                dlist.append(d["second"])   
            elif d.has_key("minute"):
                dlist.append(d["minute"] * 60)   
            elif d.has_key("hour"):
                dlist.append(d["hour"] * 3600)   
            elif d.has_key("day"):
                dlist.append(d["day"] * 24 * 3600)   
            else:
                dlist.append(d["max_days"] * 24 * 3600)   
        use_seconds = True        
    elif has_minutes:
        for d in datetime_delta:
            if d.has_key("minute"):
                dlist.append(d["minute"])   
            elif d.has_key("hour"):
                dlist.append(d["hour"] * 60)   
            elif d.has_key("day"):
                dlist.append(d["day"] * 24 * 60)   
            else:
                dlist.append(d["max_days"] * 24 * 60)   
        use_minutes = True        
    elif has_hours:
        for d in datetime_delta:
            if d.has_key("hour"):
                dlist.append(d["hour"])   
            elif d.has_key("day"):
                dlist.append(d["day"] * 24)   
            else:
                dlist.append(d["max_days"] * 24)   
        use_hours = True        
    elif has_days:
        for d in datetime_delta:
            if d.has_key("day"):
                dlist.append(d["day"])   
            else:
                dlist.append(d["max_days"])   
        use_days = True        
    elif has_months:
        for d in datetime_delta:
            if d.has_key("month"):
                dlist.append(d["month"])   
            elif d.has_key("year"):
                dlist.append(d["year"] * 12)   
        use_months = True        
    elif has_years:
        for d in datetime_delta:
            if d.has_key("year"):
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
        return "%i seconds" % granularity
    elif use_minutes:
        return "%i minutes" % granularity
    elif use_hours:
        return "%i hours" % granularity
    elif use_days:
        return "%i days" % granularity
    elif use_months:
        return "%i months" % granularity
    elif use_years:
        return "%i years" % granularity

    return None

###############################################################################
# http://akiscode.com/articles/gcd_of_a_list.shtml
# Copyright (c) 2010 Stephen Akiki
# MIT License (Means you can do whatever you want with this)
#  See http://www.opensource.org/licenses/mit-license.php
# Error Codes:
#   None
def gcd(a,b):
	""" The Euclidean Algorithm """
	a = abs(a)
	b = abs(b)
        while a:
                a, b = b%a, a
        return b
        
###############################################################################

def gcd_list(list):
	""" Finds the GCD of numbers in a list.
	Input: List of numbers you want to find the GCD of
		E.g. [8, 24, 12]
	Returns: GCD of all numbers
	"""
	return reduce(gcd, list)
