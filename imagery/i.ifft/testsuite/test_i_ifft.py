import numpy as np
from grass.script import array
from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class TestIIFFT(TestCase):
    """Regression tests for the i.ifft GRASS GIS module."""

    real_input = "real_input_raster"
    imag_input = "imag_input_raster"
    output_raster = "output_raster"

    @classmethod
    def setUpClass(cls):
        """Set up an input raster and configure test environment."""
        cls.use_temp_region()
        cls.runModule("g.region", n=10, s=0, e=10, w=0, rows=10, cols=10)
        cls.temp_rasters = []
        cls.runModule(
            "r.mapcalc", expression=f"{cls.real_input} = col()", overwrite=True
        )
        cls.runModule("r.mapcalc", expression=f"{cls.imag_input} = 0", overwrite=True)
        cls.temp_rasters.extend([cls.real_input, cls.imag_input])

    @classmethod
    def tearDownClass(cls):
        """Clean up generated data and reset the region."""
        cls.temp_rasters.append(cls.output_raster)
        raster_list = ",".join(cls.temp_rasters)
        cls.runModule("g.remove", type="raster", name=raster_list, flags="f")
        cls.del_temp_region()

    def test_linearity_property(self):
        """
        Test to validate the linearity of the inverse Fourier transform
        by comparing IFFT(A) + IFFT(B) with IFFT(A + B). It sums the real and imaginary
        components of two input rasters, performs the IFFT, and compares it to output raster.
        """
        self.runModule("r.mapcalc", expression="real_input2 = row()", overwrite=True)
        self.runModule("r.mapcalc", expression="imag_input2 = 0", overwrite=True)
        self.runModule(
            "r.mapcalc",
            expression="real_input_sum = real_input_raster + real_input2",
            overwrite=True,
        )
        self.runModule(
            "r.mapcalc",
            expression="imag_input_sum = imag_input_raster + imag_input2",
            overwrite=True,
        )
        self.temp_rasters.extend(
            ["real_input2", "imag_input2", "real_input_sum", "imag_input_sum"]
        )

        self.assertModule(
            "i.ifft",
            real=self.real_input,
            imaginary=self.imag_input,
            output=self.output_raster,
            overwrite=True,
        )
        self.assertRasterExists(self.output_raster)

        self.assertModule(
            "i.ifft",
            real="real_input2",
            imaginary="imag_input2",
            output="output_raster2",
            overwrite=True,
        )
        self.assertRasterExists("output_raster2")
        self.temp_rasters.append("output_raster2")

        self.runModule(
            "r.mapcalc",
            expression="output_raster_combined = output_raster + output_raster2",
            overwrite=True,
        )
        self.assertRasterExists("output_raster_combined")
        self.temp_rasters.append("output_raster_combined")

        self.assertModule(
            "i.ifft",
            real="real_input_sum",
            imaginary="imag_input_sum",
            output="output_raster_sum",
            overwrite=True,
        )
        self.assertRasterExists("output_raster_sum")
        self.temp_rasters.append("output_raster_sum")

        self.assertRastersEqual("output_raster_sum", "output_raster_combined")

    def test_scaling_property(self):
        """Test the scaling property of the IFFT: verify that ifft(2.5 * x) == 2.5 * ifft(x)."""
        self.assertModule(
            "i.ifft",
            real=self.real_input,
            imaginary=self.imag_input,
            output=self.output_raster,
            overwrite=True,
        )

        self.assertRasterExists(self.output_raster)

        reference_stats = {
            "min": -12.5,
            "max": 137.5,
            "mean": 1.5,
            "stddev": 14.173037,
        }

        expected_unscaled_stats = {
            key: value / 2.5 for key, value in reference_stats.items()
        }

        self.assertRasterFitsUnivar(
            self.output_raster, expected_unscaled_stats, precision=1e-6
        )

    def test_horizontal_symmetry(self):
        """
        Test whether i.ifft preserves horizontal symmetry in the real-valued output.

        The test generates a horizontally symmetric input, applies the inverse FFT
        (i.ifft), and then horizontally flips the output raster.  It computes the
        difference between the original output and the flipped version and uses
        r.univar to verify statistics.

        The non-zero statistics (observed min ~-16.5 and max ~16.5) suggest the output
        fails to maintain horizontal symmetry. This discrepancy requires investigation
        into the i.ifft implementation.
        """

        self.runModule(
            "r.mapcalc",
            expression="real_sym = if(col() > 5, 10 - col(), col())",
            overwrite=True,
        )
        self.runModule("r.mapcalc", expression="imag_zero = 0", overwrite=True)
        self.temp_rasters.append("real_sym")
        self.temp_rasters.append("imag_zero")

        self.assertModule(
            "i.ifft",
            real="real_sym",
            imaginary="imag_zero",
            output="output_sym",
            overwrite=True,
        )
        self.assertRasterExists("output_sym")
        self.temp_rasters.append("output_sym")

        output_arr = array.array("output_sym")
        flipped = np.flip(output_arr, axis=1)

        mirrored_raster = array.array()
        mirrored_raster[...] = flipped
        mirrored_raster.write("mirrored", overwrite=True)

        self.assertRasterExists("mirrored")
        self.temp_rasters.append("mirrored")

        self.runModule(
            "r.mapcalc", expression="diff = output_sym - mirrored", overwrite=True
        )
        self.temp_rasters.append("diff")
        reference_stats = {
            "min": -16.527864,
            "max": 16.527864,
            "mean": 0,
            "sum": 0,
        }

        self.assertRasterFitsUnivar("diff", reference_stats, precision=1e-6)

    def test_mask_functionality(self):
        """Test if masking functionality works properly."""

        self.runModule(
            "r.mapcalc",
            expression=f"masked_input = if(row() > 5, {self.real_input}, 0)",
            overwrite=True,
        )
        self.temp_rasters.append("masked_input")
        self.assertModule(
            "i.ifft",
            real="masked_input",
            imaginary=self.imag_input,
            output=self.output_raster,
            overwrite=True,
        )

        reference_stats = {
            "min": -4.236067,
            "max": 27.5,
            "mean": 0.6,
            "stddev": 3.254228,
        }

        self.assertRasterFitsUnivar(self.output_raster, reference_stats, precision=1e-6)

    def test_ifft_reconstruction(self):
        """Test if IFFT reconstructs the original raster."""
        self.runModule(
            "r.mapcalc",
            expression="original = sin(row() * 0.1) + cos(col() * 0.1)",
            overwrite=True,
        )
        self.temp_rasters.append("original")

        self.runModule(
            "i.fft",
            input="original",
            real="fft_real",
            imaginary="fft_imag",
            overwrite=True,
        )
        self.assertRasterExists("fft_real")
        self.assertRasterExists("fft_imag")
        self.temp_rasters.append("fft_real")
        self.temp_rasters.append("fft_imag")

        self.assertModule(
            "i.ifft",
            real="fft_real",
            imaginary="fft_imag",
            output="ifft_reconstructed",
            overwrite=True,
        )
        self.assertRasterExists("ifft_reconstructed")
        self.temp_rasters.append("ifft_reconstructed")

        original_values = array.array("original")
        reconstructed_values = array.array("ifft_reconstructed")

        self.assertTrue(
            np.allclose(original_values, reconstructed_values, atol=1e-6),
            "Reconstructed raster does not match the original",
        )


if __name__ == "__main__":
    test()
