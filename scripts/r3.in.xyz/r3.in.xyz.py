#!/usr/bin/env python
############################################################################
#
# MODULE:       r3.in.xyz
# AUTHOR:       M. Hamish Bowman, Dunedin, New Zealand
# PURPOSE:      Run r.in.xyz in a loop for various z-levels and construct
#		a 3D raster. Unlike r.in.xyz, reading from stdin and z-scaling
#		won't work.
#
# COPYRIGHT:    (c) 2011-2012 Hamish Bowman, and the GRASS Development Team
#		Port of r3.in.xyz(.sh) for GRASS 6.4.
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
#
#		This program is distributed in the hope that it will be useful,
#		but WITHOUT ANY WARRANTY; without even the implied warranty of
#		MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#		GNU General Public License for more details.
#
#############################################################################

#%Module
#% description: Create a 3D raster map from an assemblage of many coordinates using univariate statistics
#% keywords: raster3d, import, voxel, LiDAR
#%End
#%Flag
#% key: s
#% description: Scan data file for extent then exit
#%End
#%Flag
#% key: g
#% description: In scan mode, print using shell script style
#%End
#%Flag
#% key: i
#% description: Ignore broken lines
#%End
#%Option
#% key: input
#% type: string
#% required: yes
#% multiple: no
#% key_desc: name
#% description: ASCII file containing input data
#% gisprompt: old_file,file,input
#%End
#%Option
#% key: output
#% type: string
#% required: yes
#% multiple: no
#% key_desc: name
#% description: Name for output raster map
#% gisprompt: new,grid3,3d-raster
#%End
#%Option      
#% key: method
#% type: string
#% required: no
#% multiple: no
#% options: n,min,max,range,sum,mean,stddev,variance,coeff_var,median,percentile,skewness,trimmean
#% description: Statistic to use for raster values
#% answer: mean
#% guisection: Statistic
#%End         
#%Option      
#% key: type  
#% type: string
#% required: no
#% multiple: no
#% options: float,double
#% description: Storage type for resultant raster map
#% answer: float
#%End         
#%Option      
#% key: fs    
#% type: string
#% required: no
#% multiple: no
#% key_desc: character
#% description: Field separator
#% answer: |  
#% guisection: Input
#%End         
#%Option      
#% key: x     
#% type: integer
#% required: no
#% multiple: no
#% description: Column number of x coordinates in input file (first column is 1)
#% answer: 1  
#% guisection: Input
#%End         
#%Option      
#% key: y     
#% type: integer
#% required: no
#% multiple: no
#% description: Column number of y coordinates in input file
#% answer: 2  
#% guisection: Input
#%End         
#%Option      
#% key: z     
#% type: integer
#% required: no
#% multiple: no
#% description: Column number of data values in input file
#% answer: 3  
#% guisection: Input
#%End         
#%Option      
#% key: percent
#% type: integer
#% required: no
#% multiple: no
#% options: 1-100
#% description: Percent of map to keep in memory
#% answer: 100
#%End         
#%Option      
#% key: pth   
#% type: integer
#% required: no
#% multiple: no
#% options: 1-100
#% description: pth percentile of the values
#% guisection: Statistic
#%End         
#%Option      
#% key: trim  
#% type: double
#% required: no
#% multiple: no
#% options: 0-50
#% description: Discard <trim> percent of the smallest and <trim> percent of the largest observations
#% guisection: Statistic
#%End         
#%Option      
#% key: workers
#% type: integer
#% required: no
#% multiple: no
#% options: 1-256
#% answer: 1
#% description: Number of parallel processes to launch
#%End         


import sys
import os
from grass.script import core as grass


def cleanup():
    grass.run_command('g.mremove', flags = 'f', rast = 'tmp.r3xyz.%d.*' % os.getpid(),
		      quiet = True)


def main():
    infile = options['input']
    output = options['output']
    method = options['method']
    dtype = options['type']
    fs = options['fs']
    x = options['x']
    y = options['y']   
    z = options['z']
    percent = options['percent']
    pth = options['pth']
    trim = options['trim']
    workers = options['workers']
    scan_only = flags['s']
    shell_style = flags['g']
    ignore_broken = flags['i']

    if workers is 1 and "WORKERS" in os.environ:
	workers = int(os.environ["WORKERS"])

    if not os.path.exists(infile):
	grass.fatal(_("Unable to read input file <%s>") % infile)

    if scan_only or shell_style:
        if shell_style:
	    doShell = 'g'
	else:
	    doShell = ''
        grass.run_command('r.in.xyz', flags = 's' + doShell, input = infile,
			  output = 'dummy', fs = fs, x = x, y = y, z = z)
	sys.exit()

    addl_opts = []
    if pth:
        addl_opts.append("pth = '%s'" % pth)
    if trim:
        addl_opts.append("trim = '%s'" % trim)
    if ignore_broken:
        addl_opts.append("flags = 'i'")

    if dtype is 'float':
       data_type = 'FCELL'
    else:
       data_type = 'DCELL'

    region = grass.region(region3d = True)

    if region['nsres'] != region['nsres3'] or region['ewres'] != region['ewres3']:
        grass.run_command('g.region', flags = '3p')
        grass.fatal(_("The 2D and 3D region settings are different. Can not continue."))

    grass.verbose(_("Region bottom=%.15g  top=%.15g  vertical_cell_res=%.15g  (depths %d)")
       % (region['b'], region['t'], region['tbres'], region['depths']))

    grass.verbose(_("Creating slices ..."))

    # to avoid a point which falls exactly on a top bound from being
    # considered twice, we shrink the
    # For the top slice we keep it though, as someone scanning the bounds
    # may have set the bounds exactly to the data extent (a bad idea, but
    # it happens..)
    eps = 1.0e-15

    # if there are thousands of depths hopefully this array doesn't get too large
    # and so we don't have to worry much about storing/looping through the finished process infos.
    proc = {}

    for i in range(1, 1 + depths):
	tmp_layer_name = 'tmp.r3xyz.%d.%s' % (os.getpid(), '%05d' % i)

        zrange_min = region['b'] + (region['tbres'] * (i-1))

        if i < region['depths']:
            zrange_max = region['b'] + (region['tbres'] * i) - eps
        else:
	    zrange_max = region['b'] + (region['tbres'] * i)

        # spawn depth layerimport  job in the background
        #grass.debug("slice %d, <%s>  %% %d" % (band, image[band], band % workers))
        grass.message(_("Processing horizontal slice %d of %d [%.15g,%.15g) ..."),
		        % (i, depths, zrange_min, zrange_max))

	proc[i] = grass.start_command('r.in.xyz', input = infile, output = tmp_layer_name,
				      fs = fs, method = method, x = x, y = y, z = z,
				      percent = percent, type = data_type,
				      zrange = '%.15g,%.15g' % (zrange_min, zrange_max),
				      addl_opts)

	if i % workers is 0:
	    # wait for the ones launched so far to finish
	    for p_i in range(1, 1 + depths)[:i]:
		if not proc[p_i].stdout.closed:
		    pout[p_i] = proc[p_i].communicate()[0]
		if proc[p_i].wait() is not 0:
		    grass.fatal(_("Trouble importing data. Aborting."))


    # wait for jobs to finish, collect any stray output
    for p_i in range(1, 1 + depths):
	if not proc[p_i].stdout.closed:
	    pout[p_i] = proc[p_i].communicate()[0]
	if proc[p_i].wait() is not 0:
	    grass.fatal(_("Trouble importing data. Aborting."))

    del proc

    grass.verbose(_("Assembling 3D cube ..."))

    #input order: lower most strata first
    slices = grass.read_command('g.mlist', type = 'rast', sep = ',',
				pattern = 'tmp.r3xyz.%d.*' % os.getpid())

    if grass.run_command('r.to.rast3', input = slices, output = output) is 0:
	grass.message(_("Done. 3D raster map <%s> created.") % output)


if __name__ == "__main__":
    options, flags = grass.parser()
    atexit.register(cleanup)
    main()


