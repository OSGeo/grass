"""
(C) 2013 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

:authors: Soeren Gebbert and Thomas Leppelt
"""

import grass.script
import grass.temporal as tgis
from grass.gunittest.case import TestCase
from grass.gunittest.main import test
import datetime
import os

class TestTemporalAlgebraMixedDatasets(TestCase):

    @classmethod
    def setUpClass(cls):
        """Initiate the temporal GIS and set the region
        """
        tgis.init(True) # Raise on error instead of exit(1)
        cls.use_temp_region()
        cls.runModule("g.region", n=80.0, s=0.0, e=120.0,
                                       w=0.0, t=1.0, b=0.0, res=10.0)

        cls.runModule("r3.mapcalc", overwrite=True, quiet=True, expression="a1 = 1")
        cls.runModule("r3.mapcalc", overwrite=True, quiet=True, expression="a2 = 2")
        cls.runModule("r3.mapcalc", overwrite=True, quiet=True, expression="a3 = 3")
        cls.runModule("r3.mapcalc", overwrite=True, quiet=True, expression="a4 = 4")

        tgis.open_new_stds(name="A", type="str3ds", temporaltype="absolute",
                           title="A", descr="A", semantic="field", overwrite=True)

        tgis.register_maps_in_space_time_dataset(type="raster3d", name="A", maps="a1,a2,a3,a4",
                                                 start="2001-01-01", increment="1 day", interval=True)

        cls.runModule("r.mapcalc", overwrite=True, quiet=True, expression="b1 = 5")
        cls.runModule("r.mapcalc", overwrite=True, quiet=True, expression="b2 = 6")

        tgis.open_new_stds(name="B", type="strds", temporaltype="absolute",
                           title="B", descr="B", semantic="field", overwrite=True)

        tgis.register_maps_in_space_time_dataset(type="raster", name="B", maps="b1,b2",
                                                 start="2001-01-01", increment="2 day", interval=True)

        cls.runModule("v.random", overwrite=True, quiet=True, npoints=20, seed=3, output='c1')

        tgis.open_new_stds(name="C", type="stvds", temporaltype="absolute",
                           title="B", descr="C", semantic="field", overwrite=True)

        tgis.register_maps_in_space_time_dataset(type="vector", name="C", maps="c1",
                                                 start="2001-01-02", increment="2 day", interval=True)

    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region
        """
        cls.runModule("t.remove", flags="rf", type="str3ds", inputs="A", quiet=True)
        cls.runModule("t.remove", flags="rf", type="strds", inputs="B", quiet=True)
        cls.runModule("t.remove", flags="rf", type="stvds", inputs="C", quiet=True)
        cls.del_temp_region()

    def test_temporal_select_operators1(self):
        """Testing the temporal select operator. Including temporal relations. """
        ta = tgis.TemporalAlgebraParser(run=True, debug=True)
        ta.parse(expression="R = A {:,during} stvds(C)",
                 stdstype='str3ds', basename="r", overwrite=True)

        D = tgis.open_old_stds("R", type="str3ds")
        D.select()
        self.assertEqual(D.metadata.get_number_of_maps(), 2)
        self.assertEqual(D.metadata.get_min_min(), 2)
        self.assertEqual(D.metadata.get_max_max(), 3)
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 2))
        self.assertEqual(end, datetime.datetime(2001, 1, 4))
        self.assertEqual(D.check_temporal_topology(), True)
        self.assertEqual(D.get_granularity(), u'1 day')

        ta = tgis.TemporalAlgebraParser(run=True, debug=True, dry_run=True)
        pc = ta.parse(expression="R = A {:,during} stvds(C)",
                      stdstype='str3ds', basename="r", overwrite=True)

        print(pc)

        self.assertEqual(len(pc["register"]), 2)
        self.assertEqual(pc["STDS"]["name"], "R")
        self.assertEqual(pc["STDS"]["stdstype"], "str3ds")

    def test_temporal_select_operators2(self):
        """Testing the temporal select operator. Including temporal relations. """
        ta = tgis.TemporalAlgebraParser(run=True, debug=True)
        ta.parse(expression="R = A {:,equal|during} stvds(C)",
                 stdstype='str3ds', basename="r", overwrite=True)

        D = tgis.open_old_stds("R", type="str3ds")
        D.select()
        self.assertEqual(D.metadata.get_number_of_maps(), 2)
        self.assertEqual(D.metadata.get_min_min(), 2)
        self.assertEqual(D.metadata.get_max_max(), 3)
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 2))
        self.assertEqual(end, datetime.datetime(2001, 1, 4))
        self.assertEqual(D.check_temporal_topology(), True)
        self.assertEqual(D.get_granularity(), u'1 day')

        ta = tgis.TemporalAlgebraParser(run=True, debug=True,
                                        dry_run=True)
        pc = ta.parse(expression="R = A {:,equal|during} stvds(C)",
                      stdstype='str3ds', basename="r", overwrite=True)

        print(pc)

        self.assertEqual(len(pc["register"]), 2)
        self.assertEqual(pc["STDS"]["name"], "R")
        self.assertEqual(pc["STDS"]["stdstype"], "str3ds")

    def test_temporal_select_operators3(self):
        """Testing the temporal select operator. Including temporal relations
            and negation operation. """
        ta = tgis.TemporalAlgebraParser(run=True, debug=True)
        ta.parse(expression="R = A {!:,during} stvds(C)",
                 stdstype='str3ds', basename="r", overwrite=True)

        D = tgis.open_old_stds("R", type="str3ds")
        D.select()
        self.assertEqual(D.metadata.get_number_of_maps(), 2)
        self.assertEqual(D.metadata.get_min_min(), 1)
        self.assertEqual(D.metadata.get_max_max(), 4)
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))
        self.assertEqual(D.check_temporal_topology(), True)
        self.assertEqual(D.get_granularity(), u'1 day')

    def test_temporal_select_operators4(self):
        """Testing the temporal select operator. Including temporal relations and
            temporal operators. """
        ta = tgis.TemporalAlgebraParser(run=True, debug=True)
        ta.parse(expression="V = C {:,contains} str3ds(A)",
                 stdstype='stvds', basename="r", overwrite=True)

        D = tgis.open_old_stds("V", type="stvds")
        D.select()
        self.assertEqual(D.metadata.get_number_of_maps(), 1)
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 2))
        self.assertEqual(end, datetime.datetime(2001, 1, 4))
        self.assertEqual(D.check_temporal_topology(), True)
        self.assertEqual(D.get_granularity(), u'2 days')

    def test_temporal_select_operators5(self):
        """Testing the temporal select operator. Including temporal relations and
            temporal operators. """
        ta = tgis.TemporalAlgebraParser(run=True, debug=True)
        ta.parse(expression="R = A {:,during} strds(B)",
                 stdstype='str3ds', basename="r", overwrite=True)

        D = tgis.open_old_stds("R", type="str3ds")
        D.select()
        self.assertEqual(D.metadata.get_number_of_maps(), 4)
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 5))
        self.assertEqual(D.check_temporal_topology(), True)
        self.assertEqual(D.get_granularity(), u'1 day')

    def test_temporal_hash_operator1(self):
        """Testing the hash operator function in conditional statement. """
        ta = tgis.TemporalAlgebraParser(run=True, debug=True)
        ta.parse(expression="R = if(A {#,during} stvds(C) == 1, A)",
                 stdstype='str3ds', basename="r", overwrite=True)

        D = tgis.open_old_stds("R", type="str3ds")
        D.select()
        self.assertEqual(D.metadata.get_number_of_maps(), 2)
        self.assertEqual(D.metadata.get_min_min(), 2)
        self.assertEqual(D.metadata.get_max_max(), 3)
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 2))
        self.assertEqual(end, datetime.datetime(2001, 1, 4))
        self.assertEqual(D.check_temporal_topology(), True)
        self.assertEqual(D.get_granularity(), u'1 day')

    def test_temporal_hash_operator2(self):
        """Testing the hash operator function in conditional statement. """
        ta = tgis.TemporalAlgebraParser(run=True, debug=True)
        ta.parse(expression="R = if({during}, stvds(C) {#,contains} A == 2, A)",
                 stdstype='str3ds', basename="r", overwrite=True)

        D = tgis.open_old_stds("R", type="str3ds")
        D.select()
        self.assertEqual(D.metadata.get_number_of_maps(), 2)
        self.assertEqual(D.metadata.get_min_min(), 2)
        self.assertEqual(D.metadata.get_max_max(), 3)
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 2))
        self.assertEqual(end, datetime.datetime(2001, 1, 4))
        self.assertEqual(D.check_temporal_topology(), True)
        self.assertEqual(D.get_granularity(), u'1 day')

    def test_different_stds_handling1(self):
        """Testing the handling of different stds types as output. """
        ta = tgis.TemporalAlgebraParser(run=True, debug=True)
        ta.parse(expression="R = if({during}, stvds(C) {#,contains} str3ds(A) == 2, str3ds(A))",
                 stdstype='strds', basename="r", overwrite=True)

        D = tgis.open_old_stds("R", type="str3ds")
        D.select()
        self.assertEqual(D.metadata.get_number_of_maps(), 2)
        self.assertEqual(D.metadata.get_min_min(), 2)
        self.assertEqual(D.metadata.get_max_max(), 3)
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 2))
        self.assertEqual(end, datetime.datetime(2001, 1, 4))
        self.assertEqual(D.check_temporal_topology(), True)
        self.assertEqual(D.get_granularity(), u'1 day')

    def test_different_stds_handling2(self):
        """Testing the handling of different stds types as output. """
        ta = tgis.TemporalAlgebraParser(run=True, debug=True,
                                        dry_run=True)
        pc = ta.parse(expression="R = if({during}, (stvds(C) {#,contains} str3ds(A)) == 2, str3ds(A))",
                      stdstype='strds', basename="r", overwrite=True)

        self.assertEqual(len(pc["register"]), 2)
        self.assertEqual(pc["STDS"]["name"], "R")
        self.assertEqual(pc["STDS"]["stdstype"], "strds")


if __name__ == '__main__':
    test()
