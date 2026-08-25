import os
from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from pathlib import Path


class TestLandsatTOARBasic(TestCase):
    """Regression tests for i.landsat.toar"""

    input_prefix = "lsat_"
    output_prefix = "toar_"
    metfile = "LC08_MTL.txt"
    bands = ["1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11"]

    @classmethod
    def setUpClass(cls):
        """Create test data."""
        cls.use_temp_region()
        cls.runModule("g.region", n=50, s=0, e=50, w=0, rows=50, cols=50)
        for band in cls.bands:
            cls.runModule(
                "r.mapcalc",
                expression=f"{cls.input_prefix}{band} = 1000 + row() + col()",
                overwrite=True,
            )
        Path(cls.metfile).write_text("""GROUP = L1_METADATA_FILE
    GROUP = PRODUCT_METADATA
        SPACECRAFT_ID = "LANDSAT_8"
        SENSOR_ID = "OLI_TIRS"
        DATE_ACQUIRED = "2023-01-01"
        FILE_DATE = "2023-01-01"
        SCENE_CENTER_TIME = "10:30:00"
    END_GROUP = PRODUCT_METADATA
    GROUP = IMAGE_ATTRIBUTES
        SUN_ELEVATION = 45.0
        SUN_AZIMUTH = 135.0
    END_GROUP = IMAGE_ATTRIBUTES
    GROUP = PROJECTION_PARAMETERS
        EARTH_SUN_DISTANCE = 0.9833368
    END_GROUP = PROJECTION_PARAMETERS
    GROUP = RADIOMETRIC_RESCALING
        RADIANCE_MULT_BAND_1 = 0.1
        RADIANCE_ADD_BAND_1 = 0.0
        RADIANCE_MAXIMUM_BAND_1 = 6553.5
        RADIANCE_MINIMUM_BAND_1 = 0.1
        REFLECTANCE_MULT_BAND_1 = 2.0e-4
        REFLECTANCE_ADD_BAND_1 = 0.0
        REFLECTANCE_MAXIMUM_BAND_1 = 1.0
        QUANTIZE_CAL_MAX_BAND_1 = 65535
        QUANTIZE_CAL_MIN_BAND_1 = 1

        RADIANCE_MULT_BAND_10 = 0.15
        RADIANCE_ADD_BAND_10 = 0.3
        RADIANCE_MAXIMUM_BAND_10 = 6553.5
        RADIANCE_MINIMUM_BAND_10 = 0.15
        QUANTIZE_CAL_MAX_BAND_10 = 65535
        QUANTIZE_CAL_MIN_BAND_10 = 1
    END_GROUP = RADIOMETRIC_RESCALING
    GROUP = THERMAL_CONSTANTS
        K1_CONSTANT_BAND_10 = 774.89
        K2_CONSTANT_BAND_10 = 1321.08
    END_GROUP = THERMAL_CONSTANTS
    END_GROUP = L1_METADATA_FILE
END""")

    @classmethod
    def tearDownClass(cls):
        """Remove generated data"""
        cls.del_temp_region()
        input_rasters = [f"{cls.input_prefix}{b}" for b in cls.bands]
        output_rasters = [f"{cls.output_prefix}{b}" for b in cls.bands]
        extra_outputs = {
            "scaled_": ["1"],
            "rad_": ["1"],
            "manual_": ["1"],
            "dos_": ["1"],
            "gain_": ["1", "2", "3", "4", "5", "61", "62", "7", "8"],
            "dos3_": ["1"],
        }

        test_rasters = []
        for suffix, bands in extra_outputs.items():
            test_rasters += [f"{cls.output_prefix}{suffix}{b}" for b in bands]
        all_rasters = input_rasters + output_rasters + test_rasters
        cls.runModule("g.remove", flags="f", type="raster", name=all_rasters)
        if Path(cls.metfile).exists():
            os.remove(cls.metfile)

    def test_reflectance_and_temperature_conversion_with_scale(self):
        """Test conversion of DN to top-of-atmosphere reflectance and brightness temperature with scale factor"""
        self.assertModule(
            "i.landsat.toar",
            input=self.input_prefix,
            output=self.output_prefix,
            metfile=self.metfile,
            overwrite=True,
        )
        self.assertRasterExists(f"{self.output_prefix}1")
        self.assertRasterExists(f"{self.output_prefix}10")
        self.assertRasterFitsUnivar(
            f"{self.output_prefix}1",
            reference={
                "min": 0.0216226,
                "max": 0.0237374,
                "mean": 0.02268,
                "stddev": 0.0004404,
            },
            precision=1e-6,
        )
        self.assertRasterFitsUnivar(
            f"{self.output_prefix}10",
            reference={
                "min": 609.713222,
                "max": 633.735610,
                "mean": 621.791689,
                "stddev": 5.0021045,
            },
            precision=1e-6,
        )
        scaled_output_prefix = f"{self.output_prefix}scaled_"
        self.assertModule(
            "i.landsat.toar",
            input=self.input_prefix,
            output=scaled_output_prefix,
            metfile=self.metfile,
            scale=0.1,
            overwrite=True,
        )
        self.assertRasterFitsUnivar(
            f"{scaled_output_prefix}1",
            reference={
                "min": 0.0216226 * 0.1,
                "max": 0.0237374 * 0.1,
                "mean": 0.02268 * 0.1,
                "stddev": 0.0004404 * 0.1,
            },
            precision=1e-6,
        )

    def test_radiance_conversion_flag(self):
        """Test the radiance output flag (-r) for converting DN to radiance values"""
        self.assertModule(
            "i.landsat.toar",
            input=self.input_prefix,
            output=f"{self.output_prefix}rad_",
            metfile=self.metfile,
            flags="r",
            overwrite=True,
        )
        self.assertRasterFitsUnivar(
            f"{self.output_prefix}rad_1",
            reference={
                "min": 100.2,
                "max": 110.0,
                "mean": 105.1,
                "stddev": 2.040833,
            },
            precision=1e-6,
        )

    def test_sensor_specific_conversion_without_metfile(self):
        """Test TOAR conversion using explicit sensor parameters when metadata file is not available"""
        self.assertModule(
            "i.landsat.toar",
            input=self.input_prefix,
            output=f"{self.output_prefix}tm5_",
            sensor="tm5",
            date="2000-06-15",
            sun_elevation=45.0,
            product_date="2000-06-20",
            overwrite=True,
        )
        self.assertRasterFitsUnivar(
            f"{self.output_prefix}tm5_6",
            reference={
                "min": 511.520389,
                "max": 529.405584,
                "mean": 520.519746,
            },
            precision=1e-6,
        )

    def test_dos1_atmospheric_correction(self):
        """Test Dark Object Subtraction 1 (DOS1) atmospheric correction method"""
        self.assertModule(
            "i.landsat.toar",
            input=self.input_prefix,
            output=f"{self.output_prefix}dos_",
            metfile=self.metfile,
            method="dos1",
            percent=0.5,
            pixel=500,
            overwrite=True,
        )
        self.assertRasterFitsUnivar(
            f"{self.output_prefix}dos_1",
            reference={
                "min": 0.521601,
                "max": 0.523715,
                "mean": 0.522658,
                "stddev": 0.0004404,
            },
            precision=1e-6,
        )

    def test_etm_gain_settings_conversion(self):
        """Test conversion with different gain settings (High/Low) for Landsat ETM+ bands"""
        self.runModule(
            "r.mapcalc",
            expression=f"{self.input_prefix}61 = 1000 + row() + col()",
            overwrite=True,
        )
        self.addCleanup(
            self.runModule,
            "g.remove",
            flags="f",
            type="raster",
            name=f"{self.input_prefix}61",
        )
        self.runModule(
            "r.mapcalc",
            expression=f"{self.input_prefix}62 = 1000 + row() + col()",
            overwrite=True,
        )
        self.addCleanup(
            self.runModule,
            "g.remove",
            flags="f",
            type="raster",
            name=f"{self.input_prefix}62",
        )
        gain_setting = "HHHHHLLHH"
        output_prefix = f"{self.output_prefix}gain_"
        self.assertModule(
            "i.landsat.toar",
            input=self.input_prefix,
            output=output_prefix,
            sensor="tm7",
            gain=gain_setting,
            date="2002-06-15",
            sun_elevation=45.0,
            product_date="2002-06-20",
            overwrite=True,
        )
        etm_bands = ["1", "2", "3", "4", "5", "61", "62", "7", "8"]
        for band in etm_bands:
            self.assertRasterExists(f"{output_prefix}{band}")
        self.assertRasterFitsUnivar(
            f"{output_prefix}1",
            reference={
                "min": 1.8002626,
                "max": 1.977925,
                "mean": 1.889093,
                "stddev": 0.0369979,
            },
            precision=1e-6,
        )
        self.assertRasterFitsUnivar(
            f"{output_prefix}61",
            reference={
                "min": 536.587816,
                "max": 556.244270,
                "mean": 546.4777,
                "stddev": 4.092958,
            },
            precision=1e-6,
        )

    def test_dos3_atmospheric_correction_with_rayleigh(self):
        """Test Dark Object Subtraction 3 (DOS3) atmospheric correction with Rayleigh scattering parameter"""
        self.assertModule(
            "i.landsat.toar",
            input=self.input_prefix,
            output=f"{self.output_prefix}dos3_",
            metfile=self.metfile,
            method="dos3",
            percent=0.5,
            pixel=500,
            rayleigh=0.5,
            overwrite=True,
        )
        self.assertRasterFitsUnivar(
            f"{self.output_prefix}dos3_1",
            reference={
                "min": 0.5382208,
                "max": 0.541962,
                "mean": 0.540091,
                "stddev": 0.000779,
            },
            precision=1e-6,
        )


if __name__ == "__main__":
    test()
