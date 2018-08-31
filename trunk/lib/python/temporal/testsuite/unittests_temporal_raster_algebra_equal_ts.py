"""
(C) 2014 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

:authors: Soeren Gebbert and Thomas Leppelt
"""

import datetime
import grass.temporal as tgis
from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class TestTemporalRasterAlgebraImplicitAggregation(TestCase):

    @classmethod
    def setUpClass(cls):
        """Initiate the temporal GIS and set the region
        """
        tgis.init(True) # Raise on error instead of exit(1)
        cls.use_temp_region()
        cls.runModule("g.region", n=80.0, s=0.0, e=120.0,
                                       w=0.0, t=1.0, b=0.0, res=10.0)

        cls.runModule("r.mapcalc", overwrite=True, quiet=True, expression="a1 = 1")
        cls.runModule("r.mapcalc", overwrite=True, quiet=True, expression="a2 = 2")
        cls.runModule("r.mapcalc", overwrite=True, quiet=True, expression="a3 = 3")
        cls.runModule("r.mapcalc", overwrite=True, quiet=True, expression="a4 = 4")
        cls.runModule("r.mapcalc", overwrite=True, quiet=True, expression="singletmap = 100")
        cls.runModule("r.mapcalc", overwrite=True, quiet=True, expression="singlemap = 1000")

        tgis.open_new_stds(name="A", type="strds", temporaltype="absolute",
                                         title="A", descr="A", semantic="field", overwrite=True)

        tgis.register_maps_in_space_time_dataset(type="raster", name="A", maps="a1,a2,a3,a4",
                                                 start="2001-01-01", interval=False)
        tgis.register_maps_in_space_time_dataset(type="raster", name=None,  maps="singletmap",
                                                start="2001-01-01")

    def tearDown(self):
        self.runModule("t.remove", flags="rf", inputs="R", quiet=True)

    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region
        """
        cls.runModule("t.remove", flags="rf", inputs="A", quiet=True)
        cls.runModule("t.unregister", maps="singletmap", quiet=True)
        cls.del_temp_region()

    def test_simple_operator(self):
        """Test implicit aggregation

        R = A + A

        result with implicit aggregation to:

        r1 = a1 + a1 + a2 + a3 + a4    # 11
        r2 = a2 + a1 + a2 + a3 + a4    # 12
        r3 = a3 + a1 + a2 + a3 + a4    # 13
        r4 = a4 + a1 + a2 + a3 + a4    # 14

        """
        ta = tgis.TemporalRasterAlgebraParser(run=True, debug=True)
        ta.parse(expression="R = A + A",   basename="r", overwrite=True)

        D = tgis.open_old_stds("R", type="strds")
        D.select()

        self.assertEqual(D.metadata.get_number_of_maps(), 4)
        self.assertEqual(D.metadata.get_min_min(), 11)
        self.assertEqual(D.metadata.get_max_max(), 14)
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual( D.check_temporal_topology(),  False)
        self.assertEqual(D.get_granularity(),  None)

    def test_complex_operator(self):
        """Test implicit aggregation

        R = A {+,equal,l} A

        result with implicit aggregation to:

        r1 = a1 + a1 + a2 + a3 + a4    # 11
        r2 = a2 + a1 + a2 + a3 + a4    # 12
        r3 = a3 + a1 + a2 + a3 + a4    # 13
        r4 = a4 + a1 + a2 + a3 + a4    # 14

        """
        ta = tgis.TemporalRasterAlgebraParser(run=True, debug=True)
        ta.parse(expression="R = A {+, equal,l} A",   basename="r", overwrite=True)

        D = tgis.open_old_stds("R", type="strds")
        D.select()

        self.assertEqual(D.metadata.get_number_of_maps(), 4)
        self.assertEqual(D.metadata.get_min_min(), 11)
        self.assertEqual(D.metadata.get_max_max(), 14)
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual( D.check_temporal_topology(),  False)
        self.assertEqual(D.get_granularity(),  None)

    def test_single_map_complex_operator(self):
        """Test implicit aggregation

        R = singletmap {+,equal,l} A

        result with implicit aggregation to:

        r1 = singletmap + a1 + a2 + a3 + a4    # 110
        r2 = singletmap + a1 + a2 + a3 + a4    # 110
        r3 = singletmap + a1 + a2 + a3 + a4    # 110
        r4 = singletmap + a1 + a2 + a3 + a4    # 110

        """
        ta = tgis.TemporalRasterAlgebraParser(run=True, debug=True)
        ta.parse(expression="R = tmap(singletmap) {+, equal,l} A",   basename="r", overwrite=True)

        D = tgis.open_old_stds("R", type="strds")
        D.select()

        self.assertEqual(D.metadata.get_number_of_maps(), 1)
        self.assertEqual(D.metadata.get_min_min(), 110)
        self.assertEqual(D.metadata.get_max_max(), 110)
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual( D.check_temporal_topology(),  True)
        self.assertEqual(D.get_granularity(),  None)

    def test_single_map_simple_operator(self):
        """Test implicit aggregation

        R = singletmap + A

        result with implicit aggregation to:

        r1 = singletmap + a1 + a2 + a3 + a4    # 110
        r2 = singletmap + a1 + a2 + a3 + a4    # 110
        r3 = singletmap + a1 + a2 + a3 + a4    # 110
        r4 = singletmap + a1 + a2 + a3 + a4    # 110

        """
        ta = tgis.TemporalRasterAlgebraParser(run=True, debug=True)
        ta.parse(expression="R = tmap(singletmap) + A",   basename="r", overwrite=True)

        D = tgis.open_old_stds("R", type="strds")
        D.select()

        self.assertEqual(D.metadata.get_number_of_maps(), 1)
        self.assertEqual(D.metadata.get_min_min(), 110)
        self.assertEqual(D.metadata.get_max_max(), 110)
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual( D.check_temporal_topology(),  True)
        self.assertEqual(D.get_granularity(),  None)

    def test_single_map_complex_operator_right_ts(self):
        """Test implicit aggregation

        TODO: Ist this the correct result? Implicit aggregation and full permutation?

        R = singletmap {+,equal,r} A

        result with implicit aggregation to:
        r1 = singletmap + a1 + a2 + a3 + a4    # 110
        r2 = singletmap + a1 + a2 + a3 + a4    # 110
        r3 = singletmap + a1 + a2 + a3 + a4    # 110
        r4 = singletmap + a1 + a2 + a3 + a4    # 110

        """
        ta = tgis.TemporalRasterAlgebraParser(run=True, debug=True)
        ta.parse(expression="R = tmap(singletmap) {+, equal,r} A",   basename="r", overwrite=True)

        D = tgis.open_old_stds("R", type="strds")
        D.select()

        self.assertEqual(D.metadata.get_number_of_maps(), 4)
        self.assertEqual(D.metadata.get_min_min(), 110)
        self.assertEqual(D.metadata.get_max_max(), 110)
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual( D.check_temporal_topology(),  False)
        self.assertEqual(D.get_granularity(),  None)


if __name__ == '__main__':
    test()
