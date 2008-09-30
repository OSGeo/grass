#!/usr/bin/env python

############################################################################
#
# MODULE:	m.proj
# AUTHOR:	M. Hamish Bowman, Dept. Marine Science, Otago Univeristy,
#		  New Zealand
#               Converted to Python by Glynn Clements
# PURPOSE:      cs2cs reprojection frontend for a list of coordinates.
#		Replacement for m.proj2 from GRASS 5
# COPYRIGHT:	(c) 2006,2008 Hamish Bowman, and the GRASS Development Team
#		This program is free software under the GNU General Public
#		License (>=v2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

# notes:
#  - cs2cs expects "x y" data so be sure to send it "lon lat" not "lat lon"
#  - if you send cs2cs a third data column, beware it might be treated as "z"

#%Module
#%  description: Convert coordinates from one projection to another (cs2cs frontend).
#%  keywords: miscellaneous, projection
#%End
#%option
#% key: input
#% type: string
#% gisprompt: old_file,file,file
#% description: Input coordinate file (omit to read from stdin)
#% required : no
#% key_desc : filename
#%end
#%option
#% key: output
#% type: string
#% gisprompt: new_file,file,file
#% description: Output coordinate file (omit to send to stdout)
#% required : no
#% key_desc : filename
#%end
#%option
#% key: fs
#% type: string
#% description: Field separator
#% required : no
#% key_desc : character
#% answer : |
#%end
#%option
#% key: proj_in
#% type: string
#% description: Input projection parameters (PROJ.4 style)
#% required : no
#%end
#%option
#% key: proj_out
#% type: string
#% description: Output projection parameters (PROJ.4 style)
#% required : no
#%end
#%flag
#% key: i
#% description: Use LL WGS84 as input and current location as output projection
#%end
#%flag
#% key: o
#% description: Use current location as input and LL WGS84 as output projection
#%end
#%flag
#% key: d
#% description: Output long/lat in decimal degrees or other projections with many decimal places
#%end

import sys
import os
import subprocess
import grass

def main():
    input = options['input']
    output = options['output']
    fs = options['fs']
    proj_in = options['proj_in']
    proj_out = options['proj_out']
    ll_in = flags['i']
    ll_out = flags['o']
    decimal = flags['d']

    #### check for cs2cs
    if not grass.find_program('cs2cs'):
	grass.fatal("cs2cs program not found, install PROJ.4 first: http://proj.maptools.org")

    #### check for overenthusiasm
    if proj_in and ll_in:
	grass.fatal("Chose only one input parameter method")

    if proj_out and ll_out:
	grass.fatal("Chose only one output parameter method") 

    if ll_in and ll_out:
	grass.fatal("Chose only one auto-projection parameter method")

    if output and not grass.overwrite() and os.path.exists(output):
	grass.fatal("Output file already exists") 

    #### parse field separator
    if fs.lower() in ["space", "tab"]:
	fs = ' '
    else:
	fs = fs[0]

    #### set up projection params
    s = grass.read_command("g.proj", flags='j')
    kv = grass.parse_key_val(s)
    if "XY location" in kv['+proj'] and (ll_in or ll_out):
	grass.fatal("Cannot project to or from a XY location.") 

    in_proj = None

    if ll_in:
	in_proj = "+proj=longlat +datum=WGS84"
	grass.verbose("Assuming LL WGS84 as input, current projection as output.")

    if ll_out:
	in_proj = grass.read_command('g.proj', flags = 'jf')

    if proj_in:
	in_proj = proj_in

    if not in_proj:
	grass.fatal("Missing input projection parameters.")
    in_proj = in_proj.strip()

    out_proj = None

    if ll_out:
	out_proj = "+proj=longlat +datum=WGS84"
	grass.verbose("Assuming current projection as input, LL WGS84 as output.")

    if ll_in:
	out_proj = grass.read_command('g.proj', flags = 'jf')

    if proj_out:
	out_proj = proj_out

    if not out_proj:
	grass.fatal("Missing output projection parameters.")
    out_proj = out_proj.strip()
    grass.verbose("output parameters=[%s]" % out_proj)

    #### set up input file
    if input in ['', '-']:
	infile = None
	inf = sys.stdin
    else:
	infile = input
	if not os.path.exists(infile):
	    grass.fatal("Unable to read input data.") 
	inf = file(infile)
	grass.verbose("input file=[%s]" % infile)

    #### set up output file
    if not output:
	outfile = None
	outf = sys.stdout
    else:
	outfile = output
	outf = file(outfile)
	grass.message("output file=[%s]" % outfile) 

    #### set up output style
    if not decimal:
	outfmt = []
    else:
	outfmt = ["-f", "%.8f"]

    #### do the conversion
    # Convert cs2cs DMS format to GRASS DMS format:
    #   cs2cs | sed -e 's/d/:/g' -e "s/'/:/g"  -e 's/"//g'

    cmd = ['cs2cs'] + outfmt + in_proj.split() + ['+to'] + out_proj.split()
    p = subprocess.Popen(cmd, stdin = subprocess.PIPE, stdout = outf)

    while True:
	line = inf.readline()
	if not line:
	    break
	line = line.replace(fs, ' ')
	p.stdin.write(line)
	p.stdin.flush()

    p.stdin.close()

    exitcode = p.wait()

    if exitcode != 0:
	grass.warning("Projection transform probably failed, please investigate.")

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
