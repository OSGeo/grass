import os
import sys
import json
import glob
import re
from collections import OrderedDict

import grass.script as gs

# band reference should be required to have the format
# <shortcut>_<band>
# instead, the sensor name should be stored somewhere else,
# and band names should be STAC common names, see
# https://stacspec.org/
# https://github.com/radiantearth/stac-spec/blob/master/extensions/eo/README.md#band-object
# custom names must be possible

class BandReferenceReaderError(Exception):
    pass

class BandReferenceReader:
    """Band references reader"""

    def __init__(self):
        self._json_files = glob.glob(
            os.path.join(os.environ['GISBASE'], 'etc', 'g.bands', '*.json')
        )
        if not self._json_files:
            raise BandReferenceReaderError("No band definitions found")

        self._read_config()

    def _read_config(self):
        """Read configuration"""
        self.config = dict()
        for json_file in self._json_files:
            try:
                with open(json_file) as fd:
                    config = json.load(
                        fd,
                        object_pairs_hook=OrderedDict
                    )
            except json.decoder.JSONDecodeError as e:
                raise BandReferenceReaderError(
                    "Unable to parse '{}': {}".format(
                        json_file, e
                    ))

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
                    raise BandReferenceReaderError(
                        "Invalid band definition: <{}> is missing".format(item
                                                                          ))
            if len(items['bands']) < 1:
                raise BandReferenceReaderError(
                    "Invalid band definition: no bands defined"
                )

    @staticmethod
    def _print_band_extended(band, item):
        """Print band-specific metadata

        :param str band: band identifier
        :param str item: items to be printed out
        """
        def print_kv(k, v, indent):
            if isinstance(v, OrderedDict):
                print ('{}{}:'.format(' ' * indent * 2, k))
                for ki, vi in v.items():
                    print_kv(ki, vi, indent * 2)
            else:
                print ('{}{}: {}'.format(' ' * indent * 2, k, v))

        indent = 4
        print ('{}band: {}'.format(
            ' ' * indent, band
        ))
        for k, v in item[band].items():
            print_kv(k, v, indent)

    def _print_band(self, shortcut, band, tag=None):
        sys.stdout.write(self._band_identifier(shortcut, band))
        if tag:
            sys.stdout.write(' {}'.format(tag))
        sys.stdout.write(os.linesep)

    def print_info(self, shortcut=None, band=None, extended=False):
        """Prints band reference information to stdout.

        Can be filtered by shortcut or band identifier.

        :param str shortcut: shortcut to filter (eg. S2) or None
        :param str band: band (eg. 1) or None
        :param bool extended: print also extended metadata
        """
        found = False
        for root in self.config.values():
            for item in root.values():
                try:
                    if shortcut and re.match(shortcut, item['shortcut']) is None:
                        continue
                except re.error as e:
                    raise BandReferenceReaderError(
                        "Invalid pattern: {}".format(e)
                    )

                found = True
                if band and band not in item['bands']:
                    raise BandReferenceReaderError(
                        "Band <{}> not found in <{}>".format(
                            band, shortcut
                        ))

                # print generic information
                if extended:
                    for subitem in item.keys():
                        if subitem == 'bands':
                            # bands item is processed bellow
                            continue
                        print ('{}: {}'.format(
                            subitem, item[subitem]
                        ))

                    # print detailed band information
                    if band:
                        self._print_band_extended(band, item['bands'])
                    else:
                        for iband in item['bands']:
                            self._print_band_extended(iband, item['bands'])
                else:
                    # basic information only
                    if band:
                        self._print_band(
                            item['shortcut'], band,
                            item['bands'][band].get('tag')
                        )
                    else:
                        for iband in item['bands']:
                            self._print_band(
                                item['shortcut'], iband,
                                item['bands'][iband].get('tag')
                            )

        # raise error when defined shortcut not found
        if shortcut and not found:
            raise BandReferenceReaderError(
                "Band reference <{}> not found".format(shortcut)
            )

    def find_file(self, band_reference):
        """Find file by band reference.

        Match is case-insensitive.

        :param str band_reference: band reference identifier to search for (eg. S2_1)

        :return str: file basename if found or None
        """
        try:
            shortcut, band = band_reference.split('_')
        except ValueError:
            # raise BandReferenceReaderError("Invalid band identifier <{}>".format(
            #    band_reference
            # ))
            shortcut = "unknown"
            band = band_reference

        for filename, config in self.config.items():
            for root in config.keys():
                if config[root]['shortcut'].upper() == shortcut.upper() and \
                   band.upper() in map(lambda x: x.upper(), config[root]['bands'].keys()):
                    return filename

        return None

    def get_bands(self):
        """Get list of band identifiers.

        :return list: list of valid band identifiers
        """
        bands = []
        for root in self.config.values():
            for item in root.values():
                for band in item['bands']:
                    bands.append(
                        self._band_identifier(item['shortcut'], band)
                    )
        return bands

    @staticmethod
    def _band_identifier(shortcut, band):
        return '{}_{}'.format(shortcut, band)
