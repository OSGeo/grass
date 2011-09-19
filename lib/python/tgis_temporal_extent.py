"""!@package grass.script.tgis_temporal_extent

@brief GRASS Python scripting module (temporal GIS functions)

Temporal GIS related temporal extent functions to be used in Python scripts and tgis packages.

Usage:

@code
from grass.script import tgis_temporal_extent as grass

grass.raster_temporal_extent()
...
@endcode

(C) 2008-2011 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Soeren Gebbert
"""
from tgis_base import *

###############################################################################

class absolute_temporal_extent(sql_database_interface):
    """This is the absolute time base class for all maps and spacetime datasets"""
    def __init__(self, table=None, ident=None, start_time=None, end_time=None, timezone=None):

	sql_database_interface.__init__(self, table, ident)

	self.set_id(ident)
	self.set_start_time(start_time)
	self.set_end_time(end_time)
	self.set_timezone(timezone)

    def starts(self, map):
	"""Return True if this absolute time object starts at the start of the provided absolute time object and finishes within it
	   A  |-----|
	   B  |---------|
	"""
	if self.D["start_time"] == map.D["start_time"] and self.D["end_time"] < map.D["end_time"]:
	    return True
        else:
	    return False

    def started(self, map):
	"""Return True if this absolute time object is started at the start of the provided absolute time object
	   A  |---------|
	   B  |-----|
	"""
	if self.D["start_time"] == map.D["start_time"] and self.D["end_time"] > map.D["end_time"]:
	    return True
        else:
	    return False

    def finishes(self, map):
	"""Return True if this absolute time object finishes at the end and within of the provided absolute time object
	   A      |-----|
	   B  |---------|
	"""
	if self.D["end_time"] == map.D["end_time"] and  self.D["start_time"] > map.D["start_time"] :
	    return True
        else:
	    return False

    def finished(self, map):
	"""Return True if this absolute time object finished at the end of the provided absolute time object
	   A  |---------|
	   B      |-----|
	"""
	if self.D["end_time"] == map.D["end_time"] and  self.D["start_time"] < map.D["start_time"] :
	    return True
        else:
	    return False

    def after(self, map):
	"""Return True if this absolute time object is temporal located after the provided absolute time object
	   A             |---------|
	   B  |---------|
	"""
	if self.D["start_time"] > map.D["end_time"]:
	    return True
        else:
	    return False

    def before(self, map):
	"""Return True if this absolute time object is temporal located bevor the provided absolute time object
	   A  |---------|
	   B             |---------|
	"""
	if self.D["end_time"] < map.D["start_time"]:
	    return True
        else:
	    return False

    def adjacent(self, map):
	"""Return True if this absolute time object is a meeting neighbour the provided absolute time object
	   A            |---------|
	   B  |---------|
	   A  |---------|
	   B            |---------|
	"""
	if (self.D["start_time"] == map.D["end_time"]) or (self.D["end_time"] == map.D["start_time"]):
	    return True
        else:
	    return False

    def follows(self, map):
	"""Return True if this absolute time object is temporal follows the provided absolute time object
	   A            |---------|
	   B  |---------|
	"""
	if self.D["start_time"] == map.D["end_time"]:
	    return True
        else:
	    return False

    def precedes(self, map):
	"""Return True if this absolute time object is temporal precedes the provided absolute time object
	   A  |---------|
	   B            |---------|
	"""
	if self.D["end_time"] == map.D["start_time"]:
	    return True
        else:
	    return False

    def during(self, map):
	"""Return True if this absolute time object is temporal located during the provided absolute time object
	   A   |-------|
	   B  |---------|
	"""
	if self.D["start_time"] > map.D["start_time"] and self.D["end_time"] < map.D["end_time"]:
	    return True
        else:
	    return False

    def contains(self, map):
	"""Return True if this absolute time object is temporal located during the provided absolute time object
	   A  |---------|
	   B   |-------|
	"""
	if self.D["start_time"] < map.D["start_time"] and self.D["end_time"] > map.D["end_time"]:
	    return True
        else:
	    return False

    def equivalent(self, map):
	"""Return True if this absolute time object is temporal located equivalent the provided absolute time object
	   A  |---------|
	   B  |---------|
	"""
	if self.D["start_time"] == map.D["start_time"] and self.D["end_time"] == map.D["end_time"]:
	    return True
        else:
	    return False

    def overlaps(self, map):
	"""Return True if this absolute time object is temporal overlaps the provided absolute time object
           A  |---------|
	   B    |---------|
	"""
	if self.D["start_time"] < map.D["start_time"] and self.D["end_time"] < map.D["end_time"] and\
	   self.D["end_time"] > map.D["start_time"]:
	    return True
        else:
	    return False

    def overlapped(self, map):
	"""Return True if this absolute time object is temporal overlaped by the provided absolute time object
	   A    |---------|
           B  |---------|
	"""
	if self.D["start_time"] > map.D["start_time"] and self.D["end_time"] > map.D["end_time"] and\
	   self.D["start_time"] < map.D["end_time"]:
	    return True
        else:
	    return False

    def temporal_relation(self, map):
	"""Returns the temporal relation between absolute time temporal objects
	   Temporal relationsships are implemented after [Allen and Ferguson 1994 Actions and Events in Interval Temporal Logic]
	"""
	if self.equivalent(map):
	    return "equivalent"
	if self.during(map):
	    return "during"
	if self.contains(map):
	    return "contains"
	if self.overlaps(map):
	    return "overlaps"
	if self.overlapped(map):
	    return "overlapped"
	if self.after(map):
	    return "after"
	if self.before(map):
	    return "before"
	if self.starts(map):
	    return "starts"
	if self.finishes(map):
	    return "finishes"
	if self.started(map):
	    return "started"
	if self.finished(map):
	    return "finished"
	if self.equivalent(map):
	    return "equivalent"
	if self.follows(map):
	    return "follows"
	if self.precedes(map):
	    return "precedes"
        return None

    def set_id(self, ident):
	"""Convenient method to set the unique identifier (primary key)"""
	self.ident = ident
	self.D["id"] = ident

    def set_start_time(self, start_time):
	"""Set the valid start time of the map, this should be of type datetime"""
	self.D["start_time"] = start_time

    def set_end_time(self, end_time):
	"""Set the valid end time of the map, this should be of type datetime"""
	self.D["end_time"] = end_time

    def set_timezone(self, timezone):
	"""Set the timezone of the map, integer from 1 - 24"""
	self.D["timezone"] = timezone

    def get_id(self):
	"""Convenient method to get the unique identifier (primary key)
	   @return None if not found
	"""
	if self.D.has_key("id"):
	    return self.D["id"]
        else:
	    return None

    def get_start_time(self):
	"""Get the valid start time of the map
	   @return None if not found"""
	if self.D.has_key("start_time"):
	    return self.D["start_time"]
        else:
	    return None

    def get_end_time(self):
	"""Get the valid end time of the map
	   @return None if not found"""
	if self.D.has_key("end_time"):
	    return self.D["end_time"]
        else:
	    return None

    def get_timezone(self):
	"""Get the timezone of the map
	   @return None if not found"""
	if self.D.has_key("timezone"):
	    return self.D["timezone"]
        else:
	    return None

    def print_info(self):
        """Print information about this class in human readable style"""
        #      0123456789012345678901234567890
        print " +-------------------- Absolute time -----------------------------------------+"
        print " | Start time:................. " + str(self.get_start_time())
        print " | End time:................... " + str(self.get_end_time())
        print " | Timezone:................... " + str(self.get_timezone())

    def print_shell_info(self):
        """Print information about this class in shell style"""
        print "start_time=" + str(self.get_start_time())
        print "end_time=" + str(self.get_end_time())
        print "timezone=" + str(self.get_timezone())

###############################################################################

class raster_absolute_time(absolute_temporal_extent):
    def __init__(self, ident=None, start_time=None, end_time=None, timezone=None):
        absolute_temporal_extent.__init__(self, "raster_absolute_time", ident, start_time, end_time, timezone)

class raster3d_absolute_time(absolute_temporal_extent):
    def __init__(self, ident=None, start_time=None, end_time=None, timezone=None):
        absolute_temporal_extent.__init__(self, "raster3d_absolute_time", ident, start_time, end_time, timezone)

class vector_absolute_time(absolute_temporal_extent):
    def __init__(self, ident=None, start_time=None, end_time=None, timezone=None):
        absolute_temporal_extent.__init__(self, "vector_absolute_time", ident, start_time, end_time, timezone)

###############################################################################

class stds_absolute_time(absolute_temporal_extent):
    def __init__(self, table=None, ident=None, start_time=None, end_time=None, granularity=None, timezone=None):
        absolute_temporal_extent.__init__(self, table, ident, start_time, end_time, timezone)

	self.set_granularity(granularity)

    def set_granularity(self, granularity):
	"""Set the granularity of the space time dataset"""
	self.D["granularity"] = granularity

    def get_granularity(self):
	"""Get the granularity of the space time dataset
	   @return None if not found"""
	if self.D.has_key("granularity"):
	    return self.D["granularity"]
        else:
	    return None

    def print_info(self):
        """Print information about this class in human readable style"""
        absolute_temporal_extent.print_info(self)
        #      0123456789012345678901234567890
        print " | Granularity:................ " + str(self.get_granularity())

    def print_shell_info(self):
        """Print information about this class in shell style"""
        absolute_temporal_extent.print_shell_info(self)
        print "granularity=" + str(self.get_granularity())

###############################################################################

class strds_absolute_time(stds_absolute_time):
    def __init__(self, ident=None, start_time=None, end_time=None, granularity=None, timezone=None):
        stds_absolute_time.__init__(self, "strds_absolute_time", ident, start_time, end_time, granularity, timezone)

class str3ds_absolute_time(stds_absolute_time):
    def __init__(self, ident=None, start_time=None, end_time=None, granularity=None, timezone=None):
        stds_absolute_time.__init__(self, "str3ds_absolute_time", ident, start_time, end_time, granularity, timezone)

class stvds_absolute_time(stds_absolute_time):
    def __init__(self, ident=None, start_time=None, end_time=None, granularity=None, timezone=None):
        stds_absolute_time.__init__(self, "stvds_absolute_time", ident, start_time, end_time, granularity, timezone)

###############################################################################

class relative_temporal_extent(sql_database_interface):
    """This is the relative time base class for all maps and spacetime datasets"""
    def __init__(self, table=None, ident=None, interval=None):

	sql_database_interface.__init__(self, table, ident)

	self.set_id(ident)
	self.set_interval(interval)

    def after(self, map):
	"""Return True if this relative time object is temporal located after the provided relative time object
	   A   |
	   B  |
	"""
	if self.D["interval"] > map.D["interval"]:
	    return True
        else:
	    return False


    def before(self, map):
	"""Return True if this relative time object is temporal located bevor the provided relative time object
	   A  |
	   B   |
	"""
	if self.D["interval"] < map.D["interval"]:
	    return True
        else:
	    return False

    def equivalent(self, map):
	"""Return True if this relative time object is equivalent to the provided relative time object
	   A  |
	   B  |
	"""
	if self.D["interval"] == map.D["interval"]:
	    return True
        else:
	    return False

    def temporal_relation(self, map):
	"""Returns the temporal relation between relative time temporal objects
	"""
	if self.equivalent(map):
	    return "equivalent"
	if self.after(map):
	    return "after"
	if self.before(map):
	    return "before"
        return None

    def set_id(self, ident):
	"""Convenient method to set the unique identifier (primary key)"""
	self.ident = ident
	self.D["id"] = ident

    def set_interval(self, interval):
	"""Set the valid interval time of the map, this should be of type datetime"""
	self.D["interval"] = interval

    def get_id(self):
	"""Convenient method to get the unique identifier (primary key)
	   @return None if not found
	"""
	if self.D.has_key("id"):
	    return self.D["id"]
        else:
	    return None

    def get_interval(self):
	"""Get the valid interval time of the map
	   @return None if not found"""
	if self.D.has_key("interval"):
	    return self.D["interval"]
        else:
	    return None

    def print_info(self):
        """Print information about this class in human readable style"""
        #      0123456789012345678901234567890
        print " +-------------------- Relative time -----------------------------------------+"
        print " | Interval:................... " + str(self.get_interval())

    def print_shell_info(self):
        """Print information about this class in shell style"""
        print "interval=" + str(self.get_interval())


###############################################################################

class raster_relative_time(relative_temporal_extent):
    def __init__(self, ident=None, interval=None):
        relative_temporal_extent.__init__(self, "raster_relative_time", ident, interval)

class raster3d_relative_time(relative_temporal_extent):
    def __init__(self, ident=None, interval=None):
        relative_temporal_extent.__init__(self, "raster3d_relative_time", ident, interval)

class vector_relative_time(relative_temporal_extent):
    def __init__(self, ident=None, interval=None):
        relative_temporal_extent.__init__(self, "vector_relative_time", ident, interval)
        
###############################################################################

class stds_relative_time(relative_temporal_extent):
    def __init__(self, table=None, ident=None, interval=None, granularity=None):
        relative_temporal_extent.__init__(self, table, ident, interval)

	self.set_granularity(granularity)

    def set_granularity(self, granularity):
	"""Set the granularity of the space time dataset"""
	self.D["granularity"] = granularity

    def get_granularity(self):
	"""Get the granularity of the space time dataset
	   @return None if not found"""
	if self.D.has_key("granularity"):
	    return self.D["granularity"]
        else:
	    return None

    def print_info(self):
        """Print information about this class in human readable style"""
        relative_temporal_extent.print_info(self)
        #      0123456789012345678901234567890
        print " | Granularity:................ " + str(self.get_granularity())

    def print_shell_info(self):
        """Print information about this class in shell style"""
        relative_temporal_extent.print_shell_info(self)
        print "granularity=" + str(self.get_granularity())

###############################################################################

class strds_relative_time(stds_relative_time):
    def __init__(self, ident=None, interval=None, granularity=None):
        stds_relative_time.__init__(self, "strds_relative_time", ident, interval, granularity)

class str3ds_relative_time(stds_relative_time):
    def __init__(self, ident=None, interval=None, granularity=None):
        stds_relative_time.__init__(self, "str3ds_relative_time", ident, interval, granularity)

class stvds_relative_time(stds_relative_time):
    def __init__(self, ident=None, interval=None, granularity=None):
        stds_relative_time.__init__(self, "stvds_relative_time", ident, interval, granularity)

