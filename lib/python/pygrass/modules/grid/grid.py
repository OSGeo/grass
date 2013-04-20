# -*- coding: utf-8 -*-
"""
Created on Thu Mar 28 11:06:00 2013

@author: pietro
"""
import os
import multiprocessing as mltp
import subprocess as sub

from grass.script.setup import write_gisrc

from grass.pygrass.gis import Mapset, Location, make_mapset
from grass.pygrass.modules import Module
from grass.pygrass.functions import get_mapset_raster

from split import split_region_tiles
from patch import patch_map


_GREG = Module('g.region')


def get_cmd(cmdd):
    """Transforma a cmd dictionary to a list of parameters"""
    cmd = [cmdd['name'], ]
    cmd.extend(("%s=%s" % (k, v) for k, v in cmdd['inputs']))
    cmd.extend(("%s=%s" % (k, v) for k, v in cmdd['outputs']))
    cmd.extend(("%s" % (flg) for flg in cmdd['flags'] if len(flg) == 1))
    cmd.extend(("--%s" % (flg[0]) for flg in cmdd['flags'] if len(flg) > 1))
    return cmd


def cmd_exe((bbox, mapnames, msetname, cmd)):
    """Create a mapset, and execute a cmd inside."""
    mset = Mapset()
    try:
        make_mapset(msetname)
    except:
        pass
    env = os.environ.copy()
    env['GISRC'] = write_gisrc(mset.gisdbase, mset.location, msetname)
    if mapnames:
        inputs = dict(cmd['inputs'])
        # reset the inputs to
        for key in mapnames:
            inputs[key] = mapnames[key]
        cmd['inputs'] = inputs.items()
        # set the region to the tile
        _GREG(env_=env, rast=key)
    else:
        # set the computational region
        _GREG(env_=env, **bbox)
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
                 split=False, debug=False, *args, **kargs):
        kargs['run_'] = False
        self.mset = Mapset()
        self.module = Module(cmd, *args, **kargs)
        self.width = width
        self.height = height
        self.overlap = overlap
        self.processes = processes
        self.bboxes = split_region_tiles(width=width, height=height,
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
        #import pdb; pdb.set_trace()
        for inmap in self.module.inputs:
            inm = self.module.inputs[inmap]
            if inm.type == 'raster' and inm.value:
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
                works.append((bbox, inms, self.msetstr % (row, col), cmd))
        return works

    def define_mapset_inputs(self):
        for inmap in self.module.inputs:
            inm = self.module.inputs[inmap]
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

        if clean:
            self.clean_location()
            self.rm_tiles()

    def patch(self):
        """Patch the final results."""
        # patch all the outputs
        for otmap in self.module.outputs:
            otm = self.module.outputs[otmap]
            if otm.type == 'raster' and otm.value:
                patch_map(otm.value, self.mset.name, self.msetstr,
                          split_region_tiles(width=self.width,
                                             height=self.height),
                          self.module.flags.overwrite)

    def rm_tiles(self):
        """Remove all the tiles."""
        # if split, remove tiles
        if self.inlist:
            grm = Module('g.remove')
            for key in self.inlist:
                grm(rast=self.inlist[key])
