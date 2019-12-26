"""g.remove tests

(C) 2013 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

:author: Vaclav Petras
"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule
from grass.gunittest.gutils import is_map_in_mapset
from grass.gunittest.checkers import (text_to_keyvalue, keyvalue_equals,
                                      diff_keyvalue)


class RasterRenameTestCase(TestCase):
    """Test wrong input of parameters for g.list module"""

    def setUp(self):
        """Create maps in a small region.

        The raster exists must be renewed for every test.
        """
        self.use_temp_region()
        self.runModule("g.region", s=0, n=5, w=0, e=5, res=1)

        self.runModule("r.mapcalc", expression="rename_1 = 1")
        self.runModule("r.mapcalc", expression="rename_2 = 20")
        self.runModule("r.mapcalc", expression="rename_3 = 300")
        self.runModule("r.mapcalc", expression="exists = 50000")
        self.to_remove = ['rename_1', 'rename_2', 'rename_3', 'exists']

    def tearDown(self):
        """Remove temporary region and renamed maps (and also old if needed)"""
        self.runModule('g.remove', name=self.to_remove, type=['raster'], flags='f')
        self.del_temp_region()

    def test_raster(self):
        """Test that raster rename works"""
        module = SimpleModule('g.rename', raster=['rename_1', 'renamed_1'])
        self.assertModule(module)
        new_names = ['renamed_1']
        self.to_remove.extend(new_names)
        for name in new_names:
            self.assertRasterExists(name)

    def test_preserve_existing_raster(self):
        """Test that existing raster is preserved"""
        # TODO: write the same for other types
        # TODO: create a general functions to avoid duplication
        runivar = SimpleModule('r.univar', flags='g', map='exists')
        self.runModule(runivar, expecting_stdout=True)
        original_runivar = text_to_keyvalue(runivar.outputs.stdout,
                                            sep='=', skip_empty=True)
        module = SimpleModule('g.rename', raster=['rename_3', 'exists'], overwrite=False)
        self.assertModule(module)
        self.assertRasterExists('exists', msg="Destination (existing) map (to) should exist")
        self.assertRasterExists('rename_3', msg="Source map (from) should exist")
        runivar = SimpleModule('r.univar', flags='g', map='exists')
        self.runModule(runivar, expecting_stdout=True)
        new_runivar = text_to_keyvalue(runivar.outputs.stdout,
                                       sep='=', skip_empty=True)
        if not keyvalue_equals(dict_a=original_runivar, dict_b=new_runivar,
                               precision=1e-7):
            unused, missing, mismatch = diff_keyvalue(dict_a=original_runivar,
                                                      dict_b=new_runivar,
                                                      precision=1e-7)
            if mismatch:
                msg = "Raster map changed. It was probably overwritten.\n"
                msg += "Difference between r.univar of maps:\n"
                msg += "mismatch values"
                msg += " (key, reference, actual): %s\n" % mismatch
                self.fail(msg)

    def test_overwrite_existing_raster(self):
        """Test that existing raster is overridden if desired"""
        runivar_source = SimpleModule('r.univar', flags='g', map='rename_3')
        self.runModule(runivar_source, expecting_stdout=True)
        original_runivar_source = text_to_keyvalue(runivar_source.outputs.stdout,
                                                   sep='=', skip_empty=True)
        runivar_target = SimpleModule('r.univar', flags='g', map='exists')
        self.runModule(runivar_target, expecting_stdout=True)
        original_runivar_target = text_to_keyvalue(runivar_target.outputs.stdout,
                                                   sep='=', skip_empty=True)
        module = SimpleModule('g.rename', raster=['rename_3', 'exists'], overwrite=True)
        self.assertModule(module)
        self.assertRasterExists('exists', msg="Destination (here: existing) map (to) should exist after rename")
        self.assertFalse(is_map_in_mapset('rename_3', type='raster'),
                         msg="Source map (from) should not exist after rename")

        runivar = SimpleModule('r.univar', flags='g', map='exists')
        self.runModule(runivar, expecting_stdout=True)
        new_runivar = text_to_keyvalue(runivar.outputs.stdout,
                                       sep='=', skip_empty=True)

        # both these tests are probably redundant but let's test thoroughly
        if keyvalue_equals(dict_a=original_runivar_target, dict_b=new_runivar,
                           precision=1e-7):
            msg = "Raster map did not change. It probably wasn't overwritten."
            self.fail(msg)

        if not keyvalue_equals(dict_a=original_runivar_source, dict_b=new_runivar,
                               precision=1e-7):
            unused, missing, mismatch = diff_keyvalue(dict_a=original_runivar_source,
                                                      dict_b=new_runivar,
                                                      precision=1e-7)
            if mismatch:
                msg = "Destination raster map is not the same as source."
                msg += " It probably wasn't overwritten.\n"
                msg += "Difference between r.univar of maps:\n"
                msg += "mismatch values"
                msg += " (key, reference, actual): %s\n" % mismatch
                self.fail(msg)


if __name__ == '__main__':
    test()
