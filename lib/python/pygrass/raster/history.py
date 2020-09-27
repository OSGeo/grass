# -*- coding: utf-8 -*-
"""
Created on Thu Jun 28 17:44:45 2012

@author: pietro
"""
import ctypes
import grass.lib.raster as libraster
import datetime
from grass.script.utils import encode
from grass.pygrass.utils import decode


class History(object):
    """History class help to manage all the metadata of a raster map
    """

    def __init__(self, name, mapset='', mtype='',
                 creator='', src1='', src2='', keyword='',
                 date='', title=''):
        self.c_hist = ctypes.pointer(libraster.History())
        #                'Tue Nov  7 01:11:23 2006'
        self.date_fmt = '%a %b  %d %H:%M:%S %Y'
        self.name = name
        self.mapset = mapset
        self.mtype = mtype
        self.creator = creator
        self.src1 = src1
        self.src2 = src2
        self.keyword = keyword
        self.date = date
        self.title = title
        self.attrs = ['name', 'mapset', 'mtype', 'creator', 'src1', 'src2',
                      'keyword', 'date', 'title']

    def __repr__(self):
        return "History(%s)" % ', '.join(["%s=%r" % (self.attr, getattr(self, attr))
                                          for attr in self.attrs])

    def __del__(self):
        """Rast_free_history"""
        pass
        
    def __eq__(self, hist):
        for attr in self.attrs:
            if getattr(self, attr) != getattr(hist, attr):
                return False
        return True

    def __len__(self):
        return self.length()

    def __iter__(self):
        return ((attr, getattr(self, attr)) for attr in self.attrs)

    # ----------------------------------------------------------------------
    # libraster.HIST_CREATOR
    def _get_creator(self):
        return decode(libraster.Rast_get_history(self.c_hist,
                                                 libraster.HIST_CREATOR))

    def _set_creator(self, creator):
        creator = encode(creator)
        return libraster.Rast_set_history(self.c_hist,
                                          libraster.HIST_CREATOR,
                                          ctypes.c_char_p(creator))

    creator = property(fget=_get_creator, fset=_set_creator,
                       doc="Set or obtain the creator of map")

    # ----------------------------------------------------------------------
    # libraster.HIST_DATSRC_1
    def _get_src1(self):
        return decode(libraster.Rast_get_history(self.c_hist,
                                                 libraster.HIST_DATSRC_1))

    def _set_src1(self, src1):
        src1 = encode(src1)
        return libraster.Rast_set_history(self.c_hist,
                                          libraster.HIST_DATSRC_1,
                                          ctypes.c_char_p(src1))

    src1 = property(fget=_get_src1, fset=_set_src1,
                    doc="Set or obtain the first source of map")

    # ----------------------------------------------------------------------
    # libraster.HIST_DATSRC_2
    def _get_src2(self):
        return decode(libraster.Rast_get_history(self.c_hist,
                                                 libraster.HIST_DATSRC_2))

    def _set_src2(self, src2):
        src2 = encode(src2)
        return libraster.Rast_set_history(self.c_hist,
                                          libraster.HIST_DATSRC_2,
                                          ctypes.c_char_p(src2))

    src2 = property(fget=_get_src2, fset=_set_src2,
                    doc="Set or obtain the second source of map")

    # ----------------------------------------------------------------------
    # libraster.HIST_KEYWORD
    def _get_keyword(self):
        return decode(libraster.Rast_get_history(self.c_hist,
                                                 libraster.HIST_KEYWRD))

    def _set_keyword(self, keyword):
        keyword = encode(keyword)
        return libraster.Rast_set_history(self.c_hist,
                                          libraster.HIST_KEYWRD,
                                          ctypes.c_char_p(keyword))

    keyword = property(fget=_get_keyword, fset=_set_keyword,
                       doc="Set or obtain the keywords of map")

    # ----------------------------------------------------------------------
    # libraster.HIST_MAPID
    def _get_date(self):
        date_str = decode(libraster.Rast_get_history(self.c_hist,
                                                     libraster.HIST_MAPID))
        if date_str:
            try:
                return datetime.datetime.strptime(date_str, self.date_fmt)
            except:
                return date_str

    def _set_date(self, datetimeobj):
        if datetimeobj:
            date_str = datetimeobj.strftime(self.date_fmt)
            date_str = encode(date_str)
            return libraster.Rast_set_history(self.c_hist,
                                              libraster.HIST_MAPID,
                                              ctypes.c_char_p(date_str))

    date = property(fget=_get_date, fset=_set_date,
                    doc="Set or obtain the date of map")

    # ----------------------------------------------------------------------
    # libraster.HIST_MAPSET
    def _get_mapset(self):
        return decode(libraster.Rast_get_history(self.c_hist,
                                                 libraster.HIST_MAPSET))

    def _set_mapset(self, mapset):
        mapset = encode(mapset)
        return libraster.Rast_set_history(self.c_hist,
                                          libraster.HIST_MAPSET,
                                          ctypes.c_char_p(mapset))

    mapset = property(fget=_get_mapset, fset=_set_mapset,
                      doc="Set or obtain the mapset of map")

    # ----------------------------------------------------------------------
    # libraster.HIST_MAPTYPE
    def _get_maptype(self):
        return decode(libraster.Rast_get_history(self.c_hist,
                                                 libraster.HIST_MAPTYPE))

    def _set_maptype(self, maptype):
        maptype = encode(maptype)
        return libraster.Rast_set_history(self.c_hist,
                                          libraster.HIST_MAPTYPE,
                                          ctypes.c_char_p(maptype))

    maptype = property(fget=_get_maptype, fset=_set_maptype,
                       doc="Set or obtain the type of map")

    # ----------------------------------------------------------------------
    # libraster.HIST_NUM_FIELDS
    #
    # Never used in any raster modules
    #
    #    def _get_num_fields(self):
    #        return libraster.Rast_get_history(self.c_hist,
    #                                          libraster.HIST_NUM_FIELDS)
    #
    #    def _set_num_fields(self, num_fields):
    #        return libraster.Rast_set_history(self.c_hist,
    #                                          libraster.HIST_NUM_FIELDS,
    #                                          ctypes.c_char_p(num_fields))
    #
    #    num_fields = property(fget = _get_num_fields, fset = _set_num_fields)
    # ----------------------------------------------------------------------
    # libraster.HIST_TITLE
    def _get_title(self):
        return decode(libraster.Rast_get_history(self.c_hist,
                                                 libraster.HIST_TITLE))

    def _set_title(self, title):
        title = encode(title)
        return libraster.Rast_set_history(self.c_hist,
                                          libraster.HIST_TITLE,
                                          ctypes.c_char_p(title))

    title = property(fget=_get_title, fset=_set_title,
                     doc="Set or obtain the title of map")

    def append(self, obj):
        """Rast_append_history"""
        libraster.Rast_append_history(self.c_hist,
                                      ctypes.c_char_p(str(obj)))

    def append_fmt(self, fmt, *args):
        """Rast_append_format_history"""
        libraster.Rast_append_format_history(self.c_hist,
                                             ctypes.c_char_p(fmt),
                                             *args)

    def clear(self):
        """Clear the history"""
        libraster.Rast_clear_history(self.c_hist)

    def command(self):
        """Rast_command_history"""
        libraster.Rast_command_history(self.c_hist)

    def format(self, field, fmt, *args):
        """Rast_format_history"""
        libraster.Rast_format_history(self.c_hist,
                                      ctypes.c_int(field),
                                      ctypes.c_char_p(fmt),
                                      *args)

    def length(self):
        """Rast_history_length"""
        return libraster.Rast_history_length(self.c_hist)

    def line(self, line):
        """Rast_history_line"""
        return libraster.Rast_history_line(self.c_hist,
                                           ctypes.c_int(line))

    def read(self):
        """Read the history of map, users need to use this function to
        obtain all the information of map. ::

            >>> import grass.lib.gis as libgis
            >>> import ctypes
            >>> import grass.lib.raster as libraster
            >>> hist = libraster.History()

        ..
        """
        libraster.Rast_read_history(self.name, self.mapset, self.c_hist)

    def write(self):
        """Rast_write_history"""
        libraster.Rast_write_history(self.name,
                                     self.c_hist)

    def short(self):
        """Rast_short_history"""
        libraster.Rast_short_history(self.name,
                                     'raster',
                                     self.c_hist)
