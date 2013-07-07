#!/usr/bin/env python

############################################################################
#
# MODULE:	m.proj
# AUTHOR:	M. Hamish Bowman, Dept. Marine Science, Otago Univeristy,
#		  New Zealand
#               Converted to Python by Glynn Clements
# PURPOSE:      cs2cs reprojection frontend for a list of coordinates.
#		Replacement for m.proj2 from GRASS 5
# COPYRIGHT:	(c) 2006-2009 Hamish Bowman, and the GRASS Development Team
#		This program is free software under the GNU General Public
#		License (>=v2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

# notes:
#  - cs2cs expects "x y" data so be sure to send it "lon lat" not "lat lon"
#  - if you send cs2cs a third data column, beware it might be treated as "z"
# todo:
#  - `cut` away x,y columns into a temp file, feed to cs2cs, then `paste`
#    back to input file. see method in v.in.garmin.sh. that way additional
#    numeric and string columns would survive the trip, and 3rd column would
#    not be modified as z.

#%module
#% description: Converts coordinates from one projection to another (cs2cs frontend).
#% keywords: miscellaneous
#% keywords: projection
#%end
#%option G_OPT_F_INPUT
#% description: Name of input coordinate file ('-' to read from stdin)
#% answer: -
#% guisection: Files & format
#%end
#%option G_OPT_F_OUTPUT
#% description: Name for output coordinate file (omit to send to stdout)
#% required : no
#% guisection: Files & format
#%end
#%option G_OPT_F_SEP
#% label: Field separator (format: input[,output])
#% description: Valid field separators are also "space", "tab", or "comma"
#% required : no
#% guisection: Files & format
#%end
#%option
#% key: proj_in
#% type: string
#% description: Input projection parameters (PROJ.4 style)
#% required : no
#% guisection: Projections
#%end
#%option
#% key: proj_out
#% type: string
#% description: Output projection parameters (PROJ.4 style)
#% required : no
#% guisection: Projections
#%end
#%flag
#% key: i
#% description: Use LL WGS84 as input and current location as output projection
#% guisection: Projections
#%end
#%flag
#% key: o
#% description: Use current location as input and LL WGS84 as output projection
#% guisection: Projections
#%end
#%flag
#% key: d
#% description: Output long/lat in decimal degrees, or other projections with many decimal places
#% guisection: Files & format
#%end
#%flag
#% key: e
#% description: Include input coordinates in output file
#% guisection: Files & format
#%end
#%flag
#% key: c
#% description: Include column names in output file
#% guisection: Files & format
#%end


import sys
import os
import threading
from grass.script import core as grass

class TrThread(threading.Thread):
    def __init__(self, ifs, inf, outf):
        threading.Thread.__init__(self)
        self.ifs = ifs
        self.inf = inf
        self.outf = outf

    def run(self):
        while True:
            line = self.inf.readline()
            if not line:
                break
            line = line.replace(self.ifs, ' ')
            self.outf.write(line)
            self.outf.flush()

        self.outf.close()

def main():
    input = options['input']
    output = options['output']
    fs = options['separator']
    proj_in = options['proj_in']
    proj_out = options['proj_out']
    ll_in = flags['i']
    ll_out = flags['o']
    decimal = flags['d']
    copy_input = flags['e']
    include_header = flags['c']

    #### check for cs2cs
    if not grass.find_program('cs2cs'):
	grass.fatal(_("cs2cs program not found, install PROJ.4 first: http://proj.maptools.org"))

    #### check for overenthusiasm
    if proj_in and ll_in:
	grass.fatal(_("Choose only one input parameter method"))

    if proj_out and ll_out:
	grass.fatal(_("Choose only one output parameter method")) 

    if ll_in and ll_out:
	grass.fatal(_("Choise only one auto-projection parameter method"))

    if output and not grass.overwrite() and os.path.exists(output):
	grass.fatal(_("Output file already exists")) 

    #### parse field separator
    # FIXME: input_x,y needs to split on multiple whitespace between them
    if fs == ',':
        ifs = ofs = ','
    else:
	try:
	    ifs, ofs = fs.split(',')
	except ValueError:
	    ifs = ofs = fs

    ifs = ifs.lower()
    ofs = ofs.lower()
    
    if ifs in ('space', 'tab'):
	ifs = ' '
    elif ifs == 'comma':
        ifs = ','
    else:
        if len(ifs) > 1:
            grass.warning(_("Invalid field separator, using '%s'") % ifs[0])
        try:
            ifs = ifs[0]
        except IndexError:
            grass.fatal(_("Invalid field separator '%s'") % ifs)
   
    if ofs.lower() == 'space':
        ofs = ' '
    elif ofs.lower() == 'tab':
        ofs = '\t'
    elif ofs.lower() == 'comma':
        ofs = ','
    else:
        if len(ofs) > 1:
            grass.warning(_("Invalid field separator, using '%s'") % ofs[0])
        try:
            ofs = ofs[0]
        except IndexError:
            grass.fatal(_("Invalid field separator '%s'") % ifs)

    #### set up projection params
    s = grass.read_command("g.proj", flags='j')
    kv = grass.parse_key_val(s)
    if "XY location" in kv['+proj'] and (ll_in or ll_out):
	grass.fatal(_("Unable to project to or from a XY location")) 

    in_proj = None

    if ll_in:
	in_proj = "+proj=longlat +datum=WGS84"
	grass.verbose("Assuming LL WGS84 as input, current projection as output ")

    if ll_out:
	in_proj = grass.read_command('g.proj', flags = 'jf')

    if proj_in:
	in_proj = proj_in

    if not in_proj:
	grass.verbose("Assuming current location as input")
        in_proj = grass.read_command('g.proj', flags = 'jf')
    
    in_proj = in_proj.strip()
    grass.verbose("Input parameters: '%s'" % in_proj)

    out_proj = None

    if ll_out:
	out_proj = "+proj=longlat +datum=WGS84"
	grass.verbose("Assuming current projection as input, LL WGS84 as output ")

    if ll_in:
	out_proj = grass.read_command('g.proj', flags = 'jf')

    if proj_out:
	out_proj = proj_out

    if not out_proj:
	grass.fatal(_("Missing output projection parameters "))
    out_proj = out_proj.strip()
    grass.verbose("Output parameters: '%s'" % out_proj)

    #### set up input file
    if input == '-':
	infile = None
	inf = sys.stdin
    else:
	infile = input
	if not os.path.exists(infile):
	    grass.fatal(_("Unable to read input data"))
	inf = file(infile)
	grass.debug("input file=[%s]" % infile)

    #### set up output file
    if not output:
	outfile = None
	outf = sys.stdout
    else:
	outfile = output
	outf = open(outfile, 'w')
	grass.debug("output file=[%s]" % outfile) 

    #### set up output style
    if not decimal:
	outfmt = ["-w5"]
    else:
	outfmt = ["-f", "%.8f"]
    if not copy_input:
	copyinp = []
    else:
	copyinp = ["-E"]

    #### do the conversion
    # Convert cs2cs DMS format to GRASS DMS format:
    #   cs2cs | sed -e 's/d/:/g' -e "s/'/:/g"  -e 's/"//g'

    cmd = ['cs2cs'] + copyinp + outfmt + in_proj.split() + ['+to'] + out_proj.split()
    p = grass.Popen(cmd, stdin = grass.PIPE, stdout = grass.PIPE)

    tr = TrThread(ifs, inf, p.stdin)
    tr.start()

    if not copy_input:
	if include_header:
	    outf.write("x%sy%sz\n" % (ofs, ofs))
	for line in p.stdout:
	    xy, z = line.split(' ', 1)
	    x, y = xy.split('\t')
	    outf.write('%s%s%s%s%s\n' % \
                       (x.strip(), ofs, y.strip(), ofs, z.strip()))
    else:
	if include_header:
	    outf.write("input_x%sinput_y%sx%sy%sz\n" % (ofs, ofs, ofs, ofs))
	for line in p.stdout:
            inXYZ, x, rest = line.split('\t')
            inX, inY = inXYZ.split(' ')[:2]
	    y, z = rest.split(' ', 1)
	    outf.write('%s%s%s%s%s%s%s%s%s\n' % \
                       (inX.strip(), ofs, inY.strip(), ofs, x.strip(), \
		        ofs, y.strip(), ofs, z.strip()))

    p.wait()

    if p.returncode != 0:
	grass.warning(_("Projection transform probably failed, please investigate"))

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
