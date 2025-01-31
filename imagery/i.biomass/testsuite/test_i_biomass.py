import numpy as np
from grass.script import array
from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class TestIBiomass(TestCase):
    """Regression tests for the i.biomass GRASS GIS module."""

    input_rasters = {
        "fpar": "test_fpar",
        "lightuse_eff": "test_lightuse_eff",
        "latitude": "test_latitude",
        "dayofyear": "test_dayofyear",
        "transmissivity": "test_transmissivity",
        "water": "test_water_availability",
    }
    output_raster = "biomass_output"

    @classmethod
    def setUpClass(cls):
        """Set up input rasters and configure test environment."""
        cls.use_temp_region()
        cls.runModule("g.region", n=10, s=0, e=10, w=0, rows=10, cols=10)
        cls._create_input_rasters()

    @classmethod
    def _create_input_rasters(cls):
        """Helper method to create all input rasters with defined expressions."""
        expressions = {
            "fpar": "col() * 0.1",
            "lightuse_eff": "row() * 0.1",
            "latitude": "45.0",
            "dayofyear": "150",
            "transmissivity": "0.75",
            "water": "0.8",
        }
        for raster_key, expr in expressions.items():
            raster_name = cls.input_rasters[raster_key]
            cls.runModule(
                "r.mapcalc", expression=f"{raster_name} = {expr}", overwrite=True
            )

    @classmethod
    def tearDownClass(cls):
        """Clean up generated data and reset the region."""
        rasters_to_remove = list(cls.input_rasters.values()) + [cls.output_raster]
        cls.runModule(
            "g.remove",
            type="raster",
            name=",".join(rasters_to_remove),
            flags="f",
            quiet=True,
        )
        cls.del_temp_region()

    def test_biomass_regression(self):
        """Regression test to ensure i.biomass output remains consistent."""
        reference_raster = "biomass_reference"

        self.assertModule(
            "i.biomass",
            fpar=self.input_rasters["fpar"],
            lightuse_efficiency=self.input_rasters["lightuse_eff"],
            latitude=self.input_rasters["latitude"],
            dayofyear=self.input_rasters["dayofyear"],
            transmissivity_singleway=self.input_rasters["transmissivity"],
            water_availability=self.input_rasters["water"],
            output=self.output_raster,
            overwrite=True,
        )
        self.assertRasterExists(self.output_raster)
        self.assertRasterExists(reference_raster)

        output_values = array.array(self.output_raster)
        reference_values = array.array(reference_raster)

        self.assertTrue(
            np.allclose(output_values, reference_values, atol=2.0),
            "Biomass raster values should match the reference values",
        )

    def test_biomass_linearity(self):
        """Test linearity of i.biomass by scaling inputs."""
        self.runModule(
            "r.mapcalc",
            expression=f"{self.input_rasters['fpar']} = col() * 0.5",
            overwrite=True,
        )
        self.assertModule(
            "i.biomass",
            fpar=self.input_rasters["fpar"],
            lightuse_efficiency=self.input_rasters["lightuse_eff"],
            latitude=self.input_rasters["latitude"],
            dayofyear=self.input_rasters["dayofyear"],
            transmissivity_singleway=self.input_rasters["transmissivity"],
            water_availability=self.input_rasters["water"],
            output=self.output_raster,
            overwrite=True,
        )
        self.assertRasterExists(self.output_raster)

        scaled_output_values = array.array(self.output_raster)

        self.runModule(
            "r.mapcalc",
            expression=f"{self.input_rasters['fpar']} = col() * 1.0",
            overwrite=True,
        )
        self.assertModule(
            "i.biomass",
            fpar=self.input_rasters["fpar"],
            lightuse_efficiency=self.input_rasters["lightuse_eff"],
            latitude=self.input_rasters["latitude"],
            dayofyear=self.input_rasters["dayofyear"],
            transmissivity_singleway=self.input_rasters["transmissivity"],
            water_availability=self.input_rasters["water"],
            output=self.output_raster,
            overwrite=True,
        )
        self.assertRasterExists(self.output_raster)

        original_output_values = array.array(self.output_raster)
        self.assertTrue(
            np.allclose(scaled_output_values * 2, original_output_values, atol=1e-5),
            "Linearity property failed for biomass computation",
        )

    def test_biomass_with_extreme_values(self):
        """Test the behavior of i.biomass with extreme input values."""
        extreme_values = {
            "fpar": [0, 1],
            "lightuse_eff": [0, 10],
            "latitude": [-90, 90],
            "dayofyear": [1, 365],
            "transmissivity": [0, 1],
            "water": [0, 1],
        }

        for key, (min_val, max_val) in extreme_values.items():
            self.runModule(
                "r.mapcalc",
                expression=f"{self.input_rasters[key]} = {min_val}",
                overwrite=True,
            )

        self.assertModule(
            "i.biomass",
            fpar=self.input_rasters["fpar"],
            lightuse_efficiency=self.input_rasters["lightuse_eff"],
            latitude=self.input_rasters["latitude"],
            dayofyear=self.input_rasters["dayofyear"],
            transmissivity_singleway=self.input_rasters["transmissivity"],
            water_availability=self.input_rasters["water"],
            output=self.output_raster,
            overwrite=True,
        )
        self.assertRasterExists(self.output_raster)

        output_values_min = array.array(self.output_raster)
        self.assertTrue(
            np.all(output_values_min >= 0),
            "Biomass output should not contain negative values.",
        )

        for key, (min_val, max_val) in extreme_values.items():
            self.runModule(
                "r.mapcalc",
                expression=f"{self.input_rasters[key]} = {max_val}",
                overwrite=True,
            )

        self.assertModule(
            "i.biomass",
            fpar=self.input_rasters["fpar"],
            lightuse_efficiency=self.input_rasters["lightuse_eff"],
            latitude=self.input_rasters["latitude"],
            dayofyear=self.input_rasters["dayofyear"],
            transmissivity_singleway=self.input_rasters["transmissivity"],
            water_availability=self.input_rasters["water"],
            output=self.output_raster,
            overwrite=True,
        )
        self.assertRasterExists(self.output_raster)

        output_values_max = array.array(self.output_raster)
        self.assertTrue(
            np.all(np.isfinite(output_values_max)),
            "Biomass output contains non-finite values.",
        )

    def test_biomass_latitude_dependency(self):
        """Test that biomass values vary reasonably with latitude."""

        self.runModule(
            "r.mapcalc",
            expression=f"{self.input_rasters['latitude']} = 0",
            overwrite=True,
        )
        self.assertModule(
            "i.biomass",
            fpar=self.input_rasters["fpar"],
            lightuse_efficiency=self.input_rasters["lightuse_eff"],
            latitude=self.input_rasters["latitude"],
            dayofyear=self.input_rasters["dayofyear"],
            transmissivity_singleway=self.input_rasters["transmissivity"],
            water_availability=self.input_rasters["water"],
            output=self.output_raster,
            overwrite=True,
        )
        equatorial_output = array.array(self.output_raster)

        self.runModule(
            "r.mapcalc",
            expression=f"{self.input_rasters['latitude']} = 90",
            overwrite=True,
        )
        self.assertModule(
            "i.biomass",
            fpar=self.input_rasters["fpar"],
            lightuse_efficiency=self.input_rasters["lightuse_eff"],
            latitude=self.input_rasters["latitude"],
            dayofyear=self.input_rasters["dayofyear"],
            transmissivity_singleway=self.input_rasters["transmissivity"],
            water_availability=self.input_rasters["water"],
            output=self.output_raster,
            overwrite=True,
        )
        polar_output = array.array(self.output_raster)

        self.assertTrue(
            np.mean(equatorial_output) > np.mean(polar_output),
            "Biomass at equatorial regions should generally exceed"
            "biomass at polar regions.",
        )

    def test_biomass_ecological_range(self):
        """Test that biomass values fall within an expected ecological range."""

        self.runModule(
            "r.mapcalc",
            expression=f"{self.input_rasters['fpar']} = col() * 0.5",
            overwrite=True,
        )
        self.runModule(
            "r.mapcalc",
            expression=f"{self.input_rasters['lightuse_eff']} = row() * 0.1",
            overwrite=True,
        )
        self.runModule(
            "r.mapcalc",
            expression=f"{self.input_rasters['latitude']} = 45.0",
            overwrite=True,
        )
        self.runModule(
            "r.mapcalc",
            expression=f"{self.input_rasters['dayofyear']} = 150",
            overwrite=True,
        )
        self.runModule(
            "r.mapcalc",
            expression=f"{self.input_rasters['transmissivity']} = 0.75",
            overwrite=True,
        )
        self.runModule(
            "r.mapcalc",
            expression=f"{self.input_rasters['water']} = 0.8",
            overwrite=True,
        )

        self.assertModule(
            "i.biomass",
            fpar=self.input_rasters["fpar"],
            lightuse_efficiency=self.input_rasters["lightuse_eff"],
            latitude=self.input_rasters["latitude"],
            dayofyear=self.input_rasters["dayofyear"],
            transmissivity_singleway=self.input_rasters["transmissivity"],
            water_availability=self.input_rasters["water"],
            output=self.output_raster,
            overwrite=True,
        )
        self.assertRasterExists(self.output_raster)

        output_values = array.array(self.output_raster)
        self.assertTrue(
            np.all((output_values >= 0) & (output_values <= 1000)),
            "Biomass output values should be within the range [0, 1000].",
        )


if __name__ == "__main__":
    test()
