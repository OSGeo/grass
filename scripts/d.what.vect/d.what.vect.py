#!/usr/bin/env python3

############################################################################
#
# MODULE:    d.what.vect
# AUTHOR(S): Anna Petrasova <kratochanna gmail.com>
# PURPOSE:	 Script for querying vector maps in d.mon
# COPYRIGHT: (C) 2014 by the GRASS Development Team
#
#		This program is free software under the GNU General
#		Public License (>=v2). Read the file COPYING that
#		comes with GRASS for details.
#
#############################################################################

#%module
#% description: Allows the user to interactively query vector map layers at user-selected locations.
#% keyword: display
#% keyword: vector
#%end
#%option G_OPT_V_INPUTS
#% key: map
#%end


from grass.script import core as gcore


def main():
    options, flags = gcore.parser()
    gisenv = gcore.gisenv()
    if 'MONITOR' in gisenv:
        cmd_file = gcore.parse_command('d.mon', flags='g').get('cmd', None)
        if not cmd_file:
            gcore.fatal(_("Unable to open file '%s'") % cmd_file)
        dout_cmd = 'd.what.vect'
        for param, val in options.items():
            if val:
                dout_cmd += " {param}={val}".format(param=param, val=val)
        with open(cmd_file, "a") as file_:
            file_.write(dout_cmd)
    else:
        gcore.fatal(_("No graphics device selected. Use d.mon to select graphics device."))


if __name__ == "__main__":
    main()
