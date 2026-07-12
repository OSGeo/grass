import numpy as np
from grass.script import array
from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class TestIFFT(TestCase):
    """Regression tests for the i.fft GRASS module."""

    input_raster = "test_input"
    real_output = "fft_real"
    imag_output = "fft_imag"

    @classmethod
    def setUpClass(cls):
        """Set up an input raster and configure test environment."""
        cls.use_temp_region()
        cls.runModule("g.region", n=10, s=0, e=10, w=0, rows=10, cols=10)
        cls.runModule(
            "r.mapcalc", expression=f"{cls.input_raster} = col()", overwrite=True
        )

    @classmethod
    def tearDownClass(cls):
        """Clean up generated data and reset the region."""
        rasters_to_remove = [
            cls.input_raster,
            cls.real_output,
            cls.imag_output,
            "reconstructed",
            "input_2",
            "combined",
            "real_1",
            "imag_1",
            "real_2",
            "imag_2",
            "combined_real",
            "combined_imag",
        ]
        cls.runModule(
            "g.remove",
            type="raster",
            name=",".join(rasters_to_remove),
            flags="f",
            quiet=True,
        )
        cls.del_temp_region()

    def test_linearity_property(self):
        """Test linearity of the FFT by combining two rasters."""
        self.runModule("r.mapcalc", expression="input_2 = row()", overwrite=True)
        self.runModule(
            "r.mapcalc", expression="combined = test_input + input_2", overwrite=True
        )

        self.assertModule(
            "i.fft",
            input=self.input_raster,
            real="real_1",
            imaginary="imag_1",
            overwrite=True,
        )
        self.assertRasterExists("real_1")
        self.assertRasterExists("imag_1")

        self.assertModule(
            "i.fft", input="input_2", real="real_2", imaginary="imag_2", overwrite=True
        )
        self.assertRasterExists("real_2")
        self.assertRasterExists("imag_2")

        self.assertModule(
            "i.fft",
            input="combined",
            real="combined_real",
            imaginary="combined_imag",
            overwrite=True,
        )

        self.assertRasterExists("combined_real")
        self.assertRasterExists("combined_imag")

        real_combined = array.array("combined_real")
        imag_combined = array.array("combined_imag")
        real_sum = array.array("real_1") + array.array("real_2")
        imag_sum = array.array("imag_1") + array.array("imag_2")

        self.assertTrue(
            np.allclose(real_combined, real_sum, atol=1e-5),
            "Linearity failed for real component",
        )
        self.assertTrue(
            np.allclose(imag_combined, imag_sum, atol=1e-5),
            "Linearity failed for imaginary component",
        )

    def test_energy_conservation_theorem(self):
        """Validate energy conservation theorem."""
        self.assertModule(
            "i.fft",
            input=self.input_raster,
            real=self.real_output,
            imaginary=self.imag_output,
            overwrite=True,
        )

        self.assertRasterExists(self.real_output)
        self.assertRasterExists(self.imag_output)

        spatial_energy = np.sum(array.array(self.input_raster) ** 2)
        freq_energy = np.sum(
            array.array(self.real_output) ** 2 + array.array(self.imag_output) ** 2
        )
        self.assertAlmostEqual(
            spatial_energy, freq_energy, places=5, msg="Energy conservation failed"
        )

    def test_inverse_fft(self):
        """Check that inverse FFT reconstructs the original raster."""
        self.assertModule(
            "i.fft",
            input=self.input_raster,
            real=self.real_output,
            imaginary=self.imag_output,
            overwrite=True,
        )

        self.assertRasterExists(self.real_output)
        self.assertRasterExists(self.imag_output)

        self.assertModule(
            "i.ifft",
            real=self.real_output,
            imaginary=self.imag_output,
            output="reconstructed",
            overwrite=True,
        )

        original_values = array.array(self.input_raster)
        reconstructed_values = array.array("reconstructed")

        self.assertTrue(
            np.allclose(original_values, reconstructed_values, atol=1e-5),
            "Reconstructed raster does not match the original",
        )

    def test_all_zero_raster(self):
        """Test FFT behavior with an all-zero raster."""
        self.runModule(
            "r.mapcalc", expression=f"{self.input_raster} = 0", overwrite=True
        )
        self.assertModule(
            "i.fft",
            input=self.input_raster,
            real=self.real_output,
            imaginary=self.imag_output,
            overwrite=True,
        )
        self.assertRasterExists(self.real_output)
        self.assertRasterExists(self.imag_output)

        real_values = array.array(self.real_output)
        imag_values = array.array(self.imag_output)
        self.assertTrue(
            np.allclose(real_values, 0), "Real component should be all zeros"
        )
        self.assertTrue(
            np.allclose(imag_values, 0), "Imaginary component should be all zeros"
        )

    def test_all_one_raster(self):
        """Test FFT behavior with an all-one raster."""
        self.runModule(
            "r.mapcalc", expression=f"{self.input_raster} = 1", overwrite=True
        )
        self.assertModule(
            "i.fft",
            input=self.input_raster,
            real=self.real_output,
            imaginary=self.imag_output,
            overwrite=True,
        )
        self.assertRasterExists(self.real_output)
        self.assertRasterExists(self.imag_output)

        real_values = array.array(self.real_output)
        imag_values = array.array(self.imag_output)
        self.assertTrue(
            np.all(np.isfinite(real_values)), "Real component should have valid values"
        )
        self.assertTrue(
            np.all(np.isfinite(imag_values)),
            "Imaginary component should have valid values",
        )

    def test_large_raster_performance(self):
        """Assess performance with a larger raster."""
        self.runModule("g.region", n=90, s=-90, e=180, w=-180, rows=1000, cols=1000)
        self.runModule(
            "r.mapcalc", expression=f"{self.input_raster} = col()", overwrite=True
        )

        self.assertModule(
            "i.fft",
            input=self.input_raster,
            real=self.real_output,
            imaginary=self.imag_output,
            overwrite=True,
        )

        self.assertRasterExists(self.real_output)
        self.assertRasterExists(self.imag_output)


if __name__ == "__main__":
    test()
