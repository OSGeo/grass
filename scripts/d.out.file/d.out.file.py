#!/usr/bin/env python3

############################################################################
#
# MODULE: d.out.file
# AUTHOR(S): Anna Petrasova <kratochanna gmail.com>
# PURPOSE:	Script for exporting content of monitor to graphic file
# COPYRIGHT: (C) 2014-2015 by the GRASS Development Team
#
#		This program is free software under the GNU General
#		Public License (>=v2). Read the file COPYING that
#		comes with GRASS for details.
#
#############################################################################

#%module
#% description: Saves the contents of the active display monitor to a graphics file.
#% keyword: display
#% keyword: export
#% keyword: output
#%end
#%option G_OPT_F_OUTPUT
#% description: Name for output file
#% required: yes
#%end
#%option
#% key: format
#% description: Graphics file format
#% required: yes
#% options: png,jpg,bmp,gif,tif
#% answer: png
#%end
#%option
#% key: size
#% type: integer
#% key_desc: width,height
#% description: Width and height of output image
#% guisection: Images
#% required : no
#%end

from grass.script import core as gcore


def main():
    options, flags = gcore.parser()
    gisenv = gcore.gisenv()
    if 'MONITOR' in gisenv:
        cmd_file = gcore.parse_command('d.mon', flags='g')['cmd']
        dout_cmd = 'd.out.file'
        for param, val in options.items():
            if val:
                dout_cmd += " {param}={val}".format(param=param, val=val)
        with open(cmd_file, "a") as file_:
            file_.write(dout_cmd)
    else:
        gcore.fatal(_("No graphics device selected. Use d.mon to select graphics device."))


if __name__ == "__main__":
    main()
