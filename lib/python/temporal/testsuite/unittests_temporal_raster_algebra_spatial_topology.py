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


class TestTemporalRasterAlgebraSpatialTopology(TestCase):

    @classmethod
    def setUpClass(cls):
        """Initiate the temporal GIS and set the region
        """
        tgis.init(True) # Raise on error instead of exit(1)
        cls.use_temp_region()
        cls.runModule("g.region", n=30.0, s=0.0, e=30.0, w=0.0, t=1.0, b=0.0, res=10.0)
        cls.runModule("r.mapcalc", overwrite=True, quiet=True, expression="a1 = 1")
        cls.runModule("r.mapcalc", overwrite=True, quiet=True, expression="b1 = 1")

        cls.runModule("g.region", n=50.0, s=20.0, e=50.0, w=20.0, t=1.0, b=0.0, res=10.0)
        cls.runModule("r.mapcalc", overwrite=True, quiet=True, expression="a2 = 2")
        cls.runModule("r.mapcalc", overwrite=True, quiet=True, expression="b2 = 2")

        cls.runModule("g.region", n=40.0, s=25.0, e=40.0, w=25.0, t=1.0, b=0.0, res=10.0)
        cls.runModule("r.mapcalc", overwrite=True, quiet=True, expression="a3 = 3")
        cls.runModule("r.mapcalc", overwrite=True, quiet=True, expression="b3 = 3")

        cls.runModule("g.region", n=60.0, s=40.0, e=60.0, w=40.0, t=1.0, b=0.0, res=10.0)
        cls.runModule("r.mapcalc", overwrite=True, quiet=True, expression="a4 = 4")
        cls.runModule("r.mapcalc", overwrite=True, quiet=True, expression="b4 = 4")

        cls.runModule("r.mapcalc", overwrite=True, quiet=True, expression="singletmap = 100")
        cls.runModule("r.mapcalc", overwrite=True, quiet=True, expression="singlemap = 1000")

        tgis.open_new_stds(name="A", type="strds", temporaltype="absolute",
                           title="A", descr="A", semantic="field", overwrite=True)

        tgis.open_new_stds(name="B", type="strds", temporaltype="absolute",
                           title="B", descr="B", semantic="field", overwrite=True)

        tgis.register_maps_in_space_time_dataset(type="raster", name="A", maps="a1,a2,a3,a4",
                                                 start="2001-01-01", interval=False)

        tgis.register_maps_in_space_time_dataset(type="raster", name="B", maps="b1,b2,b3,b4",
                                                 start="2001-01-01", interval=False)

        tgis.register_maps_in_space_time_dataset(type="raster", name=None,  maps="singletmap",
                                                 start="2001-01-01")

    def tearDown(self):
        self.runModule("t.remove", flags="rf", inputs="R", quiet=True)
        pass

    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region
        """
        cls.runModule("t.remove", flags="rf", inputs="A,B", quiet=True)
        cls.runModule("t.unregister", maps="singletmap", quiet=True)
        cls.del_temp_region()

    def test_equal_equivalent_sum(self):
        """Spatial topology distinction with equal timestamps

        STRDS A and B have identical time stamps, hence the differentiation
        is based on the spatial topological relations

        R = A {+,equal|equivalent,l} B

        spatial equal extents pairs: a1,b1  a2,b2  a3,b3  a4,b4

        r0 = a1 + b1  # 2
        r1 = a2 + b2  # 4
        r2 = a3 + b3  # 6
        r3 = a4 + b4  # 8

        """
        ta = tgis.TemporalRasterAlgebraParser(run=True, debug=True, spatial=True)
        ta.parse(expression="R = A {+,equal|equivalent,l} B",   basename="r", overwrite=True)

        D = tgis.open_old_stds("R", type="strds")
        D.select()

        self.assertEqual(D.metadata.get_number_of_maps(), 4)
        self.assertEqual(D.metadata.get_min_min(), 2)
        self.assertEqual(D.metadata.get_max_max(), 8)
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual( D.check_temporal_topology(),  False)
        self.assertEqual(D.get_granularity(),  None)

    def test_equal_overlap_sum(self):
        """Spatial topology distinction with equal timestamps

        STRDS A and B have identical time stamps, hence the differentiation
        is based on the spatial topological relations

        R = A {+,equal|overlap,l} B

        spatial overlap extents pairs: a3,b1 a1,b2,b3 a2,b1,b4 a4,b2

        r0 = a3 + b1       # 4
        r1 = a1 + b2 + b3  # NULL
        r2 = a2 + b1 + b4  # NULL
        r3 = a4 + b2       # 6

        """
        ta = tgis.TemporalRasterAlgebraParser(run=True, debug=True, spatial=True)
        ta.parse(expression="R = A {+,equal|overlap,l} B",   basename="r", overwrite=True)

        D = tgis.open_old_stds("R", type="strds")
        D.select()

        self.assertEqual(D.metadata.get_number_of_maps(), 2)
        self.assertEqual(D.metadata.get_min_min(), 4)
        self.assertEqual(D.metadata.get_max_max(), 6)
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual( D.check_temporal_topology(),  False)
        self.assertEqual(D.get_granularity(),  None)

    def test_equal_overlap_sum_with_null(self):
        """Spatial topology distinction with equal timestamps

        STRDS A and B have identical time stamps, hence the differentiation
        is based on the spatial topological relations

        R = A {+,equal|cover,l} B

        spatial overlap extents pairs: a3,b1 a1,b2,b3 a2,b1,b4 a4,b2

        r0 = a3 + b1       # 4
        r1 = a1 + b2 + b3  # NULL
        r2 = a2 + b1 + b4  # NULL
        r3 = a4 + b2       # 6

        """
        ta = tgis.TemporalRasterAlgebraParser(run=True, debug=True, spatial=True, register_null=True)
        ta.parse(expression="R = A {+,equal|overlap,l} B", basename="r", overwrite=True)

        D = tgis.open_old_stds("R", type="strds")
        D.select()

        self.assertEqual(D.metadata.get_number_of_maps(), 4)
        self.assertEqual(D.metadata.get_min_min(), 4)
        self.assertEqual(D.metadata.get_max_max(), 6)
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual( D.check_temporal_topology(),  False)
        self.assertEqual(D.get_granularity(),  None)

    def test_equal_contain_sum(self):
        """Spatial topology distinction with equal timestamps

        STRDS A and B have identical time stamps, hence the differentiation
        is based on the spatial topological relations

        R = A {+,equal|overlap,l} B

        spatial overlap extents pairs: a2,b3

        r0 = a2 + b3       # 4

        """
        ta = tgis.TemporalRasterAlgebraParser(run=True, debug=True, spatial=True)
        ta.parse(expression="R = A {+,equal|contain,l} B",   basename="r", overwrite=True)

        D = tgis.open_old_stds("R", type="strds")
        D.select()

        self.assertEqual(D.metadata.get_number_of_maps(), 1)
        self.assertEqual(D.metadata.get_min_min(), 5)
        self.assertEqual(D.metadata.get_max_max(), 5)
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual( D.check_temporal_topology(),  True)
        self.assertEqual(D.get_granularity(),  None)

    def test_equal_equivalent_contain_sum(self):
        """Spatial topology distinction with equal timestamps

        STRDS A and B have identical time stamps, hence the differentiation
        is based on the spatial topological relations

        R = A {+,equal|equivalent|contain,l} B

        spatial overlap extents pairs: a2,b3
        spatial equal extents pairs: a1,b1  a2,b2  a3,b3  a4,b4

        r0 = a1 + b1       # 2
        r1 = a2 + b2 + b3  # 7
        r2 = a3 + b3       # 6
        r3 = a4 + b4       # 8

        """
        ta = tgis.TemporalRasterAlgebraParser(run=True, debug=True, spatial=True)
        ta.parse(expression="R = A {+,equal|equivalent|contain,l} B",   basename="r", overwrite=True)

        D = tgis.open_old_stds("R", type="strds")
        D.select()

        self.assertEqual(D.metadata.get_number_of_maps(), 4)
        self.assertEqual(D.metadata.get_min_min(), 2)
        self.assertEqual(D.metadata.get_max_max(), 8)
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual( D.check_temporal_topology(),  False)
        self.assertEqual(D.get_granularity(),  None)

    def test_equal_equivalent_compare(self):
        """Test implicit aggregation

        STRDS A and B have identical time stamps, hence the differentiation
        is based on the spatial topological relations

        R = if({equal|equivalent}, A > 0 {&&,equal|equivalent} B < 10, A {+,equal|equivalent,l} B

        spatial equal extents pairs: a1,b1  a2,b2  a3,b3  a4,b4

        r0 = a1 + b1  # 2
        r1 = a2 + b2  # 4
        r2 = a3 + b3  # 6
        r3 = a4 + b4  # 8

        """
        ta = tgis.TemporalRasterAlgebraParser(run=True, debug=True, spatial=True)
        ta.parse(expression="R = if({equal|equivalent}, A > 0 {&&,equal|equivalent} B < 10, A {+,equal|equivalent,l} B)",
                 basename="r", overwrite=True)

        D = tgis.open_old_stds("R", type="strds")
        D.select()

        self.assertEqual(D.metadata.get_number_of_maps(), 4)
        self.assertEqual(D.metadata.get_min_min(), 2)
        self.assertEqual(D.metadata.get_max_max(), 8)
        start, end = D.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual( D.check_temporal_topology(),  False)
        self.assertEqual(D.get_granularity(),  None)


if __name__ == '__main__':
    test()
