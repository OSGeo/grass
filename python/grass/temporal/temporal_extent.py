"""
Temporal extent classes

Usage:

.. code-block:: python

    >>> import grass.temporal as tgis
    >>> from datetime import datetime
    >>> tgis.init()
    >>> t = tgis.RasterRelativeTime()
    >>> t = tgis.RasterAbsoluteTime()


(C) 2012-2013 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

:authors: Soeren Gebbert
"""
from __future__ import print_function
from .base import SQLDatabaseInterface

###############################################################################


class TemporalExtent(SQLDatabaseInterface):
    """This is the abstract time base class for relative and absolute time
    objects.

    It abstract class implements the interface to absolute and relative time.
    Absolute time is represented by datetime time stamps,
    relative time is represented by a unit an integer value.

    This class implements temporal topology relationships computation
    after [Allen and Ferguson 1994 Actions and Events in Interval Temporal Logic].

    Usage:

    .. code-block:: python

        >>> init()
        >>> A = TemporalExtent(table="raster_absolute_time",
        ... ident="soil@PERMANENT", start_time=datetime(2001, 01, 01),
        ... end_time=datetime(2005,01,01) )
        >>> A.id
        'soil@PERMANENT'
        >>> A.start_time
        datetime.datetime(2001, 1, 1, 0, 0)
        >>> A.end_time
        datetime.datetime(2005, 1, 1, 0, 0)
        >>> A.print_info()
         | Start time:................. 2001-01-01 00:00:00
         | End time:................... 2005-01-01 00:00:00
        >>> A.print_shell_info()
        start_time='2001-01-01 00:00:00'
        end_time='2005-01-01 00:00:00'
        >>> # relative time
        >>> A = TemporalExtent(table="raster_absolute_time",
        ... ident="soil@PERMANENT", start_time=0, end_time=1 )
        >>> A.id
        'soil@PERMANENT'
        >>> A.start_time
        0
        >>> A.end_time
        1
        >>> A.print_info()
         | Start time:................. 0
         | End time:................... 1
        >>> A.print_shell_info()
        start_time='0'
        end_time='1'

    """

    def __init__(self, table=None, ident=None, start_time=None, end_time=None):

        SQLDatabaseInterface.__init__(self, table, ident)

        self.set_id(ident)
        self.set_start_time(start_time)
        self.set_end_time(end_time)

    def intersect(self, extent):
        """Intersect this temporal extent with the provided temporal extent and
        return a new temporal extent with the new start and end time

        :param extent: The temporal extent to intersect with
        :return: The new temporal extent with start and end time,
                or None in case of no intersection

        Usage:

        .. code-block:: python

            >>> A = TemporalExtent(start_time=5, end_time=6 )
            >>> inter = A.intersect(A)
            >>> inter.print_info()
             | Start time:................. 5
             | End time:................... 6

            >>> A = TemporalExtent(start_time=5, end_time=6 )
            >>> B = TemporalExtent(start_time=5, end_time=7 )
            >>> inter = A.intersect(B)
            >>> inter.print_info()
             | Start time:................. 5
             | End time:................... 6
            >>> inter = B.intersect(A)
            >>> inter.print_info()
             | Start time:................. 5
             | End time:................... 6

            >>> A = TemporalExtent(start_time=3, end_time=6 )
            >>> B = TemporalExtent(start_time=5, end_time=7 )
            >>> inter = A.intersect(B)
            >>> inter.print_info()
             | Start time:................. 5
             | End time:................... 6
            >>> inter = B.intersect(A)
            >>> inter.print_info()
             | Start time:................. 5
             | End time:................... 6

            >>> A = TemporalExtent(start_time=3, end_time=8 )
            >>> B = TemporalExtent(start_time=5, end_time=6 )
            >>> inter = A.intersect(B)
            >>> inter.print_info()
             | Start time:................. 5
             | End time:................... 6
            >>> inter = B.intersect(A)
            >>> inter.print_info()
             | Start time:................. 5
             | End time:................... 6

            >>> A = TemporalExtent(start_time=5, end_time=8 )
            >>> B = TemporalExtent(start_time=3, end_time=6 )
            >>> inter = A.intersect(B)
            >>> inter.print_info()
             | Start time:................. 5
             | End time:................... 6
            >>> inter = B.intersect(A)
            >>> inter.print_info()
             | Start time:................. 5
             | End time:................... 6

            >>> A = TemporalExtent(start_time=5, end_time=None )
            >>> B = TemporalExtent(start_time=3, end_time=6 )
            >>> inter = A.intersect(B)
            >>> inter.print_info()
             | Start time:................. 5
             | End time:................... None
            >>> inter = B.intersect(A)
            >>> inter.print_info()
             | Start time:................. 5
             | End time:................... None

            >>> A = TemporalExtent(start_time=5, end_time=8 )
            >>> B = TemporalExtent(start_time=3, end_time=4 )
            >>> inter = A.intersect(B)
            >>> print(inter)
            None

            >>> A = TemporalExtent(start_time=5, end_time=8 )
            >>> B = TemporalExtent(start_time=3, end_time=None )
            >>> inter = A.intersect(B)
            >>> print(inter)
            None

        """
        relation = self.temporal_relation(extent)

        if relation == "after" or relation == "before":
            return None

        if self.D["end_time"] is None:
            return TemporalExtent(start_time=self.D["start_time"])

        if extent.D["end_time"] is None:
            return TemporalExtent(start_time=extent.D["start_time"])

        start = None
        end = None

        if self.D["start_time"] > extent.D["start_time"]:
            start = self.D["start_time"]
        else:
            start = extent.D["start_time"]

        if self.D["end_time"] > extent.D["end_time"]:
            end = extent.D["end_time"]
        else:
            end = self.D["end_time"]

        if issubclass(type(self), RelativeTemporalExtent):
            return RelativeTemporalExtent(
                start_time=start, end_time=end, unit=self.get_unit()
            )
        elif issubclass(type(self), AbsoluteTemporalExtent):
            return AbsoluteTemporalExtent(start_time=start, end_time=end)
        elif issubclass(type(self), TemporalExtent):
            return TemporalExtent(start_time=start, end_time=end)

    def disjoint_union(self, extent):
        """Creates a disjoint union with this temporal extent and the provided one.
        Return a new temporal extent with the new start and end time.

        :param extent: The temporal extent to create a union with
        :return: The new temporal extent with start and end time

        Usage:

        .. code-block:: python

            >>> A = TemporalExtent(start_time=5, end_time=6 )
            >>> inter = A.intersect(A)
            >>> inter.print_info()
             | Start time:................. 5
             | End time:................... 6

            >>> A = TemporalExtent(start_time=5, end_time=6 )
            >>> B = TemporalExtent(start_time=5, end_time=7 )
            >>> inter = A.disjoint_union(B)
            >>> inter.print_info()
             | Start time:................. 5
             | End time:................... 7
            >>> inter = B.disjoint_union(A)
            >>> inter.print_info()
             | Start time:................. 5
             | End time:................... 7

            >>> A = TemporalExtent(start_time=3, end_time=6 )
            >>> B = TemporalExtent(start_time=5, end_time=7 )
            >>> inter = A.disjoint_union(B)
            >>> inter.print_info()
             | Start time:................. 3
             | End time:................... 7
            >>> inter = B.disjoint_union(A)
            >>> inter.print_info()
             | Start time:................. 3
             | End time:................... 7

            >>> A = TemporalExtent(start_time=3, end_time=8 )
            >>> B = TemporalExtent(start_time=5, end_time=6 )
            >>> inter = A.disjoint_union(B)
            >>> inter.print_info()
             | Start time:................. 3
             | End time:................... 8
            >>> inter = B.disjoint_union(A)
            >>> inter.print_info()
             | Start time:................. 3
             | End time:................... 8

            >>> A = TemporalExtent(start_time=5, end_time=8 )
            >>> B = TemporalExtent(start_time=3, end_time=6 )
            >>> inter = A.disjoint_union(B)
            >>> inter.print_info()
             | Start time:................. 3
             | End time:................... 8
            >>> inter = B.disjoint_union(A)
            >>> inter.print_info()
             | Start time:................. 3
             | End time:................... 8

            >>> A = TemporalExtent(start_time=5, end_time=None )
            >>> B = TemporalExtent(start_time=3, end_time=6 )
            >>> inter = A.disjoint_union(B)
            >>> inter.print_info()
             | Start time:................. 3
             | End time:................... 6
            >>> inter = B.disjoint_union(A)
            >>> inter.print_info()
             | Start time:................. 3
             | End time:................... 6

            >>> A = TemporalExtent(start_time=5, end_time=8 )
            >>> B = TemporalExtent(start_time=3, end_time=4 )
            >>> inter = A.disjoint_union(B)
            >>> inter.print_info()
             | Start time:................. 3
             | End time:................... 8
            >>> inter = B.disjoint_union(A)
            >>> inter.print_info()
             | Start time:................. 3
             | End time:................... 8
            >>> A = TemporalExtent(start_time=5, end_time=8 )
            >>> B = TemporalExtent(start_time=3, end_time=None )
            >>> inter = A.disjoint_union(B)
            >>> inter.print_info()
             | Start time:................. 3
             | End time:................... 8
            >>> inter = B.disjoint_union(A)
            >>> inter.print_info()
             | Start time:................. 3
             | End time:................... 8
            >>> A = TemporalExtent(start_time=5, end_time=None )
            >>> B = TemporalExtent(start_time=3, end_time=8 )
            >>> inter = A.disjoint_union(B)
            >>> inter.print_info()
             | Start time:................. 3
             | End time:................... 8
            >>> inter = B.disjoint_union(A)
            >>> inter.print_info()
             | Start time:................. 3
             | End time:................... 8
            >>> A = TemporalExtent(start_time=5, end_time=None )
            >>> B = TemporalExtent(start_time=3, end_time=None )
            >>> inter = A.disjoint_union(B)
            >>> inter.print_info()
             | Start time:................. 3
             | End time:................... 5
            >>> inter = B.disjoint_union(A)
            >>> inter.print_info()
             | Start time:................. 3
             | End time:................... 5

            >>> A = RelativeTemporalExtent(start_time=5, end_time=None, unit="years" )
            >>> B = RelativeTemporalExtent(start_time=3, end_time=None, unit="years" )
            >>> inter = A.disjoint_union(B)
            >>> inter.print_info()
             +-------------------- Relative time -----------------------------------------+
             | Start time:................. 3
             | End time:................... 5
             | Relative time unit:......... years

            >>> inter = B.disjoint_union(A)
            >>> inter.print_info()
             +-------------------- Relative time -----------------------------------------+
             | Start time:................. 3
             | End time:................... 5
             | Relative time unit:......... years


            >>> from datetime import datetime as dt
            >>> A = AbsoluteTemporalExtent(start_time=dt(2001,1,10), end_time=dt(2003,1,1))
            >>> B = AbsoluteTemporalExtent(start_time=dt(2005,1,10), end_time=dt(2008,1,1))
            >>> inter = A.disjoint_union(B)
            >>> inter.print_info()
             +-------------------- Absolute time -----------------------------------------+
             | Start time:................. 2001-01-10 00:00:00
             | End time:................... 2008-01-01 00:00:00

            >>> inter = B.disjoint_union(A)
            >>> inter.print_info()
             +-------------------- Absolute time -----------------------------------------+
             | Start time:................. 2001-01-10 00:00:00
             | End time:................... 2008-01-01 00:00:00

        """

        start = None
        end = None

        if self.D["start_time"] < extent.D["start_time"]:
            start = self.D["start_time"]
        else:
            start = extent.D["start_time"]

        # End time handling
        if self.D["end_time"] is None and extent.D["end_time"] is None:
            if self.D["start_time"] > extent.D["start_time"]:
                end = self.D["start_time"]
            else:
                end = extent.D["start_time"]
        elif self.D["end_time"] is None:
            if self.D["start_time"] > extent.D["end_time"]:
                end = self.D["start_time"]
            else:
                end = extent.D["end_time"]
        elif extent.D["end_time"] is None:
            if self.D["end_time"] > extent.D["start_time"]:
                end = self.D["end_time"]
            else:
                end = extent.D["start_time"]
        elif self.D["end_time"] < extent.D["end_time"]:
            end = extent.D["end_time"]
        else:
            end = self.D["end_time"]

        if issubclass(type(self), RelativeTemporalExtent):
            return RelativeTemporalExtent(
                start_time=start, end_time=end, unit=self.get_unit()
            )
        elif issubclass(type(self), AbsoluteTemporalExtent):
            return AbsoluteTemporalExtent(start_time=start, end_time=end)
        elif issubclass(type(self), TemporalExtent):
            return TemporalExtent(start_time=start, end_time=end)

    def union(self, extent):
        """Creates a union with this temporal extent and the provided one.
        Return a new temporal extent with the new start and end time.

        :param extent: The temporal extent to create a union with
        :return: The new temporal extent with start and end time,
                 or None in case the temporal extents are unrelated
                 (before or after)

        .. code-block:: python

            >>> A = TemporalExtent(start_time=5, end_time=8 )
            >>> B = TemporalExtent(start_time=3, end_time=4 )
            >>> inter = A.intersect(B)
            >>> print(inter)
            None

            >>> A = TemporalExtent(start_time=5, end_time=8 )
            >>> B = TemporalExtent(start_time=3, end_time=None )
            >>> inter = A.intersect(B)
            >>> print(inter)
            None

        """

        relation = self.temporal_relation(extent)

        if relation == "after" or relation == "before":
            return None

        return self.disjoint_union(extent)

    def starts(self, extent):
        """Return True if this temporal extent (A) starts at the start of the
        provided temporal extent (B) and finishes within it
        ::

            A  |-----|
            B  |---------|


        :param extent: The temporal extent object with which this extent
                       starts

        Usage:

        .. code-block:: python

            >>> A = TemporalExtent(start_time=5, end_time=6 )
            >>> B = TemporalExtent(start_time=5, end_time=7 )
            >>> A.starts(B)
            True
            >>> B.starts(A)
            False

        """
        if self.D["end_time"] is None or extent.D["end_time"] is None:
            return False

        if (
            self.D["start_time"] == extent.D["start_time"]
            and self.D["end_time"] < extent.D["end_time"]
        ):
            return True
        else:
            return False

    def started(self, extent):
        """Return True if this temporal extent (A) started at the start of the
        provided temporal extent (B) and finishes after it
        ::

            A  |---------|
            B  |-----|

        :param extent: The temporal extent object with which this extent
                       started

        Usage:

        .. code-block:: python

            >>> A = TemporalExtent(start_time=5, end_time=7 )
            >>> B = TemporalExtent(start_time=5, end_time=6 )
            >>> A.started(B)
            True
            >>> B.started(A)
            False

        """
        if self.D["end_time"] is None or extent.D["end_time"] is None:
            return False

        if (
            self.D["start_time"] == extent.D["start_time"]
            and self.D["end_time"] > extent.D["end_time"]
        ):
            return True
        else:
            return False

    def finishes(self, extent):
        """Return True if this temporal extent (A) starts after the start of
        the provided temporal extent (B) and finishes with it
        ::

            A      |-----|
            B  |---------|

        :param extent: The temporal extent object with which this extent
                       finishes

        Usage:

        .. code-block:: python

            >>> A = TemporalExtent(start_time=6, end_time=7 )
            >>> B = TemporalExtent(start_time=5, end_time=7 )
            >>> A.finishes(B)
            True
            >>> B.finishes(A)
            False

        """
        if self.D["end_time"] is None or extent.D["end_time"] is None:
            return False

        if (
            self.D["end_time"] == extent.D["end_time"]
            and self.D["start_time"] > extent.D["start_time"]
        ):
            return True
        else:
            return False

    def finished(self, extent):
        """Return True if this temporal extent (A) starts before the start of
        the provided temporal extent (B) and finishes with it
        ::

            A  |---------|
            B      |-----|

        :param extent: The temporal extent object with which this extent
                       finishes

        Usage:

        .. code-block:: python

            >>> A = TemporalExtent(start_time=5, end_time=7 )
            >>> B = TemporalExtent(start_time=6, end_time=7 )
            >>> A.finished(B)
            True
            >>> B.finished(A)
            False

        """
        if self.D["end_time"] is None or extent.D["end_time"] is None:
            return False

        if (
            self.D["end_time"] == extent.D["end_time"]
            and self.D["start_time"] < extent.D["start_time"]
        ):
            return True
        else:
            return False

    def after(self, extent):
        """Return True if this temporal extent (A) is located after the
        provided temporal extent (B)
        ::

            A             |---------|
            B  |---------|

        :param extent: The temporal extent object that is located before
                       this extent

        Usage:

        .. code-block:: python

            >>> A = TemporalExtent(start_time=8, end_time=9 )
            >>> B = TemporalExtent(start_time=6, end_time=7 )
            >>> A.after(B)
            True
            >>> B.after(A)
            False

        """
        if extent.D["end_time"] is None:
            if self.D["start_time"] > extent.D["start_time"]:
                return True
            else:
                return False

        if self.D["start_time"] > extent.D["end_time"]:
            return True
        else:
            return False

    def before(self, extent):
        """Return True if this temporal extent (A) is located before the
        provided temporal extent (B)
        ::

            A  |---------|
            B             |---------|

        :param extent: The temporal extent object that is located after
                       this extent

        Usage:

        .. code-block:: python

            >>> A = TemporalExtent(start_time=6, end_time=7 )
            >>> B = TemporalExtent(start_time=8, end_time=9 )
            >>> A.before(B)
            True
            >>> B.before(A)
            False

        """
        if self.D["end_time"] is None:
            if self.D["start_time"] < extent.D["start_time"]:
                return True
            else:
                return False

        if self.D["end_time"] < extent.D["start_time"]:
            return True
        else:
            return False

    def adjacent(self, extent):
        """Return True if this temporal extent (A) is a meeting neighbor the
        provided temporal extent (B)
        ::

            A            |---------|
            B  |---------|
            A  |---------|
            B            |---------|

        :param extent: The temporal extent object that is a meeting neighbor
                       of this extent

        Usage:

        .. code-block:: python

            >>> A = TemporalExtent(start_time=5, end_time=7 )
            >>> B = TemporalExtent(start_time=7, end_time=9 )
            >>> A.adjacent(B)
            True
            >>> B.adjacent(A)
            True
            >>> A = TemporalExtent(start_time=5, end_time=7 )
            >>> B = TemporalExtent(start_time=3, end_time=5 )
            >>> A.adjacent(B)
            True
            >>> B.adjacent(A)
            True

        """
        if self.D["end_time"] is None and extent.D["end_time"] is None:
            return False

        if (self.D["start_time"] == extent.D["end_time"]) or (
            self.D["end_time"] == extent.D["start_time"]
        ):
            return True
        else:
            return False

    def follows(self, extent):
        """Return True if this temporal extent (A) follows the
        provided temporal extent (B)
        ::

            A            |---------|
            B  |---------|

        :param extent: The temporal extent object that is the predecessor
                       of this extent

        Usage:

        .. code-block:: python

            >>> A = TemporalExtent(start_time=5, end_time=7 )
            >>> B = TemporalExtent(start_time=3, end_time=5 )
            >>> A.follows(B)
            True
            >>> B.follows(A)
            False

        """
        if extent.D["end_time"] is None:
            return False

        if self.D["start_time"] == extent.D["end_time"]:
            return True
        else:
            return False

    def precedes(self, extent):
        """Return True if this temporal extent (A) precedes the provided
        temporal extent (B)
        ::

            A  |---------|
            B            |---------|


        :param extent: The temporal extent object that is the successor
                       of this extent

        Usage:

        .. code-block:: python

            >>> A = TemporalExtent(start_time=5, end_time=7 )
            >>> B = TemporalExtent(start_time=7, end_time=9 )
            >>> A.precedes(B)
            True
            >>> B.precedes(A)
            False


        """
        if self.D["end_time"] is None:
            return False

        if self.D["end_time"] == extent.D["start_time"]:
            return True
        else:
            return False

    def during(self, extent):
        """Return True if this temporal extent (A) is located during the provided
        temporal extent (B)
        ::

            A   |-------|
            B  |---------|

        :param extent: The temporal extent object that contains this extent

        Usage:

        .. code-block:: python

            >>> A = TemporalExtent(start_time=5, end_time=7 )
            >>> B = TemporalExtent(start_time=4, end_time=9 )
            >>> A.during(B)
            True
            >>> B.during(A)
            False

        """
        # Check single point of time in interval
        if extent.D["end_time"] is None:
            return False

        # Check single point of time in interval
        if self.D["end_time"] is None:
            if (
                self.D["start_time"] >= extent.D["start_time"]
                and self.D["start_time"] < extent.D["end_time"]
            ):
                return True
            else:
                return False

        if (
            self.D["start_time"] > extent.D["start_time"]
            and self.D["end_time"] < extent.D["end_time"]
        ):
            return True
        else:
            return False

    def contains(self, extent):
        """Return True if this temporal extent (A) contains the provided
        temporal extent (B)
        ::

            A  |---------|
            B   |-------|

        :param extent: The temporal extent object that is located
                       during this extent

        Usage:

        .. code-block:: python

            >>> A = TemporalExtent(start_time=4, end_time=9 )
            >>> B = TemporalExtent(start_time=5, end_time=8 )
            >>> A.contains(B)
            True
            >>> B.contains(A)
            False

        """
        # Check single point of time in interval
        if self.D["end_time"] is None:
            return False

        # Check single point of time in interval
        if extent.D["end_time"] is None:
            if (
                self.D["start_time"] <= extent.D["start_time"]
                and self.D["end_time"] > extent.D["start_time"]
            ):
                return True
            else:
                return False

        if (
            self.D["start_time"] < extent.D["start_time"]
            and self.D["end_time"] > extent.D["end_time"]
        ):
            return True
        else:
            return False

    def equal(self, extent):
        """Return True if this temporal extent (A) is equal to the provided
        temporal extent (B)
        ::

            A  |---------|
            B  |---------|

        :param extent: The temporal extent object that is equal
                       during this extent

        Usage:

        .. code-block:: python

            >>> A = TemporalExtent(start_time=5, end_time=6 )
            >>> B = TemporalExtent(start_time=5, end_time=6 )
            >>> A.equal(B)
            True
            >>> B.equal(A)
            True

        """
        if self.D["end_time"] is None and extent.D["end_time"] is None:
            if self.D["start_time"] == extent.D["start_time"]:
                return True
            else:
                return False

        if self.D["end_time"] is None or extent.D["end_time"] is None:
            return False

        if (
            self.D["start_time"] == extent.D["start_time"]
            and self.D["end_time"] == extent.D["end_time"]
        ):
            return True
        else:
            return False

    def overlaps(self, extent):
        """Return True if this temporal extent (A) overlapped the provided
        temporal extent (B)
        ::

            A  |---------|
            B    |---------|

        :param extent: The temporal extent object that is overlaps
                       this extent

        Usage:

        .. code-block:: python

            >>> A = TemporalExtent(start_time=5, end_time=7 )
            >>> B = TemporalExtent(start_time=6, end_time=8 )
            >>> A.overlaps(B)
            True
            >>> B.overlaps(A)
            False

            >>> A = TemporalExtent(start_time=5, end_time=6 )
            >>> B = TemporalExtent(start_time=6, end_time=8 )
            >>> A.overlaps(B)
            False
            >>> B.overlaps(A)
            False

        """
        if self.D["end_time"] is None or extent.D["end_time"] is None:
            return False

        if (
            self.D["start_time"] < extent.D["start_time"]
            and self.D["end_time"] < extent.D["end_time"]
            and self.D["end_time"] > extent.D["start_time"]
        ):
            return True
        else:
            return False

    def overlapped(self, extent):
        """Return True if this temporal extent (A) overlapps the provided
        temporal extent (B)
        ::

            A    |---------|
            B  |---------|


        :param extent: The temporal extent object that is overlapped
                       this extent

        Usage:

        .. code-block:: python

            >>> A = TemporalExtent(start_time=6, end_time=8 )
            >>> B = TemporalExtent(start_time=5, end_time=7 )
            >>> A.overlapped(B)
            True
            >>> B.overlapped(A)
            False

            >>> A = TemporalExtent(start_time=6, end_time=8 )
            >>> B = TemporalExtent(start_time=5, end_time=6 )
            >>> A.overlapped(B)
            False
            >>> B.overlapped(A)
            False

        """
        if self.D["end_time"] is None or extent.D["end_time"] is None:
            return False

        if (
            self.D["start_time"] > extent.D["start_time"]
            and self.D["end_time"] > extent.D["end_time"]
            and self.D["start_time"] < extent.D["end_time"]
        ):
            return True
        else:
            return False

    def temporal_relation(self, extent):
        """Returns the temporal relation between temporal objects
        Temporal relationships are implemented after
        [Allen and Ferguson 1994 Actions and Events in Interval Temporal Logic]

        The following temporal relationships are supported:

            - equal
            - during
            - contains
            - overlaps
            - overlapped
            - after
            - before
            - starts
            - finishes
            - started
            - finished
            - follows
            - precedes

        :param extent: The temporal extent
        :return: The name of the temporal relation or None if no relation
                 found
        """

        # First check for correct time
        if "start_time" not in self.D:
            return None
        if "end_time" not in self.D:
            return None
        if "start_time" not in extent.D:
            return None
        if "end_time" not in extent.D:
            return None
        # Return None if the start_time is undefined
        if self.D["start_time"] is None or extent.D["start_time"] is None:
            return None

        if self.equal(extent):
            return "equal"
        if self.during(extent):
            return "during"
        if self.contains(extent):
            return "contains"
        if self.overlaps(extent):
            return "overlaps"
        if self.overlapped(extent):
            return "overlapped"
        if self.after(extent):
            return "after"
        if self.before(extent):
            return "before"
        if self.starts(extent):
            return "starts"
        if self.finishes(extent):
            return "finishes"
        if self.started(extent):
            return "started"
        if self.finished(extent):
            return "finished"
        if self.follows(extent):
            return "follows"
        if self.precedes(extent):
            return "precedes"
        return None

    def set_id(self, ident):
        """Convenient method to set the unique identifier (primary key)"""
        self.ident = ident
        self.D["id"] = ident

    def set_start_time(self, start_time):
        """Set the valid start time of the extent"""
        self.D["start_time"] = start_time

    def set_end_time(self, end_time):
        """Set the valid end time of the extent"""
        self.D["end_time"] = end_time

    def get_id(self):
        """Convenient method to get the unique identifier (primary key)
        :return: None if not found
        """
        if "id" in self.D:
            return self.D["id"]
        else:
            return None

    def get_start_time(self):
        """Get the valid start time of the extent
        :return: None if not found"""
        if "start_time" in self.D:
            return self.D["start_time"]
        else:
            return None

    def get_end_time(self):
        """Get the valid end time of the extent
        :return: None if not found"""
        if "end_time" in self.D:
            return self.D["end_time"]
        else:
            return None

    # Set the properties
    id = property(fget=get_id, fset=set_id)
    start_time = property(fget=get_start_time, fset=set_start_time)
    end_time = property(fget=get_end_time, fset=set_end_time)

    def print_info(self):
        """Print information about this class in human readable style"""
        #      0123456789012345678901234567890
        print(" | Start time:................. " + str(self.get_start_time()))
        print(" | End time:................... " + str(self.get_end_time()))

    def print_shell_info(self):
        """Print information about this class in shell style"""
        print("start_time='{}'".format(str(self.get_start_time())))
        print("end_time='{}'".format(str(self.get_end_time())))


###############################################################################


class AbsoluteTemporalExtent(TemporalExtent):
    """This is the absolute time class for all maps and spacetime datasets

    start_time and end_time must be of type datetime
    """

    def __init__(self, table=None, ident=None, start_time=None, end_time=None):

        TemporalExtent.__init__(self, table, ident, start_time, end_time)

    def print_info(self):
        """Print information about this class in human readable style"""
        #      0123456789012345678901234567890
        print(
            " +-------------------- Absolute time -----------------------------------------+"
        )
        TemporalExtent.print_info(self)

    def print_shell_info(self):
        """Print information about this class in shell style"""
        TemporalExtent.print_shell_info(self)


###############################################################################


class RasterAbsoluteTime(AbsoluteTemporalExtent):
    def __init__(self, ident=None, start_time=None, end_time=None):
        AbsoluteTemporalExtent.__init__(
            self, "raster_absolute_time", ident, start_time, end_time
        )


class Raster3DAbsoluteTime(AbsoluteTemporalExtent):
    def __init__(self, ident=None, start_time=None, end_time=None):
        AbsoluteTemporalExtent.__init__(
            self, "raster3d_absolute_time", ident, start_time, end_time
        )


class VectorAbsoluteTime(AbsoluteTemporalExtent):
    def __init__(self, ident=None, start_time=None, end_time=None):
        AbsoluteTemporalExtent.__init__(
            self, "vector_absolute_time", ident, start_time, end_time
        )


###############################################################################


class STDSAbsoluteTime(AbsoluteTemporalExtent):
    """This class implements the absolute time extent for space time dataset

    In addition to the existing functionality the granularity and the
    map_time are added.

    Usage:

    .. code-block:: python

        >>> init()
        >>> A = STDSAbsoluteTime(table="strds_absolute_time",
        ... ident="strds@PERMANENT", start_time=datetime(2001, 01, 01),
        ... end_time=datetime(2005,01,01), granularity="1 days",
        ... map_time="interval")
        >>> A.id
        'strds@PERMANENT'
        >>> A.start_time
        datetime.datetime(2001, 1, 1, 0, 0)
        >>> A.end_time
        datetime.datetime(2005, 1, 1, 0, 0)
        >>> A.granularity
        '1 days'
        >>> A.map_time
        'interval'
        >>> A.print_info()
         +-------------------- Absolute time -----------------------------------------+
         | Start time:................. 2001-01-01 00:00:00
         | End time:................... 2005-01-01 00:00:00
         | Granularity:................ 1 days
         | Temporal type of maps:...... interval
        >>> A.print_shell_info()
        start_time='2001-01-01 00:00:00'
        end_time='2005-01-01 00:00:00'
        granularity='1 days'
        map_time=interval

    """

    def __init__(
        self,
        table=None,
        ident=None,
        start_time=None,
        end_time=None,
        granularity=None,
        map_time=None,
    ):
        AbsoluteTemporalExtent.__init__(self, table, ident, start_time, end_time)

        self.set_granularity(granularity)
        self.set_map_time(map_time)

    def set_granularity(self, granularity):
        """Set the granularity of the space time dataset"""
        self.D["granularity"] = granularity

    def set_map_time(self, map_time):
        """Set the type of the map time

        Registered maps may have different types of time:

        - Single point of time "point"
        - Time intervals "interval"
        - Single point and interval time "mixed"

        This variable will be set automatically when maps are registered.
        """
        self.D["map_time"] = map_time

    def get_granularity(self):
        """Get the granularity of the space time dataset
        :return: None if not found"""
        if "granularity" in self.D:
            return self.D["granularity"]
        else:
            return None

    def get_map_time(self):
        """Get the type of the map time

        Registered maps may have different types of time:

        - Single point of time "point"
        - Time intervals "interval"
        - Single point and interval time "mixed"

        This variable will be set automatically when maps are registered.
        """
        if "map_time" in self.D:
            return self.D["map_time"]
        else:
            return None

    # Properties
    granularity = property(fget=get_granularity, fset=set_granularity)
    map_time = property(fget=get_map_time, fset=set_map_time)

    def print_info(self):
        """Print information about this class in human readable style"""
        AbsoluteTemporalExtent.print_info(self)
        #      0123456789012345678901234567890
        print(" | Granularity:................ " + str(self.get_granularity()))
        print(" | Temporal type of maps:...... " + str(self.get_map_time()))

    def print_shell_info(self):
        """Print information about this class in shell style"""
        AbsoluteTemporalExtent.print_shell_info(self)
        print("granularity='{}'".format(str(self.get_granularity())))
        print("map_time=" + str(self.get_map_time()))


###############################################################################


class STRDSAbsoluteTime(STDSAbsoluteTime):
    def __init__(self, ident=None, start_time=None, end_time=None, granularity=None):
        STDSAbsoluteTime.__init__(
            self, "strds_absolute_time", ident, start_time, end_time, granularity
        )


class STR3DSAbsoluteTime(STDSAbsoluteTime):
    def __init__(self, ident=None, start_time=None, end_time=None, granularity=None):
        STDSAbsoluteTime.__init__(
            self, "str3ds_absolute_time", ident, start_time, end_time, granularity
        )


class STVDSAbsoluteTime(STDSAbsoluteTime):
    def __init__(self, ident=None, start_time=None, end_time=None, granularity=None):
        STDSAbsoluteTime.__init__(
            self, "stvds_absolute_time", ident, start_time, end_time, granularity
        )


###############################################################################


class RelativeTemporalExtent(TemporalExtent):
    """This is the relative time class for all maps and space time datasets

    start_time and end_time must be of type integer

    Usage:

    .. code-block:: python

        >>> init()
        >>> A = RelativeTemporalExtent(table="raster_relative_time",
        ... ident="soil@PERMANENT", start_time=0, end_time=1, unit="years")
        >>> A.id
        'soil@PERMANENT'
        >>> A.start_time
        0
        >>> A.end_time
        1
        >>> A.unit
        'years'
        >>> A.print_info()
         +-------------------- Relative time -----------------------------------------+
         | Start time:................. 0
         | End time:................... 1
         | Relative time unit:......... years
        >>> A.print_shell_info()
        start_time='0'
        end_time='1'
        unit=years

    """

    def __init__(
        self, table=None, ident=None, start_time=None, end_time=None, unit=None
    ):

        TemporalExtent.__init__(self, table, ident, start_time, end_time)
        self.set_unit(unit)

    def set_unit(self, unit):
        """Set the unit of the relative time. Valid units are:

        - years
        - months
        - days
        - hours
        - minutes
        - seconds
        """
        self.D["unit"] = unit

    def get_unit(self):
        """Get the unit of the relative time
        :return: None if not found"""
        if "unit" in self.D:
            return self.D["unit"]
        else:
            return None

    def temporal_relation(self, map):
        """Returns the temporal relation between temporal objects
        Temporal relationships are implemented after
        [Allen and Ferguson 1994 Actions and Events in Interval Temporal Logic]
        """

        # Check units for relative time
        if "unit" not in self.D:
            return None
        if "unit" not in map.D:
            return None

        # Units must be equal
        if self.D["unit"] != map.D["unit"]:
            return None

        return TemporalExtent.temporal_relation(self, map)

    # Properties
    unit = property(fget=get_unit, fset=set_unit)

    def print_info(self):
        """Print information about this class in human readable style"""
        #      0123456789012345678901234567890
        print(
            " +-------------------- Relative time -----------------------------------------+"
        )
        TemporalExtent.print_info(self)
        print(" | Relative time unit:......... " + str(self.get_unit()))

    def print_shell_info(self):
        """Print information about this class in shell style"""
        TemporalExtent.print_shell_info(self)
        print("unit=" + str(self.get_unit()))


###############################################################################


class RasterRelativeTime(RelativeTemporalExtent):
    def __init__(self, ident=None, start_time=None, end_time=None, unit=None):
        RelativeTemporalExtent.__init__(
            self, "raster_relative_time", ident, start_time, end_time, unit
        )


class Raster3DRelativeTime(RelativeTemporalExtent):
    def __init__(self, ident=None, start_time=None, end_time=None, unit=None):
        RelativeTemporalExtent.__init__(
            self, "raster3d_relative_time", ident, start_time, end_time, unit
        )


class VectorRelativeTime(RelativeTemporalExtent):
    def __init__(self, ident=None, start_time=None, end_time=None, unit=None):
        RelativeTemporalExtent.__init__(
            self, "vector_relative_time", ident, start_time, end_time, unit
        )


###############################################################################


class STDSRelativeTime(RelativeTemporalExtent):
    """This is the relative time class for all maps and space time datasets

    start_time and end_time must be of type integer

    Usage:

    .. code-block:: python

        >>> init()
        >>> A = STDSRelativeTime(table="strds_relative_time",
        ... ident="strds@PERMANENT", start_time=0, end_time=1, unit="years",
        ... granularity=5, map_time="interval")
        >>> A.id
        'strds@PERMANENT'
        >>> A.start_time
        0
        >>> A.end_time
        1
        >>> A.unit
        'years'
        >>> A.granularity
        5
        >>> A.map_time
        'interval'
        >>> A.print_info()
         +-------------------- Relative time -----------------------------------------+
         | Start time:................. 0
         | End time:................... 1
         | Relative time unit:......... years
         | Granularity:................ 5
         | Temporal type of maps:...... interval
        >>> A.print_shell_info()
        start_time='0'
        end_time='1'
        unit=years
        granularity=5
        map_time=interval

    """

    def __init__(
        self,
        table=None,
        ident=None,
        start_time=None,
        end_time=None,
        unit=None,
        granularity=None,
        map_time=None,
    ):
        RelativeTemporalExtent.__init__(self, table, ident, start_time, end_time, unit)

        self.set_granularity(granularity)
        self.set_map_time(map_time)

    def set_granularity(self, granularity):
        """Set the granularity of the space time dataset"""
        self.D["granularity"] = granularity

    def set_map_time(self, map_time):
        """Set the type of the map time

        Registered maps may have different types of time:

        - Single point of time "point"
        - Time intervals "interval"
        - Single point and interval time "mixed"

        This variable will be set automatically when maps are registered.
        """
        self.D["map_time"] = map_time

    def get_granularity(self):
        """Get the granularity of the space time dataset
        :return: None if not found"""
        if "granularity" in self.D:
            return self.D["granularity"]
        else:
            return None

    def get_map_time(self):
        """Get the type of the map time

        Registered maps may have different types of time:

        - Single point of time "point"
        - Time intervals "interval"
        - Single point and interval time "mixed"

        This variable will be set automatically when maps are registered.
        """
        if "map_time" in self.D:
            return self.D["map_time"]
        else:
            return None

    # Properties
    granularity = property(fget=get_granularity, fset=set_granularity)
    map_time = property(fget=get_map_time, fset=set_map_time)

    def print_info(self):
        """Print information about this class in human readable style"""
        RelativeTemporalExtent.print_info(self)
        #      0123456789012345678901234567890
        print(" | Granularity:................ " + str(self.get_granularity()))
        print(" | Temporal type of maps:...... " + str(self.get_map_time()))

    def print_shell_info(self):
        """Print information about this class in shell style"""
        RelativeTemporalExtent.print_shell_info(self)
        print("granularity=" + str(self.get_granularity()))
        print("map_time=" + str(self.get_map_time()))


###############################################################################


class STRDSRelativeTime(STDSRelativeTime):
    def __init__(
        self,
        ident=None,
        start_time=None,
        end_time=None,
        unit=None,
        granularity=None,
        map_time=None,
    ):
        STDSRelativeTime.__init__(
            self,
            "strds_relative_time",
            ident,
            start_time,
            end_time,
            unit,
            granularity,
            map_time,
        )


class STR3DSRelativeTime(STDSRelativeTime):
    def __init__(
        self,
        ident=None,
        start_time=None,
        end_time=None,
        unit=None,
        granularity=None,
        map_time=None,
    ):
        STDSRelativeTime.__init__(
            self,
            "str3ds_relative_time",
            ident,
            start_time,
            end_time,
            unit,
            granularity,
            map_time,
        )


class STVDSRelativeTime(STDSRelativeTime):
    def __init__(
        self,
        ident=None,
        start_time=None,
        end_time=None,
        unit=None,
        granularity=None,
        map_time=None,
    ):
        STDSRelativeTime.__init__(
            self,
            "stvds_relative_time",
            ident,
            start_time,
            end_time,
            unit,
            granularity,
            map_time,
        )


###############################################################################

if __name__ == "__main__":
    import doctest

    doctest.testmod()
