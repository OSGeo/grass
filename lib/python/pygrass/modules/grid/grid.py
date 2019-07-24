# -*- coding: utf-8 -*-
from __future__ import (nested_scopes, generators, division, absolute_import,
                        with_statement, print_function, unicode_literals)
import os
import sys
import multiprocessing as mltp
import subprocess as sub
import shutil as sht

from grass.script.setup import write_gisrc

from grass.pygrass.gis import Mapset, Location
from grass.pygrass.gis.region import Region
from grass.pygrass.modules import Module
from grass.pygrass.utils import get_mapset_raster, findmaps

from grass.pygrass.modules.grid.split import split_region_tiles
from grass.pygrass.modules.grid.patch import rpatch_map


def select(parms, ptype):
    """Select only a  certain type of parameters.

    :param parms: a DictType parameter with inputs or outputs of a Module class
    :type parms: DictType parameters
    :param ptype: String define the type of parameter that we want to select,
                  valid ptype are: 'raster', 'vector', 'group'
    :type ptype: str
    :returns: An iterator with the value of the parameter.

    >>> slp = Module('r.slope.aspect',
    ...              elevation='ele', slope='slp', aspect='asp',
    ...              run_=False)
    >>> for rast in select(slp.outputs, 'raster'):
    ...     print(rast)
    ...
    slp
    asp
    """
    for k in parms:
        par = parms[k]
        if par.type == ptype or par.typedesc == ptype and par.value:
            if par.multiple:
                for val in par.value:
                    yield val
            else:
                yield par.value


def copy_special_mapset_files(path_src, path_dst):
    """Copy all the special GRASS files that are contained in
    a mapset to another mapset

    :param path_src: the path to the original mapset
    :type path_src: str
    :param path_dst: the path to the new mapset
    :type path_dst: str
    """
    for fil in (fi for fi in os.listdir(path_src) if fi.isupper()):
        sht.copy(os.path.join(path_src, fil), path_dst)


def copy_mapset(mapset, path):
    """Copy mapset to another place without copying raster and vector data.

    :param mapset: a Mapset instance to copy
    :type mapset: Mapset object
    :param path: path where the new mapset must be copied
    :type path: str
    :returns: the instance of the new Mapset.


    >>> from grass.script.core import gisenv
    >>> mname = gisenv()['MAPSET']
    >>> mset = Mapset()
    >>> mset.name == mname
    True
    >>> import tempfile as tmp
    >>> import os
    >>> path = os.path.join(tmp.gettempdir(), 'my_loc', 'my_mset')
    >>> copy_mapset(mset, path)                           # doctest: +ELLIPSIS
    Mapset(...)
    >>> sorted(os.listdir(path))                          # doctest: +ELLIPSIS
    [...'PERMANENT'...]
    >>> sorted(os.listdir(os.path.join(path, 'PERMANENT')))
    ['DEFAULT_WIND', 'PROJ_INFO', 'PROJ_UNITS', 'VAR', 'WIND']
    >>> sorted(os.listdir(os.path.join(path, mname)))   # doctest: +ELLIPSIS
    [...'SEARCH_PATH',...'WIND']
    >>> import shutil
    >>> shutil.rmtree(path)

    """
    per_old = os.path.join(mapset.gisdbase, mapset.location, 'PERMANENT')
    per_new = os.path.join(path, 'PERMANENT')
    map_old = mapset.path()
    map_new = os.path.join(path, mapset.name)
    if not os.path.isdir(per_new):
        os.makedirs(per_new)
    if not os.path.isdir(map_new):
        os.mkdir(map_new)
    copy_special_mapset_files(per_old, per_new)
    copy_special_mapset_files(map_old, map_new)
    gisdbase, location = os.path.split(path)
    return Mapset(mapset.name, location, gisdbase)


def read_gisrc(gisrc):
    """Read a GISRC file and return a tuple with the mapset, location
    and gisdbase.

    :param gisrc: the path to GISRC file
    :type gisrc: str
    :returns: a tuple with the mapset, location and gisdbase

    >>> import os
    >>> from grass.script.core import gisenv
    >>> genv = gisenv()
    >>> (read_gisrc(os.environ['GISRC']) == (genv['MAPSET'],
    ...                                      genv['LOCATION_NAME'],
    ...                                      genv['GISDBASE']))
    True
    """
    with open(gisrc, 'r') as gfile:
        gis = dict([(k.strip(), v.strip())
                    for k, v in [row.split(':', 1) for row in gfile]])
    return gis['MAPSET'], gis['LOCATION_NAME'], gis['GISDBASE']


def get_mapset(gisrc_src, gisrc_dst):
    """Get mapset from a GISRC source to a GISRC destination.

    :param gisrc_src: path to the GISRC source
    :type gisrc_src: str
    :param gisrc_dst: path to the GISRC destination
    :type gisrc_dst: str
    :returns: a tuple with Mapset(src), Mapset(dst)

    """
    msrc, lsrc, gsrc = read_gisrc(gisrc_src)
    mdst, ldst, gdst = read_gisrc(gisrc_dst)
    path_src = os.path.join(gsrc, lsrc, msrc)
    path_dst = os.path.join(gdst, ldst, mdst)
    if not os.path.isdir(path_dst):
        os.makedirs(path_dst)
        copy_special_mapset_files(path_src, path_dst)
    src = Mapset(msrc, lsrc, gsrc)
    dst = Mapset(mdst, ldst, gdst)
    visible = [m for m in src.visible]
    if src.name not in visible:
        visible.append(src.name)
    dst.visible.extend(visible)
    return src, dst


def copy_groups(groups, gisrc_src, gisrc_dst, region=None):
    """Copy group from one mapset to another, crop the raster to the region

    :param groups: a list of strings with the group that must be copied
                   from a master to another.
    :type groups: list of strings
    :param gisrc_src: path of the GISRC file from where we want to copy the groups
    :type gisrc_src: str
    :param gisrc_dst: path of the GISRC file where the groups will be created
    :type gisrc_dst: str
    :param region: a region like object or a dictionary with the region
                   parameters that will be used to crop the rasters of the
                   groups
    :type region: Region object or dictionary
    :returns: None

    """
    env = os.environ.copy()
    # instantiate modules
    get_grp = Module('i.group', flags='lg', stdout_=sub.PIPE, run_=False)
    set_grp = Module('i.group')
    get_grp.run_ = True
    rmloc = lambda r: r.split('@')[0] if '@' in r else r

    src = read_gisrc(gisrc_src)
    dst = read_gisrc(gisrc_dst)
    rm = True if src[2] != dst[2] else False
    all_rasts = [r[0]
                 for r in findmaps('raster', location=dst[1], gisdbase=dst[2])]
    for grp in groups:
        # change gisdbase to src
        env['GISRC'] = gisrc_src
        get_grp(group=grp, env_=env)
        rasts = [r for r in get_grp.outputs.stdout.split()]
        # change gisdbase to dst
        env['GISRC'] = gisrc_dst
        rast2cp = [r for r in rasts if rmloc(r) not in all_rasts]
        if rast2cp:
            copy_rasters(rast2cp, gisrc_src, gisrc_dst, region=region)
        set_grp(group=grp,
                input=[rmloc(r) for r in rasts] if rast2cp or rm else rasts,
                env_=env)


def set_region(region, gisrc_src, gisrc_dst, env):
    """Set a region into two different mapsets.

    :param region: a region like object or a dictionary with the region
                   parameters that will be used to crop the rasters of the
                   groups
    :type region: Region object or dictionary
    :param gisrc_src: path of the GISRC file from where we want to copy the groups
    :type gisrc_src: str
    :param gisrc_dst: path of the GISRC file where the groups will be created
    :type gisrc_dst: str
    :param env:
    :type env:
    :returns: None
    """
    reg_str = "g.region n=%(north)r s=%(south)r " \
              "e=%(east)r w=%(west)r " \
              "nsres=%(nsres)r ewres=%(ewres)r"
    reg_cmd = reg_str % dict(region.items())
    env['GISRC'] = gisrc_src
    sub.Popen(reg_cmd, shell=True, env=env)
    env['GISRC'] = gisrc_dst
    sub.Popen(reg_cmd, shell=True, env=env)


def copy_rasters(rasters, gisrc_src, gisrc_dst, region=None):
    """Copy rasters from one mapset to another, crop the raster to the region.

    :param rasters: a list of strings with the raster map that must be copied
                    from a master to another.
    :type rasters: list
    :param gisrc_src: path of the GISRC file from where we want to copy the groups
    :type gisrc_src: str
    :param gisrc_dst: path of the GISRC file where the groups will be created
    :type gisrc_dst: str
    :param region: a region like object or a dictionary with the region
                   parameters that will be used to crop the rasters of the
                   groups
    :type region: Region object or dictionary
    :returns: None
    """
    env = os.environ.copy()
    if region:
        set_region(region, gisrc_src, gisrc_dst, env)

    path_dst = os.path.join(*read_gisrc(gisrc_dst)[::-1])
    nam = "copy%d__%s" % (id(gisrc_dst), '%s')

    # instantiate modules
    mpclc = Module('r.mapcalc')
    rpck = Module('r.pack')
    rupck = Module('r.unpack')
    remove = Module('g.remove')

    for rast in rasters:
        rast_clean = rast.split('@')[0] if '@' in rast else rast
        # change gisdbase to src
        env['GISRC'] = gisrc_src
        name = nam % rast_clean
        mpclc(expression="%s=%s" % (name, rast), overwrite=True, env_=env)
        file_dst = "%s.pack" % os.path.join(path_dst, name)
        rpck(input=name, output=file_dst, overwrite=True, env_=env)
        remove(flags='f', type='raster', name=name, env_=env)
        # change gisdbase to dst
        env['GISRC'] = gisrc_dst
        rupck(input=file_dst, output=rast_clean, overwrite=True, env_=env)
        os.remove(file_dst)


def copy_vectors(vectors, gisrc_src, gisrc_dst):
    """Copy vectors from one mapset to another, crop the raster to the region.

    :param vectors: a list of strings with the vector map that must be copied
                    from a master to another.
    :type vectors: list
    :param gisrc_src: path of the GISRC file from where we want to copy the groups
    :type gisrc_src: str
    :param gisrc_dst: path of the GISRC file where the groups will be created
    :type gisrc_dst: str
    :returns: None
    """
    env = os.environ.copy()
    path_dst = os.path.join(*read_gisrc(gisrc_dst))
    nam = "copy%d__%s" % (id(gisrc_dst), '%s')

    # instantiate modules
    vpck = Module('v.pack')
    vupck = Module('v.unpack')
    remove = Module('g.remove')

    for vect in vectors:
        # change gisdbase to src
        env['GISRC'] = gisrc_src
        name = nam % vect
        file_dst = "%s.pack" % os.path.join(path_dst, name)
        vpck(input=name, output=file_dst, overwrite=True, env_=env)
        remove(flags='f', type='vector', name=name, env_=env)
        # change gisdbase to dst
        env['GISRC'] = gisrc_dst
        vupck(input=file_dst, output=vect, overwrite=True, env_=env)
        os.remove(file_dst)


def get_cmd(cmdd):
    """Transform a cmd dictionary to a list of parameters. It is useful to
    pickle a Module class and cnvert into a string that can be used with
    `Popen(get_cmd(cmdd), shell=True)`.

    :param cmdd: a module dictionary with all the parameters
    :type cmdd: dict

    >>> slp = Module('r.slope.aspect',
    ...              elevation='ele', slope='slp', aspect='asp',
    ...              overwrite=True, run_=False)
    >>> get_cmd(slp.get_dict())  # doctest: +ELLIPSIS
    ['r.slope.aspect', 'elevation=ele', 'format=degrees', ..., '--o']
    """
    cmd = [cmdd['name'], ]
    cmd.extend(("%s=%s" % (k, v) for k, v in cmdd['inputs']
                if not isinstance(v, list)))
    cmd.extend(("%s=%s" % (k, ','.join(vals if isinstance(vals[0], str)
                                       else [repr(v) for v in vals]))
                for k, vals in cmdd['inputs']
                if isinstance(vals, list)))
    cmd.extend(("%s=%s" % (k, v) for k, v in cmdd['outputs']
                if not isinstance(v, list)))
    cmd.extend(("%s=%s" % (k, ','.join([repr(v) for v in vals]))
                for k, vals in cmdd['outputs'] if isinstance(vals, list)))
    cmd.extend(("-%s" % (flg) for flg in cmdd['flags'] if len(flg) == 1))
    cmd.extend(("--%s" % (flg[0]) for flg in cmdd['flags'] if len(flg) > 1))
    return cmd


def cmd_exe(args):
    """Create a mapset, and execute a cmd inside.

    :param args: is a tuple that contains several information see below
    :type args: tuple
    :returns: None

    The puple has to contain:

    - bbox (dict): a dict with the region parameters (n, s, e, w, etc.)
      that we want to set before to apply the command.
    - mapnames (dict): a dictionary to substitute the input if the domain has
      been split in several tiles.
    - gisrc_src (str): path of the GISRC file from where we want to copy the
      groups.
    - gisrc_dst (str): path of the GISRC file where the groups will be created.
    - cmd (dict): a dictionary with all the parameter of a GRASS module.
    - groups (list): a list of strings with the groups that we want to copy in
      the mapset.

    """
    bbox, mapnames, gisrc_src, gisrc_dst, cmd, groups = args
    src, dst = get_mapset(gisrc_src, gisrc_dst)
    env = os.environ.copy()
    env['GISRC'] = gisrc_dst
    shell = True if sys.platform == 'win32' else False
    if mapnames:
        inputs = dict(cmd['inputs'])
        # reset the inputs to
        for key in mapnames:
            inputs[key] = mapnames[key]
        cmd['inputs'] = inputs.items()
        # set the region to the tile
        sub.Popen(['g.region', 'raster=%s' % key], shell=shell, env=env).wait()
    else:
        # set the computational region
        lcmd = ['g.region', ]
        lcmd.extend(["%s=%s" % (k, v) for k, v in bbox.items()])
        sub.Popen(lcmd, shell=shell, env=env).wait()
    if groups:
        copy_groups(groups, gisrc_src, gisrc_dst)
    # run the grass command
    sub.Popen(get_cmd(cmd), shell=shell, env=env).wait()
    # remove temp GISRC
    os.remove(gisrc_dst)


class GridModule(object):
    # TODO maybe also i.* could be supported easily
    """Run GRASS raster commands in a multiprocessing mode.

    :param cmd: raster GRASS command, only command staring with r.* are valid.
    :type cmd: str
    :param width: width of the tile, in pixel
    :type width: int
    :param height: height of the tile, in pixel.
    :type height: int
    :param overlap: overlap between tiles, in pixel.
    :type overlap: int
    :param processes: number of threads, default value is equal to the number
                      of processor available.
    :param split: if True use r.tile to split all the inputs.
    :type split: bool
    :param mapset_prefix: if specified created mapsets start with this prefix
    :type mapset_prefix: str
    :param run_: if False only instantiate the object
    :type run_: bool
    :param args: give all the parameters to the command
    :param kargs: give all the parameters to the command

    >>> grd = GridModule('r.slope.aspect',
    ...                  width=500, height=500, overlap=2,
    ...                  processes=None, split=False,
    ...                  elevation='elevation',
    ...                  slope='slope', aspect='aspect', overwrite=True)
    >>> grd.run()
    """
    def __init__(self, cmd, width=None, height=None, overlap=0, processes=None,
                 split=False, debug=False, region=None, move=None, log=False,
                 start_row=0, start_col=0, out_prefix='', mapset_prefix=None,
                 *args, **kargs):
        kargs['run_'] = False
        self.mset = Mapset()
        self.module = Module(cmd, *args, **kargs)
        self.width = width
        self.height = height
        self.overlap = overlap
        self.processes = processes
        self.region = region if region else Region()
        self.start_row = start_row
        self.start_col = start_col
        self.out_prefix = out_prefix
        self.log = log
        self.move = move
        self.gisrc_src = os.environ['GISRC']
        self.n_mset, self.gisrc_dst = None, None
        if self.move:
            self.n_mset = copy_mapset(self.mset, self.move)
            self.gisrc_dst = write_gisrc(self.n_mset.gisdbase,
                                         self.n_mset.location,
                                         self.n_mset.name)
            rasters = [r for r in select(self.module.inputs, 'raster')]
            if rasters:
                copy_rasters(rasters, self.gisrc_src, self.gisrc_dst,
                             region=self.region)
            vectors = [v for v in select(self.module.inputs, 'vector')]
            if vectors:
                copy_vectors(vectors, self.gisrc_src, self.gisrc_dst)
            groups = [g for g in select(self.module.inputs, 'group')]
            if groups:
                copy_groups(groups, self.gisrc_src, self.gisrc_dst,
                            region=self.region)
        self.bboxes = split_region_tiles(region=region,
                                         width=width, height=height,
                                         overlap=overlap)
        if mapset_prefix:
            self.msetstr = mapset_prefix + "_%03d_%03d"
        else:
            self.msetstr = cmd.replace('.', '') + "_%03d_%03d"
        self.inlist = None
        if split:
            self.split()
        self.debug = debug

    def __del__(self):
        if self.gisrc_dst:
            # remove GISRC file
            os.remove(self.gisrc_dst)

    def clean_location(self, location=None):
        """Remove all created mapsets.

        :param location: a Location instance where we are running the analysis
        :type location: Location object
        """
        if location is None:
            if self.n_mset:
                self.n_mset.current()
            location = Location()

        mapsets = location.mapsets(self.msetstr.split('_')[0] + '_*')
        for mset in mapsets:
            Mapset(mset).delete()
        if self.n_mset and self.n_mset.is_current():
            self.mset.current()

    def split(self):
        """Split all the raster inputs using r.tile"""
        rtile = Module('r.tile')
        inlist = {}
        for inm in select(self.module.inputs, 'raster'):
            rtile(input=inm.value, output=inm.value,
                  width=self.width, height=self.height,
                  overlap=self.overlap)
            patt = '%s-*' % inm.value
            inlist[inm.value] = sorted(self.mset.glist(type='raster',
                                                       pattern=patt))
        self.inlist = inlist

    def get_works(self):
        """Return a list of tuble with the parameters for cmd_exe function"""
        works = []
        reg = Region()
        if self.move:
            mdst, ldst, gdst = read_gisrc(self.gisrc_dst)
        else:
            ldst, gdst = self.mset.location, self.mset.gisdbase
        cmd = self.module.get_dict()
        groups = [g for g in select(self.module.inputs, 'group')]
        for row, box_row in enumerate(self.bboxes):
            for col, box in enumerate(box_row):
                inms = None
                if self.inlist:
                    inms = {}
                    cols = len(box_row)
                    for key in self.inlist:
                        indx = row * cols + col
                        inms[key] = "%s@%s" % (self.inlist[key][indx],
                                               self.mset.name)
                # set the computational region, prepare the region parameters
                bbox = dict([(k[0], str(v)) for k, v in box.items()[:-2]])
                bbox['nsres'] = '%f' % reg.nsres
                bbox['ewres'] = '%f' % reg.ewres
                new_mset = self.msetstr % (self.start_row + row,
                                           self.start_col + col),
                works.append((bbox, inms,
                              self.gisrc_src,
                              write_gisrc(gdst, ldst, new_mset),
                              cmd, groups))
        return works

    def define_mapset_inputs(self):
        """Add the mapset information to the input maps
        """
        for inmap in self.module.inputs:
            inm = self.module.inputs[inmap]
            if inm.type in ('raster', 'vector') and inm.value:
                if '@' not in inm.value:
                    mset = get_mapset_raster(inm.value)
                    inm.value = inm.value + '@%s' % mset

    def run(self, patch=True, clean=True):
        """Run the GRASS command

        :param patch: set False if you does not want to patch the results
        :type patch: bool
        :param clean: set False if you does not want to remove all the stuff
                      created by GridModule
        :type clean: bool
        """
        self.module.flags.overwrite = True
        self.define_mapset_inputs()
        if self.debug:
            for wrk in self.get_works():
                cmd_exe(wrk)
        else:
            pool = mltp.Pool(processes=self.processes)
            result = pool.map_async(cmd_exe, self.get_works())
            result.wait()
            pool.close()
            pool.join()
            if not result.successful():
                raise RuntimeError(_("Execution of subprocesses was not successful"))

        if patch:
            if self.move:
                os.environ['GISRC'] = self.gisrc_dst
                self.n_mset.current()
                self.patch()
                os.environ['GISRC'] = self.gisrc_src
                self.mset.current()
                # copy the outputs from dst => src
                routputs = [self.out_prefix + o
                            for o in select(self.module.outputs, 'raster')]
                copy_rasters(routputs, self.gisrc_dst, self.gisrc_src)
            else:
                self.patch()

        if self.log:
            # record in the temp directory
            from grass.lib.gis import G_tempfile
            tmp, dummy = os.path.split(G_tempfile())
            tmpdir = os.path.join(tmp, self.module.name)
            for k in self.module.outputs:
                par = self.module.outputs[k]
                if par.typedesc == 'raster' and par.value:
                    dirpath = os.path.join(tmpdir, par.name)
                    if not os.path.isdir(dirpath):
                        os.makedirs(dirpath)
                    fil = open(os.path.join(dirpath,
                                            self.out_prefix + par.value), 'w+')
                    fil.close()

        if clean:
            self.clean_location()
            self.rm_tiles()
            if self.n_mset:
                gisdbase, location = os.path.split(self.move)
                self.clean_location(Location(location, gisdbase))
                # rm temporary gis_rc
                os.remove(self.gisrc_dst)
                self.gisrc_dst = None
                sht.rmtree(os.path.join(self.move, 'PERMANENT'))
                sht.rmtree(os.path.join(self.move, self.mset.name))

    def patch(self):
        """Patch the final results."""
        bboxes = split_region_tiles(width=self.width, height=self.height)
        loc = Location()
        mset = loc[self.mset.name]
        mset.visible.extend(loc.mapsets())
        noutputs = 0
        for otmap in self.module.outputs:
            otm = self.module.outputs[otmap]
            if otm.typedesc == 'raster' and otm.value:
                rpatch_map(otm.value,
                           self.mset.name, self.msetstr, bboxes,
                           self.module.flags.overwrite,
                           self.start_row, self.start_col, self.out_prefix)
                noutputs += 1
        if noutputs < 1:
            msg = 'No raster output option defined for <{}>'.format(self.module.name)
            if self.module.name == 'r.mapcalc':
                msg += '. Use <{}.simple> instead'.format(self.module.name)
            raise RuntimeError(msg)

    def rm_tiles(self):
        """Remove all the tiles."""
        # if split, remove tiles
        if self.inlist:
            grm = Module('g.remove')
            for key in self.inlist:
                grm(flags='f', type='raster', name=self.inlist[key])
