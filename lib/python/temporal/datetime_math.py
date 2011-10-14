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

def datetime_delta_to_double(dt1, dt2):
    """Compute the the dfference dt2 - dt1 and convert the time delta into a 
       double value, representing days.
    """

    delta = dt2 - dt1

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

def test_increment_datetime_by_string():

    # First test
    print "# Test 1"
    dt = datetime(2001, 9, 1, 0, 0, 0)
    string = "60 seconds, 4 minutes, 12 hours, 10 days, 1 weeks, 5 months, 1 years"

    dt1 = datetime(2003,2,18,12,5,0)
    dt2 = increment_datetime_by_string(dt, string)

    print dt
    print dt2

    delta = dt1 -dt2

    if delta.days != 0 or delta.seconds != 0:
        core.error("increment computation is wrong %s" % (delta))

    # Second test
    print "# Test 2"
    dt = datetime(2001, 11, 1, 0, 0, 0)
    string = "1 months"

    dt1 = datetime(2001,12,1)
    dt2 = increment_datetime_by_string(dt, string)

    print dt
    print dt2

    delta = dt1 -dt2

    if delta.days != 0 or delta.seconds != 0:
        core.error("increment computation is wrong %s" % (delta))

    # Third test
    print "# Test 3"
    dt = datetime(2001, 11, 1, 0, 0, 0)
    string = "13 months"

    dt1 = datetime(2002,12,1)
    dt2 = increment_datetime_by_string(dt, string)

    print dt
    print dt2

    delta = dt1 -dt2

    if delta.days != 0 or delta.seconds != 0:
        core.error("increment computation is wrong %s" % (delta))

    # 4. test
    print "# Test 4"
    dt = datetime(2001, 1, 1, 0, 0, 0)
    string = "72 months"

    dt1 = datetime(2007,1,1)
    dt2 = increment_datetime_by_string(dt, string)

    print dt
    print dt2

    delta = dt1 -dt2

    if delta.days != 0 or delta.seconds != 0:
        core.error("increment computation is wrong %s" % (delta))

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


def test_adjust_datetime_to_granularity():

    # First test
    print "Test 1"
    dt = datetime(2001, 8, 8, 12,30,30)
    result = adjust_datetime_to_granularity(dt, "5 seconds")
    correct =  datetime(2001, 8, 8, 12,30,30)

    delta = correct - result 

    if delta.days != 0 or delta.seconds != 0:
        core.error("Granularity adjustment computation is wrong %s" % (delta))

    # Second test
    print "Test 2"
    result = adjust_datetime_to_granularity(dt, "20 minutes")
    correct =  datetime(2001, 8, 8, 12,30,00)

    delta = correct - result 

    if delta.days != 0 or delta.seconds != 0:
        core.error("Granularity adjustment computation is wrong %s" % (delta))

    # Third test
    print "Test 2"
    result = adjust_datetime_to_granularity(dt, "20 minutes")
    correct =  datetime(2001, 8, 8, 12,30,00)

    delta = correct - result 

    if delta.days != 0 or delta.seconds != 0:
        core.error("Granularity adjustment computation is wrong %s" % (delta))

    # 4. test
    print "Test 4"
    result = adjust_datetime_to_granularity(dt, "3 hours")
    correct =  datetime(2001, 8, 8, 12,00,00)

    delta = correct - result 

    if delta.days != 0 or delta.seconds != 0:
        core.error("Granularity adjustment computation is wrong %s" % (delta))

    # 5. test
    print "Test 5"
    result = adjust_datetime_to_granularity(dt, "5 days")
    correct =  datetime(2001, 8, 8, 00,00,00)

    delta = correct - result 

    if delta.days != 0 or delta.seconds != 0:
        core.error("Granularity adjustment computation is wrong %s" % (delta))

    # 6. test
    print "Test 6"
    result = adjust_datetime_to_granularity(dt, "2 weeks")
    correct =  datetime(2001, 8, 6, 00,00,00)

    delta = correct - result 

    if delta.days != 0 or delta.seconds != 0:
        core.error("Granularity adjustment computation is wrong %s" % (delta))

    # 7. test
    print "Test 7"
    result = adjust_datetime_to_granularity(dt, "6 months")
    correct =  datetime(2001, 8, 1, 00,00,00)

    delta = correct - result 

    if delta.days != 0 or delta.seconds != 0:
        core.error("Granularity adjustment computation is wrong %s" % (delta))

    # 8. test
    print "Test 8"
    result = adjust_datetime_to_granularity(dt, "2 years")
    correct =  datetime(2001, 1, 1, 00,00,00)

    delta = correct - result 

    if delta.days != 0 or delta.seconds != 0:
        core.error("Granularity adjustment computation is wrong %s" % (delta))

    # 9. test
    print "Test 9"
    result = adjust_datetime_to_granularity(dt, "2 years, 3 months, 5 days, 3 hours, 3 minutes, 2 seconds")
    correct =  datetime(2001, 8, 8, 12,30,30)

    delta = correct - result 

    if delta.days != 0 or delta.seconds != 0:
        core.error("Granularity adjustment computation is wrong %s" % (delta))

    # 10. test
    print "Test 10"
    result = adjust_datetime_to_granularity(dt, "3 months, 5 days, 3 minutes")
    correct =  datetime(2001, 8, 8, 12,30,00)

    delta = correct - result 

    if delta.days != 0 or delta.seconds != 0:
        core.error("Granularity adjustment computation is wrong %s" % (delta))

    # 11. test
    print "Test 11"
    result = adjust_datetime_to_granularity(dt, "3 weeks, 5 days")
    correct =  datetime(2001, 8, 8, 00,00,00)

    delta = correct - result 

    if delta.days != 0 or delta.seconds != 0:
        core.error("Granularity adjustment computation is wrong %s" % (delta))










