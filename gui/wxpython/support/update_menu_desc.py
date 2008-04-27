"""
@brief Reads menu data from menudata.py and for each command updates
its description (based on interface description).

Updated menu data is printed to stdout.

Support script for wxGUI.

COPYRIGHT:  (C) 2008 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

Usage: python update_menu_desc.py

@author Martin Landa <landa.martin gmail.com>
"""

import os
import sys

import xml.sax
import xml.sax.handler
HandlerBase=xml.sax.handler.ContentHandler
from xml.sax import make_parser

def read_menudata():
    menu = menudata.Data() # get menu data
    for mainItem in menu.GetMenu()[0]:
        print '                (_("%s"), (' % mainItem[0]
        for item1 in mainItem[1]:
            if len(item1[0]) == 0: # separator
                print '                        ("","","", ""),'
                continue
            if len(item1) == 4:
                desc = get_description(item1[1:])
                print '                        (_("%s"),' % item1[0]
                if desc and desc != item1[1]:
                    print '                         _("%s"),' % desc
                else:
                    print '                         _("%s"),' % item1[1]
                print '                         "%s",' % item1[2]
                print '                         "%s"),' % item1[3]
            else: # submenu
                print '                        (_("%s"), (' % item1[0]
                for item2 in item1[1]:
                    if len(item2[0]) == 0: # separator
                        print '                                ("","","", ""),'
                        continue
                    desc = get_description(item2[1:])
                    print '                                (_("%s"),' % item2[0]
                    if desc and desc != item2[1]:
                        print '                                 _("%s"),' % desc
                    else:
                        print '                                 _("%s"),' % item2[1]
                    print '                                 "%s",' % item2[2]
                    print '                                 "%s"),' % item2[3]
                print '                                )'
                print '                         ),'
        print '                        )'
        print '                 ),'

def get_description(item):
    """Return command desctiption based on interface
    description"""
    print 
    desc, type, cmd = item
    if type in ("self.OnMenuCmd", "self.RunMenuCmd"):
        module = cmd.split(' ')[0]
        grass_task = menuform.grassTask()
        handler = menuform.processTask(grass_task)
        xml.sax.parseString(menuform.getInterfaceDescription(module), handler )

        return grass_task.description.replace('"', '\\"')

    return None

def main(argv=None):
    if argv is None:
        argv = sys.argv

    if len(argv) != 1:
        print >> sys.stderr, __doc__
        return 0

    ### i18N
    import gettext
    gettext.install('grasswxpy', os.path.join(os.getenv("GISBASE"), 'locale'), unicode=True)

    read_menudata()

    return 0

if __name__ == '__main__':
    if os.getenv("GISBASE") is None:
        print >> sys.stderr, "You must be in GRASS GIS to run this program."
        sys.exit(1)

    sys.path.append('../gui_modules')
    import menudata
    import menuform

    sys.exit(main())
