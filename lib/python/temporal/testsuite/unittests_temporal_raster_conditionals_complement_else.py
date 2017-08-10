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


class TestTemporalRasterAlgebraConditionalComplements(TestCase):

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
        cls.runModule("r.mapcalc", overwrite=True, quiet=True, expression="a5 = 5")
        cls.runModule("r.mapcalc", overwrite=True, quiet=True, expression="a6 = 6")
        cls.runModule("r.mapcalc", overwrite=True, quiet=True, expression="b1 = 7")
        cls.runModule("r.mapcalc", overwrite=True, quiet=True, expression="b2 = 8")

        tgis.open_new_stds(name="A", type="strds", temporaltype="absolute",
                                         title="A", descr="A", semantic="field", overwrite=True)
        tgis.open_new_stds(name="B", type="strds", temporaltype="absolute",
                                         title="B", descr="B", semantic="field", overwrite=True)

        tgis.register_maps_in_space_time_dataset(type="raster", name="A", maps="a1,a2,a3,a4,a5,a6",
                                                 start="2001-01-01", increment="1 day", interval=True)
        tgis.register_maps_in_space_time_dataset(type="raster", name="B", maps="b1,b2",
                                                 start="2001-01-02", increment="2 day", interval=True)

    def tearDown(self):
        self.runModule("t.remove", flags="rf", inputs="R", quiet=True)

    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region
        """
        cls.runModule("t.remove", flags="rf", inputs="A,B", quiet=True)
        cls.del_temp_region()

    def test_temporal_conditional_complement(self):
        """Test the conditional expression that evaluate if then else statements
        so that the else statement is a complement operation

        R = if({equal|contains}, B {#,contains,r} A == 2, B {+,contains,l} A, A)

        IF B contains two maps from A
        THEN add maps from A and B that fulfill the B contains A condition using implicit aggregation
        ELSE copy the maps from A

        The connection between the if then else statements are the topological relations equal and contains

        r0 = a1            # 1
        r1 = b1 + a2 + a3  # 12
        r2 = b2 + a4 + a5  # 16
        r4 = a6            # 6

        """
        tra = tgis.TemporalRasterAlgebraParser(run = True, debug = True)
        tra.parse(expression='R = if({equal|contains}, B {#,contains,r} A == 2, B {+,contains,l} A, A)',
                  basename="r", overwrite=True)

        R = tgis.open_old_stds("R", type="strds")
        R.select()
        self.assertEqual(R.metadata.get_number_of_maps(), 4)
        self.assertEqual(R.metadata.get_min_min(), 1)
        self.assertEqual(R.metadata.get_max_max(), 16)
        start, end = R.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 7))
        self.assertEqual( R.check_temporal_topology(),  True)
        self.assertEqual(R.get_granularity(),  u'1 day')

    def test_temporal_conditional_complement_right_side_timestamps(self):
        """Test the conditional expression that evaluate if then else statements
        so that the else statement is a complement operation

        R = if({equal|contains|during}, B {#,contains,r} A == 2, B {+,contains,r} A, A)

        IF B contains two maps from A
        THEN add maps from A and B that fulfill the B contains A condition using the right side timestamps
        ELSE copy the maps from A

        The connection between the if then else statements are the
        topological relations equal, contains and during

        r0 = a1            # 1
        r1 = b1 + a2 + a3  # 12
        r2 = b1 + a2 + a3  # 12
        r3 = b2 + a4 + a5  # 16
        r4 = b2 + a4 + a5  # 16
        r5 = a6            # 6

        """
        tra = tgis.TemporalRasterAlgebraParser(run = True, debug = True)
        tra.parse(expression='R = if({equal|contains|during}, B {#,contains,r} A == 2, B {+,contains,r} A, A)',
                  basename="r", overwrite=True)

        R = tgis.open_old_stds("R", type="strds")
        R.select()
        self.assertEqual(R.metadata.get_number_of_maps(), 6)
        self.assertEqual(R.metadata.get_min_min(), 1)
        self.assertEqual(R.metadata.get_max_max(), 16)
        start, end = R.get_absolute_time()
        self.assertEqual(start, datetime.datetime(2001, 1, 1))
        self.assertEqual(end, datetime.datetime(2001, 1, 7))
        self.assertEqual( R.check_temporal_topology(),  True)
        self.assertEqual(R.get_granularity(),  u'1 day')


if __name__ == '__main__':
    test()
