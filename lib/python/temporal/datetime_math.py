"""!@package grass.temporal

@brief GRASS Python scripting module (temporal GIS functions)

Temporal GIS datetime math functions to be used in Python sripts.

Usage:

@code
import grass.temporal as tgis

tgis.increment_datetime_by_string(mydate, "3 month, 2 hours")
...
@endcode

(C) 2008-2011 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Soeren Gebbert
"""
from datetime import datetime, date, time, timedelta
import grass.script.core as core
import copy


###############################################################################

def relative_time_to_time_delta(value):
    """Convert the double value representing days
       into a timedelta object.
    """


    days = int(value)
    seconds = value % 1
    seconds = round(seconds * 86400)

    return timedelta(days, seconds)

###############################################################################

def time_delta_to_relative_time(delta):
    """Convert the time delta into a 
       double value, representing days.
    """

    return float(delta.days) + float(delta.seconds/86400.0)

###############################################################################

def increment_datetime_by_string(mydate, increment, mult = 1):
    """Return a new datetime object incremented with the provided relative dates specified as string.
       Additional a multiplier can be specified to multiply the increment bevor adding to the provided datetime object.

       @param mydate A datetime object to incremented
       @param increment A string providing increment information:
                  The string may include comma separated values of type seconds, minutes, hours, days, weeks, months and years
                  Example: Increment the datetime 2001-01-01 00:00:00 with "60 seconds, 4 minutes, 12 hours, 10 days, 1 weeks, 5 months, 1 years"
                  will result in the datetime 2003-02-18 12:05:00
       @param mult A multiplier, default is 1
    """

    if increment:

        seconds = 0
        minutes = 0
        hours = 0
        days = 0
        weeks = 0
        months = 0
        years = 0

        inclist = []
        # Split the increment string
        incparts = increment.split(",")
        for incpart in incparts:
            inclist.append(incpart.strip().split(" "))

        for inc in inclist:
            if inc[1].find("seconds") >= 0:
                seconds = mult * int(inc[0])
            elif inc[1].find("minutes") >= 0:
                minutes = mult * int(inc[0])
            elif inc[1].find("hours") >= 0:
                hours = mult * int(inc[0])
            elif inc[1].find("days") >= 0:
                days = mult * int(inc[0])
            elif inc[1].find("weeks") >= 0:
                weeks = mult * int(inc[0])
            elif inc[1].find("months") >= 0:
                months = mult * int(inc[0])
            elif inc[1].find("years") >= 0:
                years = mult * int(inc[0])
            else:
                core.error(_("Wrong increment format: %s") % (increment))
                return None

        return increment_datetime(mydate, years, months, weeks, days, hours, minutes, seconds)
    
    return mydate

###############################################################################

def increment_datetime(mydate, years=0, months=0, weeks=0, days=0, hours=0, minutes=0, seconds=0):
    """Return a new datetime object incremented with the provided relative dates and times"""

    tdelta_seconds = timedelta(seconds=seconds)
    tdelta_minutes = timedelta(minutes=minutes)
    tdelta_hours = timedelta(hours=hours)
    tdelta_days = timedelta(days=days)
    tdelta_weeks = timedelta(weeks=weeks)
    tdelta_months = timedelta(0)
    tdelta_years = timedelta(0)


    if months > 0:
        # Compute the actual number of days in the month to add as timedelta
        year = mydate.year
        month = mydate.month

        all_months = int(months) + int(month)
        years_to_add = int(all_months/12.001)
        residual_months = all_months - (years_to_add * 12)

        # Make a deep copy of the datetime object
        dt1 = copy.copy(mydate)

        # Make sure the montha starts with a 1
        if residual_months == 0:
            residual_months = 1

        dt1 = dt1.replace(year = year + years_to_add, month = residual_months)
        tdelta_months = dt1 - mydate

    if years > 0:
        # Make a deep copy of the datetime object
        dt1 = copy.copy(mydate)
        # Compute the number of days
        dt1 = dt1.replace(year=mydate.year + int(years))
        tdelta_years = dt1 - mydate

    return mydate + tdelta_seconds + tdelta_minutes + tdelta_hours + \
                    tdelta_days + tdelta_weeks + tdelta_months + tdelta_years

###############################################################################

def adjust_datetime_to_granularity(mydate, granularity):
    """Mofiy the datetime object to fit the given granularity    """

    if granularity:

        has_seconds = False
        has_minutes = False
        has_hours = False
        has_days = False
        has_weeks = False
        has_months = False
        has_years = False

        seconds = mydate.second
        minutes = mydate.minute
        hours = mydate.hour
        days = mydate.day
        weekday = mydate.weekday()
        months = mydate.month
        years = mydate.year

        granlist = []
        # Split the increment string
        granparts = granularity.split(",")
        for granpart in granparts:
            granlist.append(granpart.strip().split(" "))

        for inc in granlist:
            if inc[1].find("seconds") >= 0:
                has_seconds = True
            elif inc[1].find("minutes") >= 0:
                has_minutes = True
            elif inc[1].find("hours") >= 0:
                has_hours = True
            elif inc[1].find("days") >= 0:
                has_days = True
            elif inc[1].find("weeks") >= 0:
                has_weeks = True
            elif inc[1].find("months") >= 0:
                has_months = True
            elif inc[1].find("years") >= 0:
                has_years = True
            else:
                core.error(_("Wrong granularity format: %s") % (granularity))
                return None

        if has_seconds:
            pass          
        elif has_minutes: # Start at 0 seconds
            seconds = 0
        elif has_hours: # Start at 0 minutes and seconds
            seconds = 0
            minutes = 0
        elif has_days: # Start at 0 hours, minuts and seconds
            seconds = 0
            minutes = 0
            hours = 0
        elif has_weeks: # Start at the first day of the week (monday) at 00:00:00
            seconds = 0
            minutes = 0
            hours = 0
            days = days - weekday
        elif has_months: # Start at the first day of the month at 00:00:00
            seconds = 0
            minutes = 0
            hours = 0
            days = 1
        elif has_years: # Start at the first day of the first month at 00:00:00
            seconds = 0
            minutes = 0
            hours = 0
            days = 1
            months = 1

        dt = copy.copy(mydate)
        result = dt.replace(year=years, month=months, day=days, hour=hours, minute=minutes, second=seconds)
        core.verbose(_("Adjust datetime from %s to %s with granularity %s") % (dt, result, granularity))

        return result

###############################################################################

def compute_datetime_delta(start, end):
    """Return a dictionary with the accumulated delta in year, month, day, hour, minute and second
    
       @return A dictionary with year, month, day, hour, minute and second as keys()
    """
    comp = {}

    day_diff = (end - start).days

    comp["max_days"] = day_diff

    # Date
    # Count full years
    d = end.year - start.year
    comp["year"] = d

    # Count full months
    if start.month == 1 and end.month == 1:
        comp["month"] = 0
    elif   start.day == 1 and end.day == 1:
        d = end.month - start.month
        if d < 0:
            d = d + 12 * comp["year"]
        elif d == 0:
            d = 12 * comp["year"]
        comp["month"] = d

    # Count full days
    if  start.day == 1 and end.day == 1:
        comp["day"] = 0
    else:
        comp["day"] = day_diff

    # Time
    # Hours
    if start.hour == 0 and end.hour == 0:
        comp["hour"] = 0
    else:
        d = end.hour - start.hour
        if d < 0:
            d = d + 24  + 24 * day_diff
        else:
            d = d + 24 * day_diff
        comp["hour"] = d
    
    # Minutes
    if start.minute == 0 and end.minute == 0:
        comp["minute"] = 0
    else:
        d = end.minute - start.minute
        if d != 0:
            if comp["hour"]:
                d = d + 60 * comp["hour"]
            else:
                d = d + 24 * 60 * day_diff
        elif d == 0:
            if comp["hour"]:
                d = 60* comp["hour"]
            else:
                d = 24 * 60 * day_diff

        comp["minute"] = d

    # Seconds
    if start.second == 0 and end.second == 0:
        comp["second"] = 0
    else:
        d = end.second - start.second
        if d != 0:
            if comp["minute"]:
                d = d + 60* comp["minute"]
            elif comp["hour"]:
                d = d + 3600* comp["hour"]
            else:
                d = d + 24 * 60 * 60 * day_diff
        elif d == 0:
            if comp["minute"]:
                d = 60* comp["minute"]
            elif comp["hour"]:
                d = 3600 * comp["hour"]
            else:
                d = 24 * 60 * 60 * day_diff
        comp["second"] = d

    return comp

