#!/usr/bin/env python3

############################################################################
#
# MODULE:    d.to.rast
# AUTHOR(S): Anna Petrasova <kratochanna gmail.com>
# PURPOSE:	 Script for exporting content of monitor to raster map
# COPYRIGHT: (C) 2014-2015 by the GRASS Development Team
#
#		This program is free software under the GNU General
#		Public License (>=v2). Read the file COPYING that
#		comes with GRASS for details.
#
#############################################################################

#%module
#% description: Saves the contents of the active display monitor to a raster map.
#% keyword: display
#% keyword: export
#% keyword: raster
#%end
#%option G_OPT_R_OUTPUT
#%end


from grass.script import core as gcore


def main():
    options, flags = gcore.parser()
    gisenv = gcore.gisenv()
    if 'MONITOR' in gisenv:
        cmd_file = gcore.parse_command('d.mon', flags='g')['cmd']
        d_cmd = 'd.to.rast'
        for param, val in options.items():
            if val:
                d_cmd += " {param}={val}".format(param=param, val=val)
        if gcore.overwrite():
            d_cmd += ' --overwrite'
        with open(cmd_file, "a") as file_:
            file_.write(d_cmd)
    else:
        gcore.fatal(_("No graphics device selected. Use d.mon to select graphics device."))


if __name__ == "__main__":
    main()
