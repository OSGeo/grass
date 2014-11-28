#!/usr/bin/env python

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
#% keywords: display
#% keywords: vector
#%end
#%option G_OPT_V_INPUTS
#% key: map
#%end


from grass.script import core as gcore


def main():
    options, flags = gcore.parser()
    gisenv = gcore.gisenv()
    if 'MONITOR' in gisenv:
        cmd_file = gisenv['MONITOR_{monitor}_CMDFILE'.format(monitor=gisenv['MONITOR'].upper())]
        dout_cmd = 'd.what.vect'
        for param, val in options.iteritems():
            if val:
                dout_cmd += " {param}={val}".format(param=param, val=val)
        with open(cmd_file, "a") as file_:
            file_.write(dout_cmd)
    else:
        gcore.fatal(_("No graphics device selected. Use d.mon to select graphics device."))


if __name__ == "__main__":
    main()
