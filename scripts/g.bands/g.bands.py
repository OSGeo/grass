#!/usr/bin/env python

############################################################################
#
# MODULE:       g.bands
# AUTHOR(S):    Martin Landa <landa.martin gmail com>
#
# PURPOSE:      Print bands references.
#
# COPYRIGHT:    (C) 2019 by mundialis GmbH & Co.KG, and the GRASS Development Team
#
#               This program is free software under the GNU General
#               Public License (>=v2). Read the file COPYING that
#               comes with GRASS for details.
#
#############################################################################

#%module
#% description: Prints bands references.
#% keyword: general
#% keyword: imagery
#% keyword: image collections
#% keyword: bands
#%end
#%option
#% key: bands
#% type: string
#% key_desc: name
#% description: Name of band identifier (example: S2A or S2A_1)
#% required: no
#% multiple: no
#%end

import os
import sys
import json
import glob
from collections import OrderedDict

import grass.script as gs

class BandReaderError(Exception):
    def __init__(self, msg):
        gs.fatal(msg)

class BandReader:
    def __init__(self):
        json_files = glob.glob(
            os.path.join(os.environ['GISBASE'], 'etc', 'g.bands', '*.json')
        )
        if not json_files:
            raise BandReaderError("No band definitions found")
        self.config = OrderedDict()
        for json_file in json_files:
            with open(json_file) as fd:
                config = json.load(
                    fd,
                    object_pairs_hook=OrderedDict
                )
                # check if configuration is valid
                self._check_config(config)

                # check configuration intersection
                config_intersection = list(set(config.keys()) & set(self.config.keys()))
                if config_intersection:
                    raise BandReaderError("Band identifiers intersection: {}".format(
                        ','.join(config_intersection)
                    ))
                # merge configs
                self.config.update(config)

    @staticmethod
    def _check_config(config):
        for items in config.values():
            for item in ('shortcut', 'bands'):
                if item not in items.keys():
                    raise BandReaderError(
                        "Invalid band definition: <{}> is missing".format(item
                ))
            if len(items['bands']) < 1:
                raise BandReaderError(
                    "Invalid band definition: no bands defined"
                )

    @staticmethod
    def _print_band(band, item):
        indent = 4
        print ('{}band: {}'.format(
            ' ' * indent, band
        ))
        for k, v in item[band].items():
            print ('{}{}: {}'.format(' ' * indent * 2, k, v))

    def print_info(self, shortcut=None, band=None):
        found = False
        for item in self.config.values():
            if shortcut and item['shortcut'] != shortcut:
                continue

            found = True
            if band and band not in item['bands']:
                raise BandReaderError(
                    "Band <{}> not found in <{}>".format(
                        band, shortcut
                    ))

            # print generic information
            for subitem in item.keys():
                if subitem == 'bands':
                    # bands item is processed bellow
                    continue
                print ('{}: {}'.format(
                    subitem, item[subitem]
                ))

            # print detailed band information
            if band:
                self._print_band(band, item['bands'])
            else:
                for iband in item['bands']:
                    self._print_band(iband, item['bands'])

        # raise error when defined shortcut not found
        if shortcut and not found:
            raise BandReaderError(
                "<{}> not found".format(shortcut)
            )

def main():
    reader = BandReader()

    band = None
    kwargs = {}
    if ',' in options['bands']:
        gs.fatal("Multiple values not supported")
    if '_' in options['bands']:
        kwargs['shortcut'], kwargs['band'] = options['bands'].split('_')
    else:
        kwargs['shortcut'] = options['bands']

    reader.print_info(**kwargs)

if __name__ == "__main__":
    options, flags = gs.parser()

    sys.exit(
        main()
    )
