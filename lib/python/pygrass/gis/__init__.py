#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from __future__ import (nested_scopes, generators, division, absolute_import,
                        with_statement, print_function, unicode_literals)
from os import listdir
from os.path import join, isdir
import shutil
import ctypes as ct
import fnmatch


import grass.lib.gis as libgis
from grass.pygrass.errors import GrassError
from grass.script.utils import encode, decode
from grass.pygrass.utils import getenv
from grass.pygrass.gis.region import Region

test_vector_name = "Gis_test_vector"
test_raster_name = "Gis_test_raster"

libgis.G_gisinit('')


ETYPE = {'raster': libgis.G_ELEMENT_RASTER,
         'raster_3d': libgis.G_ELEMENT_RASTER3D,
         'vector': libgis.G_ELEMENT_VECTOR,
         'label': libgis.G_ELEMENT_LABEL,
         'region': libgis.G_ELEMENT_REGION,
         'group': libgis.G_ELEMENT_GROUP}


CHECK_IS = {"GISBASE": libgis.G_is_gisbase,
            "GISDBASE": lambda x: True,
            "LOCATION_NAME": libgis.G_is_project,
            "MAPSET": libgis.G_is_subproject}


def is_valid(value, path, type):
    """Private function to check the correctness of a value.

    :param value: Name of the directory
    :type value: str

    :param path: Path where the directory is located
    :type path: path

    :param type: it is a string defining the type that will e checked,
                 valid types are: GISBASE, GISDBASE, LOCATION_NAME, MAPSET
    :type type: str

    :return: True if valid else False
    :rtype: str
    """
    return bool(CHECK_IS[type](join(path, value)))


def _check_raise(value, path, type):
    """Private function to check the correctness of a value.

    :param value: Name of the directory
    :type value: str

    :param path: Path where the directory is located
    :type path: path

    :param type: it is a string defining the type that will e checked,
                 valid types are: GISBASE, GISDBASE, LOCATION_NAME, MAPSET
    :type type: str

    :return: the value if verify else None and
             if value is empty return environmental variable
    :rtype: str
    """
    if value == '':
        from grass.pygrass.utils import getenv
        return getenv(type)
    if is_valid(value, path, type):
        return value
    raise GrassError("%s <%s> not found" % (type.title(), join(path, value)))


def set_current_subproject(subproject, project=None, gisdbase=None):
    """Set the current subproject as working area

    :param subproject: Name of the subproject
    :type value: str

    :param project: Name of the project
    :type project: str

    :param gisdbase: Name of the gisdbase
    :type gisdbase: str
    """
    libgis.G_setenv('MAPSET', subproject)
    if project:
        libgis.G_setenv('LOCATION_NAME', project)
    if gisdbase:
        libgis.G_setenv('GISDBASE', gisdbase)


def make_subproject(subproject, project=None, gisdbase=None):
    """Create a new subproject

    :param subproject: Name of the subproject
    :type value: str

    :param project: Name of the project
    :type project: str

    :param gisdbase: Name of the gisdbase
    :type gisdbase: str"""
    res = libgis.G_make_subproject(gisdbase, project, subproject)
    if res == -1:
        raise GrassError("Cannot create new subproject")
    elif res == -2:
        raise GrassError("Illegal name")


class Gisdbase(object):
    """Return Gisdbase object. ::

        >>> from grass.script.core import gisenv
        >>> gisdbase = Gisdbase()
        >>> gisdbase.name == gisenv()['GISDBASE']
        True

    ..
    """

    def __init__(self, gisdbase=''):
        self.name = gisdbase

    def _get_name(self):
        return self._name

    def _set_name(self, name):
        self._name = _check_raise(name, '', "GISDBASE")

    name = property(fget=_get_name, fset=_set_name,
                    doc="Set or obtain the name of GISDBASE")

    def __str__(self):
        return self.name

    def __repr__(self):
        return 'Gisdbase(%s)' % self.name

    def __getitem__(self, project):
        """Return a Project object. ::

            >>> from grass.script.core import gisenv
            >>> loc_env = gisenv()['LOCATION_NAME']
            >>> gisdbase = Gisdbase()
            >>> loc_py = gisdbase[loc_env]
            >>> loc_env == loc_py.name
            True

        ..
        """
        if project in self.projects():
            return Project(project, self.name)
        else:
            raise KeyError('Project: %s does not exist' % project)

    def __iter__(self):
        for loc in self.projects():
            yield Project(loc, self.name)

    # TODO remove or complete this function
    def new_project(self):
        if libgis.G_make_project() != 0:
            raise GrassError("Cannot create new project")

    def projects(self):
        """Return a list of projects that are available in the gisdbase: ::

            >>> gisdbase = Gisdbase()
            >>> gisdbase.projects()                     # doctest: +ELLIPSIS
            [...]

        ..
        """
        return sorted([loc for loc in listdir(self.name)
                       if libgis.G_is_project(encode(join(self.name, loc)))])


class Project(object):
    """Project object ::

        >>> from grass.script.core import gisenv
        >>> project = Project()
        >>> project                                      # doctest: +ELLIPSIS
        Project(...)
        >>> project.gisdbase == gisenv()['GISDBASE']
        True
        >>> project.name == gisenv()['LOCATION_NAME']
        True

    ..
    """

    def __init__(self, project='', gisdbase=''):
        self.gisdbase = gisdbase
        self.name = project

    def _get_gisdb(self):
        return self._gisdb

    def _set_gisdb(self, gisdb):
        self._gisdb = _check_raise(gisdb, '', "GISDBASE")

    gisdbase = property(fget=_get_gisdb, fset=_set_gisdb,
                        doc="Set or obtain the name of GISDBASE")

    def _get_name(self):
        return self._name

    def _set_name(self, name):
        self._name = _check_raise(name, self._gisdb, "LOCATION_NAME")

    name = property(fget=_get_name, fset=_set_name,
                    doc="Set or obtain the name of LOCATION")

    def __getitem__(self, subproject):
        if subproject in self.subprojects():
            return Subproject(subproject)
        else:
            raise KeyError('Subproject: %s does not exist' % subproject)

    def __iter__(self):
        lpath = self.path()
        return (m for m in listdir(lpath)
                if (isdir(join(lpath, m)) and is_valid(m, lpath, "MAPSET")))

    def __len__(self):
        return len(self.subprojects())

    def __str__(self):
        return self.name

    def __repr__(self):
        return 'Project(%r)' % self.name

    def subprojects(self, pattern=None, permissions=True):
        """Return a list of the available subprojects.

        :param pattern: the pattern to filter the result
        :type pattern: str
        :param permissions: check the permission of subproject
        :type permissions: bool
        :return: a list of subproject's names
        :rtype: list of strings

        ::

            >>> project = Project()
            >>> sorted(project.subprojects())                # doctest: +ELLIPSIS
            [...]

        """
        subprojects = [subproject for subproject in self]
        if permissions:
            subprojects = [subproject for subproject in subprojects
                       if libgis.G_subproject_permissions(encode(subproject))]
        if pattern:
            return fnmatch.filter(subprojects, pattern)
        return subprojects

    def path(self):
        """Return the complete path of the project"""
        return join(self.gisdbase, self.name)


class Subproject(object):
    """Subproject ::

        >>> from grass.script.core import gisenv
        >>> genv = gisenv()
        >>> subproject = Subproject()
        >>> subproject                                        # doctest: +ELLIPSIS
        Subproject(...)
        >>> subproject.gisdbase == genv['GISDBASE']
        True
        >>> subproject.project == genv['LOCATION_NAME']
        True
        >>> subproject.name == genv['MAPSET']
        True

    ..
    """

    def __init__(self, subproject='', project='', gisdbase=''):
        self.gisdbase = gisdbase
        self.project = project
        self.name = subproject
        self.visible = VisibleSubproject(self.name, self.project, self.gisdbase)

    def _get_gisdb(self):
        return self._gisdb

    def _set_gisdb(self, gisdb):
        self._gisdb = _check_raise(gisdb, '', "GISDBASE")

    gisdbase = property(fget=_get_gisdb, fset=_set_gisdb,
                        doc="Set or obtain the name of GISDBASE")

    def _get_loc(self):
        return self._loc

    def _set_loc(self, loc):
        self._loc = _check_raise(loc, self._gisdb, "LOCATION_NAME")

    project = property(fget=_get_loc, fset=_set_loc,
                        doc="Set or obtain the name of LOCATION")

    def _get_name(self):
        return self._name

    def _set_name(self, name):
        self._name = _check_raise(name, join(self._gisdb, self._loc), "MAPSET")

    name = property(fget=_get_name, fset=_set_name,
                    doc="Set or obtain the name of MAPSET")

    def __str__(self):
        return self.name

    def __repr__(self):
        return 'Subproject(%r)' % self.name

    def glist(self, type, pattern=None):
        """Return a list of grass types like:

            * 'group',
            * 'label',
            * 'raster',
            * 'raster_3d',
            * 'region',
            * 'vector',

        :param type: the type of element to query
        :type type: str
        :param pattern: the pattern to filter the result
        :type pattern: str

        ::

            >>> subproject = Subproject()
            >>> subproject.current()
            >>> rast = subproject.glist('raster')
            >>> test_raster_name in rast
            True
            >>> vect = subproject.glist('vector')
            >>> test_vector_name in vect
            True

        ..
        """
        if type not in ETYPE:
            str_err = "Type %s is not valid, valid types are: %s."
            raise TypeError(str_err % (type, ', '.join(ETYPE.keys())))
        clist = libgis.G_list(ETYPE[type], self.gisdbase,
                              self.project, self.name)
        elist = []
        for el in clist:
            el_name = ct.cast(el, ct.c_char_p).value
            if el_name:
                elist.append(decode(el_name))
            else:
                if pattern:
                    return fnmatch.filter(elist, pattern)
                return elist

    def is_current(self):
        """Check if the MAPSET is the working MAPSET"""
        return (self.name == getenv('MAPSET') and
                self.project == getenv('LOCATION_NAME') and
                self.gisdbase == getenv('GISDBASE'))

    def current(self):
        """Set the subproject as current"""
        set_current_subproject(self.name, self.project, self.gisdbase)

    def delete(self):
        """Delete the subproject"""
        if self.is_current():
            raise GrassError('The subproject is in use.')
        shutil.rmtree(self.path())

    def path(self):
        """Return the complete path of the subproject"""
        return join(self.gisdbase, self.project, self.name)


class VisibleSubproject(object):
    """VisibleSubproject object
    """

    def __init__(self, subproject, project='', gisdbase=''):
        self.subproject = subproject
        self.project = Project(project, gisdbase)
        self._list = []
        self.spath = join(self.project.path(), self.subproject, 'SEARCH_PATH')

    def __repr__(self):
        return repr(self.read())

    def __iter__(self):
        for subproject in self.read():
            yield subproject

    def read(self):
        """Return the subprojects in the search path"""
        with open(self.spath, "ab+") as f:
            lines = f.readlines()
            if lines:
                return [decode(l.strip()) for l in lines]
        lns = [u'PERMANENT', ]
        self._write(lns)
        return lns

    def _write(self, subprojects):
        """Write to SEARCH_PATH file the changes in the search path

        :param subprojects: a list of subproject's names
        :type subprojects: list
        """
        with open(self.spath, "wb+") as f:
            ms = [decode(m) for m in self.project.subprojects()]
            f.write(b'\n'.join([encode(m) for m in subprojects if m in ms]))

    def add(self, subproject):
        """Add a subproject to the search path

        :param subproject: a subproject's name
        :type subproject: str
        """
        if subproject not in self.read() and subproject in self.project:
            with open(self.spath, "a+") as f:
                f.write('\n%s' % subproject)
        else:
            raise TypeError('Subproject not found')

    def remove(self, subproject):
        """Remove subproject to the search path

        :param subproject: a subproject's name
        :type subproject: str
        """
        subprojects = self.read()
        subprojects.remove(subproject)
        self._write(subprojects)

    def extend(self, subprojects):
        """Add more subprojects to the search path

        :param subprojects: a list of subproject's names
        :type subprojects: list
        """
        ms = [decode(m) for m in self.project.subprojects()]
        final = [decode(m) for m in self.read()]
        subprojects = [decode(m) for m in subprojects]
        final.extend([m for m in subprojects if m in ms and m not in final])
        self._write(final)

    def reset(self):
        """Reset to the original search path"""
        final = [self.subproject, 'PERMANENT']
        self._write(final)


if __name__ == "__main__":
    import doctest
    from grass.pygrass import utils
    from grass.script.core import run_command

    utils.create_test_vector_map(test_vector_name)
    run_command("g.region", n=50, s=0, e=60, w=0, res=1)
    run_command("r.mapcalc", expression="%s = 1" % (test_raster_name),
                overwrite=True)
    run_command("g.region", n=40, s=0, e=40, w=0, res=2)

    doctest.testmod()

    # Remove the generated vector map, if exist
    mset = utils.get_subproject_vector(test_vector_name, subproject='')
    if mset:
        run_command("g.remove", flags='f', type='vector',
                    name=test_vector_name)
    mset = utils.get_subproject_raster(test_raster_name, subproject='')
    if mset:
        run_command("g.remove", flags='f', type='raster',
                    name=test_raster_name)
