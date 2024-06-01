import os
import grass.script as gs
from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule
import numpy as np
import matplotlib.pyplot as plt

class TestPansharpen(TestCase):
    @classmethod
    def setUpClass(cls):
        # Ensure that the region settings are appropriate for the pansharpening process
        gs.run_command('g.region', raster='lsat7_2002_10@PERMANENT', flags='p')

    def setUp(self):
        # Set up temporary names for output
        self.output_brovey = 'temp_brovey'
        self.output_ihs = 'temp_ihs'
        self.output_pca = 'temp_pca'

    def tearDown(self):
        # Clean up after each test
        gs.run_command('g.remove', type='raster', name=[self.output_brovey, self.output_ihs, self.output_pca], flags='f')

    def test_brovey(self):
        """Test Brovey pansharpening method, checking statistical properties."""
        self.run_pansharpen('brovey', self.output_brovey)
        self.raster_statistics(self.output_brovey)

    def test_ihs(self):
        """Test IHS pansharpening method, comparing output to known good result."""
        self.run_pansharpen('ihs', self.output_ihs)
        self.raster_statistics(self.output_ihs)
        self.compare_to_known_good(self.output_ihs, 'known_good_ihs')

    def test_pca(self):
        """Test PCA pansharpening method and visualize output."""
        self.run_pansharpen('pca', self.output_pca)
        self.raster_statistics(self.output_pca)
        self.visual_check(self.output_pca)

    def run_pansharpen(self, method, output):
        """Generalized function to run pansharpening."""
        module = SimpleModule('i.pansharpen', red='lsat7_2002_30@PERMANENT', green='lsat7_2002_40@PERMANENT', blue='lsat7_2002_10@PERMANENT', pan='lsat7_2002_10@PERMANENT', output=output, method=method)
        self.assertModule(module)

    def raster_statistics(self, raster):
        """Check basic statistics of the output raster."""
        univar = gs.parse_command('r.univar', map=raster, flags='g')
        mean = float(univar['mean'])
        stddev = float(univar['stddev'])
        self.assertGreater(mean, 10, msg="Mean is too low, expected more intensity in the image")
        self.assertLess(stddev, 50, msg="Standard deviation is too high, expected less variability")

    def compare_to_known_good(self, raster, known_good):
        """Compare to a known good result stored in the system."""
        diff = gs.raster_difference(raster, known_good, precision=0.01)
        self.assertEqual(diff, 0, msg=f"Output raster {raster} differs from known good {known_good}")

    def visual_check(self, raster):
        """Generate a plot for visual inspection (not automatic)."""
        array = gs.raster_to_numpy_array(raster)
        plt.imshow(array, cmap='gray')
        plt.title(f'Output of {raster}')
        plt.colorbar()
        plt.show()

if __name__ == '__main__':
    test()
