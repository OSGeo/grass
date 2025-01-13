import numpy as np
from grass.script import raster_info, core
from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class TestIFFT(TestCase):
    """Regression tests for the i.fft GRASS GIS module."""

    input_raster = "test_input"
    real_output = "fft_real"
    imag_output = "fft_imag"

    def setUp(self):
        """Set up an input raster and configure test environment."""
        self.use_temp_region()
        self.runModule("g.region", n=10, s=0, e=10, w=0, rows=10, cols=10)
        self.runModule(
            "r.mapcalc", expression=f"{self.input_raster} = col()", overwrite=True
        )

    def tearDown(self):
        """Clean up generated data and reset the region."""
        rasters_to_remove = [
            self.input_raster,
            self.real_output,
            self.imag_output,
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
        self.runModule(
            "g.remove",
            type="raster",
            name=",".join(rasters_to_remove),
            flags="f",
            quiet=True,
        )
        self.del_temp_region()

    def test_fft_output_exists(self):
        """Check that i.fft produces real and imaginary output rasters."""
        self.assertModule(
            "i.fft",
            input=self.input_raster,
            real=self.real_output,
            imaginary=self.imag_output,
            overwrite=True,
        )
        self.assertRasterExists(self.real_output)
        self.assertRasterExists(self.imag_output)

    def test_fft_output_dimensions(self):
        """Verify that FFT output dimensions match the input raster."""
        self.assertModule(
            "i.fft",
            input=self.input_raster,
            real=self.real_output,
            imaginary=self.imag_output,
            overwrite=True,
        )
        input_info = raster_info(self.input_raster)
        real_info = raster_info(self.real_output)
        imag_info = raster_info(self.imag_output)

        self.assertEqual(
            input_info["rows"],
            real_info["rows"],
            "Mismatch in row count for real raster",
        )
        self.assertEqual(
            input_info["cols"],
            real_info["cols"],
            "Mismatch in column count for real raster",
        )
        self.assertEqual(
            input_info["rows"],
            imag_info["rows"],
            "Mismatch in row count for imaginary raster",
        )
        self.assertEqual(
            input_info["cols"],
            imag_info["cols"],
            "Mismatch in column count for imaginary raster",
        )

    def _parse_raster_values(self, raster):
        """Helper function to parse raster values into a numpy array."""
        output = core.read_command(
            "r.stats", input=raster, flags="n", separator="space"
        ).splitlines()
        return np.array(
            [float(line.split()[1]) for line in output if len(line.split()) == 2]
        )

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
        self.assertModule(
            "i.fft", input="input_2", real="real_2", imaginary="imag_2", overwrite=True
        )
        self.assertModule(
            "i.fft",
            input="combined",
            real="combined_real",
            imaginary="combined_imag",
            overwrite=True,
        )

        real_combined = self._parse_raster_values("combined_real")
        imag_combined = self._parse_raster_values("combined_imag")
        real_sum = self._parse_raster_values("real_1") + self._parse_raster_values(
            "real_2"
        )
        imag_sum = self._parse_raster_values("imag_1") + self._parse_raster_values(
            "imag_2"
        )

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
        spatial_energy = sum(v**2 for v in self._parse_raster_values(self.input_raster))
        freq_energy = sum(
            rv**2 + iv**2
            for rv, iv in zip(
                self._parse_raster_values(self.real_output),
                self._parse_raster_values(self.imag_output),
            )
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
        self.assertModule(
            "i.ifft",
            real=self.real_output,
            imaginary=self.imag_output,
            output="reconstructed",
            overwrite=True,
        )

        original_values = self._parse_raster_values(self.input_raster)
        reconstructed_values = self._parse_raster_values("reconstructed")

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
        real_values = self._parse_raster_values(self.real_output)
        imag_values = self._parse_raster_values(self.imag_output)
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
        real_values = self._parse_raster_values(self.real_output)
        imag_values = self._parse_raster_values(self.imag_output)
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
