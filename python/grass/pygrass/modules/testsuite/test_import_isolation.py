# -*- coding: utf-8 -*-
"""
Authors:   pietro

Copyright: (C) 2015 pietro

           This program is free software under the GNU General Public
           License (>=v2). Read the file COPYING that comes with GRASS
           for details.

Created on  Wed Jul 15 11:34:32 2015
"""
from __future__ import absolute_import

import sys
import fnmatch

from grass.gunittest.case import TestCase
from grass.gunittest.main import test


def check(*patterns):
    """Return a set of the imported libraries that soddisfies several patterns.
    """
    result = []
    imports = sorted(sys.modules.keys())
    for pattern in patterns:
        result.extend(fnmatch.filter(imports, pattern))
    return set(result)


class TestImportIsolation(TestCase):
    patterns = ['grass.lib*']

    def test_import_isolation(self):
        """Check that modules  classes are not using ctypes"""
        isolate = set()
        self.assertEqual(isolate, check(*self.patterns),
                         msg="Test isolation before any import.")
        # same import done in __init__ file
        from grass.pygrass.modules.interface import Module, ParallelModuleQueue
        from grass.pygrass.modules import shortcuts
        self.assertEqual(isolate, check(*self.patterns),
                         msg="Test isolation after import Module.")
        # test the other way round
        from grass.pygrass.vector import VectorTopo
        self.assertNotEqual(isolate, check(*self.patterns),
                            msg=("Test the isolation is broken, therefore "
                                 "the defined patterns are correct"))


if __name__ == '__main__':
    test()
