from grass.gunittest import TestCase, test
from grass.gunittest.gmodules import SimpleModule
import grass.script.core as gcore

# not used yet
LOCATION = 'nc_spm'

output1 = """
 0.000000 88.370453
 10.000000 88.397057
 20.000000 89.526253
 30.000000 89.677551
 40.000000 91.297195
 50.000000 91.297195
 60.000000 92.330658
 70.000000 93.069199
 80.000000 94.768280
 90.000000 95.524551
 100.000000 96.770805
 110.000000 96.770805
 120.000000 97.418869
"""

output2 = """
637656.000000 224222.000000 0.000000 88.370453
637664.540486 224227.201932 10.000000 88.397057
637673.080972 224232.403865 20.000000 89.526253
637681.621458 224237.605797 30.000000 89.677551
637690.161944 224242.807729 40.000000 91.297195
637698.702430 224248.009662 50.000000 91.297195
637707.242916 224253.211594 60.000000 92.330658
637715.783402 224258.413526 70.000000 93.069199
637724.323887 224263.615459 80.000000 94.768280
637732.864373 224268.817391 90.000000 95.524551
637741.404859 224274.019323 100.000000 96.770805
637749.945345 224279.221256 110.000000 96.770805
637758.485831 224284.423188 120.000000 97.418869
"""

output3 = """
 0.000000 91.071831
 10.000000 91.431198
 20.000000 91.746628
 30.000000 91.746628
 40.000000 91.748047
 50.000000 91.872192
 60.000000 91.730049
 70.000000 91.690292
 80.000000 91.341331
 86.533231 91.341331
 96.533231 91.639000
 106.533231 nodata
 116.533231 nodata
 126.533231 nodata
 136.533231 nodata
 146.533231 nodata
 156.533231 nodata
 166.533231 nodata
 176.533231 nodata
 186.533231 nodata
 196.533231 nodata
 206.533231 nodata
 216.533231 nodata
"""

output4 = """
 0.000000 88.370453
 25.000000 89.526253
 50.000000 91.297195
 75.000000 94.768280
 100.000000 96.770805
 125.000000 97.646629
"""


class TestProfileNCSPM(TestCase):

    @classmethod
    def setUpClass(cls):
        gcore.use_temp_region()
        gcore.run_command('g.region', rast='elevation')

    @classmethod
    def tearDownClass(cls):
        gcore.del_temp_region()

    def test_profile_default(self):
        rprofile = SimpleModule('r.profile', input='elevation',
                                coordinates=[637656, 224222, 637766, 224289])
        self.assertModule(rprofile)
        self.assertMultiLineEqual(rprofile.outputs.stdout.strip(), output1.strip())
        self.assertIn('128.798294 [meters]', rprofile.outputs.stderr)  # distance
        self.assertIn('10 [meters]', rprofile.outputs.stderr)  # resolution

    def test_profile_m(self):
        rprofile = SimpleModule('r.profile', input='elevation', flags='m',
                                coordinates=[637656, 224222, 637766, 224289])
        self.assertModule(rprofile)
        self.assertIn('128.798294 [meters]', rprofile.outputs.stderr)  # distance
        self.assertIn('10 [meters]', rprofile.outputs.stderr)  # resolution

    def test_profile_resolution(self):
        rprofile = SimpleModule('r.profile', input='elevation', resolution=25,
                                coordinates=[637656, 224222, 637766, 224289])
        self.assertModule(rprofile)
        self.assertMultiLineEqual(rprofile.outputs.stdout.strip(), output4.strip())
        self.assertIn('128.798294 [meters]', rprofile.outputs.stderr)  # distance
        self.assertIn('25 [meters]', rprofile.outputs.stderr)  # resolution

    def test_profile_ne(self):
        rprofile = SimpleModule('r.profile', input='elevation', flags='g',
                                coordinates=[637656, 224222, 637766, 224289])
        self.assertModule(rprofile)
        self.assertMultiLineEqual(rprofile.outputs.stdout.strip(), output2.strip())

    def test_profile_region(self):
        rprofile = SimpleModule('r.profile', input='elevation', null='nodata',
                                coordinates=[644914, 224579, 644986,
                                             224627, 645091, 224549])
        self.assertModule(rprofile)
        self.assertMultiLineEqual(rprofile.outputs.stdout.strip(), output3.strip())
        self.assertIn("WARNING: Endpoint coordinates are outside of current region settings",
                      rprofile.outputs.stderr,)


if __name__ == '__main__':
    test()
