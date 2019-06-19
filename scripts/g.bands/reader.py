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
    """Band references reader"""
    def __init__(self):
        self._json_files = glob.glob(
            os.path.join(os.environ['GISBASE'], 'etc', 'g.bands', '*.json')
        )
        if not self._json_files:
            raise BandReaderError("No band definitions found")

        self._read_config()

    def _read_config(self):
        """Read configuration"""
        self.config = dict()
        for json_file in self._json_files:
            with open(json_file) as fd:
                config = json.load(
                    fd,
                    object_pairs_hook=OrderedDict
                )
            # check if configuration is valid
            self._check_config(config)

            self.config[os.path.basename(json_file)] = config

    @staticmethod
    def _check_config(config):
        """Check if config is valid

        :todo: check shortcut uniqueness

        :param dict config: configuration to be validated
        """
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
        """Prints band reference information to stdout.

        Can be filtered by shortcut or band identifier.

        :param str shortcut: shortcut to filter (eg. S2A) or None
        :param str band: band (eg. 1) or None
        """
        found = False
        for root in self.config.values():
            for item in root.values():
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

    def find_file(self, band_reference):
        """Find file by band reference.

        Match is case-insensitive.

        :param str band_reference: band reference identifier to search for (eg. S2A_1)

        :return str: file basename if found or None
        """
        try:
            shortcut, band = band_reference.split('_')
        except ValueError:
            raise BandReaderError("Invalid band identifier <{}>".format(
                band_reference
            ))

        for filename, config in self.config.items():
            for root in config.keys():
                if config[root]['shortcut'].upper() == shortcut.upper() and \
                   band.upper() in map(lambda x: x.upper(), config[root]['bands'].keys()):
                    return filename

        return None
