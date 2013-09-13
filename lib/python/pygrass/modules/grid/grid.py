# -*- coding: utf-8 -*-
"""
Created on Thu Mar 28 11:06:00 2013

@author: pietro
"""
import os
import multiprocessing as mltp
import subprocess as sub
import shutil as sht

from grass.script.setup import write_gisrc

from grass.pygrass.gis import Mapset, Location, make_mapset
from grass.pygrass.gis.region import Region
from grass.pygrass.modules import Module
from grass.pygrass.functions import get_mapset_raster

from split import split_region_tiles
from patch import patch_map


def select(parms, ptype):
    """Select only a  certain type of parameters. ::

        >>> slp = Module('r.slope.aspect',
        ...              elevation='ele', slope='slp', aspect='asp',
        ...              run_=False)
        >>> for rast in select(slp.outputs, 'raster'):
        ...     print rast
        ...
        slp
        asp

    """
    for k in parms:
        par = parms[k]
        if par.type == ptype or par.typedesc == ptype and par.value:
            if par.multiple:
                for p in par.value:
                    yield p
            else:
                yield par.value


def copy_mapset(mapset, path):
    """Copy mapset to another place without copying raster and vector data.
    """
    per_old = os.path.join(mapset.gisdbase, mapset.location, 'PERMANENT')
    per_new = os.path.join(path, 'PERMANENT')
    map_old = mapset.path()
    map_new = os.path.join(path, mapset.name)
    if not os.path.isdir(per_new):
        os.makedirs(per_new)
    if not os.path.isdir(map_new):
        os.mkdir(map_new)
    for f in (fi for fi in os.listdir(per_old) if fi.isupper()):
        sht.copy(os.path.join(per_old, f), per_new)
    for f in (fi for fi in os.listdir(map_old) if fi.isupper()):
        sht.copy(os.path.join(map_old, f), map_new)
    gisdbase, location = os.path.split(path)
    return Mapset(mapset.name, location, gisdbase)


def copy_raster(rasters, src, dst, region=None, gisrc_dst=None):
    """Copy raster from one mapset to another, crop the raster to the region.
    """
    # set region
    if region:
        region.set_current()

    nam = "copy%d__%s" % (id(dst), '%s')
    expr = "%s=%s"

    # instantiate modules
    mpclc = Module('r.mapcalc')
    rpck = Module('r.pack')
    rupck = Module('r.unpack')
    rm = Module('g.remove')

    # get and set GISRC
    gisrc_src = os.environ['GISRC']
    gisrc_dst = gisrc_dst if gisrc_dst else write_gisrc(dst.gisdbase,
                                                        dst.location,
                                                        dst.name)

    pdst = dst.path()
    for rast in rasters:
        # change gisdbase to src
        os.environ['GISRC'] = gisrc_src
        src.current()
        name = nam % rast
        mpclc(expression=expr % (name, rast), overwrite=True)
        file_dst = "%s.pack" % os.path.join(pdst, name)
        rpck(input=name, output=file_dst, overwrite=True)
        rm(rast=name)
        # change gisdbase to dst
        os.environ['GISRC'] = gisrc_dst
        dst.current()
        rupck(input=file_dst, output=rast, overwrite=True)
        os.remove(file_dst)
    return gisrc_src, gisrc_dst


def get_cmd(cmdd):
    """Transforma a cmd dictionary to a list of parameters"""
    cmd = [cmdd['name'], ]
    cmd.extend(("%s=%s" % (k, v) for k, v in cmdd['inputs']
                if not isinstance(v, list)))
    cmd.extend(("%s=%s" % (k, ','.join(vals if isinstance(vals[0], str)
                                       else map(repr, vals)))
                for k, vals in cmdd['inputs']
                if isinstance(vals, list)))
    cmd.extend(("%s=%s" % (k, v) for k, v in cmdd['outputs']
                if not isinstance(v, list)))
    cmd.extend(("%s=%s" % (k, ','.join(map(repr, vals)))
                for k, vals in cmdd['outputs']
                if isinstance(vals, list)))
    cmd.extend(("%s" % (flg) for flg in cmdd['flags'] if len(flg) == 1))
    cmd.extend(("--%s" % (flg[0]) for flg in cmdd['flags'] if len(flg) > 1))
    return cmd


def cmd_exe(args):
    """Create a mapset, and execute a cmd inside."""
    bbox, mapnames, msetname, cmd = args
    mset = Mapset()
    try:
        make_mapset(msetname)
    except:
        pass
    ms = Mapset(msetname)
    ms.visible.extend(mset.visible)
    env = os.environ.copy()
    env['GISRC'] = write_gisrc(mset.gisdbase, mset.location, msetname)
    if mapnames:
        inputs = dict(cmd['inputs'])
        # reset the inputs to
        for key in mapnames:
            inputs[key] = mapnames[key]
        cmd['inputs'] = inputs.items()
        # set the region to the tile
        sub.Popen(['g,region', 'rast=%s' % key], env=env).wait()
    else:
        #reg = Region() nsres=reg.nsres, ewres=reg.ewres,
        # set the computational region
        lcmd = ['g.region', ]
        lcmd.extend(["%s=%s" % (k, v) for k, v in bbox.iteritems()])
        sub.Popen(lcmd, env=env).wait()
    # run the grass command
    sub.Popen(get_cmd(cmd), env=env).wait()


class GridModule(object):
    """Run GRASS raster commands in a multiproccessing mode.

    Parameters
    -----------

    cmd: raster GRASS command
        Only command staring with r.* are valid.
    width: integer
        Width of the tile, in pixel.
    height: integer
        Height of the tile, in pixel.
    overlap: integer
        Overlap between tiles, in pixel.
    nthreads: number of threads
        Default value is equal to the number of processor available.
    split: boolean
        If True use r.tile to split all the inputs.
    run_: boolean
        If False only instantiate the object.
    args and kargs: cmd parameters
        Give all the parameters to the command.

    Examples
    --------

    ::

        >>> grd = GridModule('r.slope.aspect',
        ...                  width=500, height=500, overlap=2,
        ...                  processes=None, split=True,
        ...                  elevation='elevation',
        ...                  slope='slope', aspect='aspect', overwrite=True)
        >>> grd.run()
    """
    def __init__(self, cmd, width=None, height=None, overlap=0, processes=None,
                 split=False, debug=False, region=None, move=None, log=False,
                 start_row=0, start_col=0, out_prefix='',
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
        self.n_mset = None
        self.gisrc_src = self.gisrc_dst = None
        self.log = log
        if move:
            self.n_mset = copy_mapset(self.mset, move)
            rasters = select(self.module.inputs, 'raster')
            self.gisrc_src, self.gisrc_dst = copy_raster(rasters,
                                                         self.mset,
                                                         self.n_mset,
                                                         region=self.region)

        self.bboxes = split_region_tiles(region=region,
                                         width=width, height=height,
                                         overlap=overlap)
        self.msetstr = cmd.replace('.', '') + "_%03d_%03d"
        self.inlist = None
        if split:
            self.split()
        self.debug = debug

    def clean_location(self):
        """Remove all created mapsets."""
        mapsets = Location().mapsets(self.msetstr.split('_')[0] + '_*')
        for mset in mapsets:
            Mapset(mset).delete()

    def split(self):
        """Split all the raster inputs using r.tile"""
        rtile = Module('r.tile')
        inlist = {}
        for inm in select(self.module.inputs, 'raster'):
            rtile(input=inm.value, output=inm.value,
                  width=self.width, height=self.height,
                  overlap=self.overlap)
            patt = '%s-*' % inm.value
            inlist[inm.value] = sorted(self.mset.glist(type='rast',
                                                       pattern=patt))
        self.inlist = inlist

    def get_works(self):
        """Return a list of tuble with the parameters for cmd_exe function"""
        works = []
        reg = Region()
        cmd = self.module.get_dict()
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
                works.append((bbox, inms,
                              self.msetstr % (self.start_row + row,
                                              self.start_col + col),
                              cmd))
        return works

    def define_mapset_inputs(self):
        for inmap in self.module.inputs:
            inm = self.module.inputs[inmap]
            # (inm.type == 'raster' or inm.typedesc == 'group') and inm.value:
            if inm.type == 'raster' and inm.value:
                if '@' not in inm.value:
                    mset = get_mapset_raster(inm.value)
                    inm.value = inm.value + '@%s' % mset

    def run(self, patch=True, clean=True):
        """Run the GRASS command."""
        self.module.flags.overwrite = True
        self.define_mapset_inputs()
        if self.debug:
            for wrk in self.get_works():
                cmd_exe(wrk)
        else:
            pool = mltp.Pool(processes=self.processes)
            result = pool.map_async(cmd_exe, self.get_works())
            result.wait()
            if not result.successful():
                raise RuntimeError

        if patch:
            self.patch()

        if self.n_mset is not None:
            # move the outputs to the original mapset
            outputs = [self.out_prefix + o
                       for o in select(self.module.outputs, 'raster')]
            copy_raster(outputs, self.n_mset, self.mset, self.region,
                        self.gisrc_src)

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
                    fp = open(os.path.join(dirpath,
                                           self.out_prefix + par.value), 'w+')
                    fp.close()

        if clean:
            self.clean_location()
            self.rm_tiles()

    def patch(self):
        """Patch the final results."""
        # patch all the outputs
        for otmap in self.module.outputs:
            otm = self.module.outputs[otmap]
            if otm.typedesc == 'raster' and otm.value:
                patch_map(otm.value,
                          self.mset.name, self.msetstr,
                          split_region_tiles(width=self.width,
                                             height=self.height),
                          self.module.flags.overwrite,
                          self.start_row, self.start_col, self.out_prefix)

    def rm_tiles(self):
        """Remove all the tiles."""
        # if split, remove tiles
        if self.inlist:
            grm = Module('g.remove')
            for key in self.inlist:
                grm(rast=self.inlist[key])
