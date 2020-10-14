"""
TEST:      test_g.search.modules.py

AUTHOR(S): Jachym Cepicky <jachym.cepicky gmail com>

PURPOSE:   Test g.search.modules script outputs

COPYRIGHT: (C) 2015 Jachym Cepicky, and by the GRASS Development Team

           This program is free software under the GNU General Public
           License (>=v2). Read the file COPYING that comes with GRASS
           for details.
"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule
from grass.script.utils import decode

import unittest

try:
    has_termcolor = True
    import termcolor
except ImportError:
    has_termcolor = False


class TestSearchModule(TestCase):

    def test_all_output(self):
        module = SimpleModule('g.search.modules', flags="g")
        self.assertModule(module)
        stdout = decode(module.outputs.stdout).split()
        # we expect at least 100 hits since we search for all modules
        assert(len(stdout) > 100)

    def test_terminal_output(self):
        """ """
        module = SimpleModule('g.search.modules', keyword="water")
        self.assertModule(module)
        stdout = decode(module.outputs.stdout)
        self.assertEqual(stdout.split()[0], 'r.basins.fill')

    def test_json_output(self):
        import json
        module = SimpleModule('g.search.modules', keyword="water", flags="j")
        self.assertModule(module)
        stdout = json.loads(decode(module.outputs.stdout))
        self.assertEqual(len(stdout), 6, 'Six modules found')
        self.assertEqual(stdout[3]['name'], 'r.water.outlet', 'r.water.outlet')
        self.assertTrue('keywords' in stdout[3]['attributes'])

    def test_shell_output(self):
        module = SimpleModule('g.search.modules', keyword="water", flags="g")
        self.assertModule(module)
        stdout = decode(module.outputs.stdout).split()
        self.assertEqual(len(stdout), 6)
        self.assertEqual(stdout[3], 'r.water.outlet')

    @unittest.skipUnless(has_termcolor,
                         "not supported in this library version")
    def test_colored_terminal(self):
        module = SimpleModule('g.search.modules', keyword="water", flags="c")
        self.assertModule(module)
        stdout = decode(module.outputs.stdout).split()
        self.assertEqual(stdout[0],
                         termcolor.colored('r.basins.fill',
                                           attrs=['bold']))

    def test_manual_pages(self):
        module = SimpleModule('g.search.modules', keyword="kapri", flags="gm")
        self.assertModule(module)
        stdout = decode(module.outputs.stdout).split()
        self.assertEqual(len(stdout), 2)

if __name__ == '__main__':
    test()
