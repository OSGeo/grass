#!/usr/bin/env python3

from grass.gunittest.case import TestCase
from grass.gunittest.main import test


# Input data notes

# Created in:
# nc_spm/PERMANENT

# all_types... files
# File with 2 points, 1 line, and 3 areas created in wxGUI digitizer
# v.out.ogr input=all_types output=data/all_types.gpkg format=GPKG
# ogr2ogr data/all_types_wgs84.gpkg data/all_types.gpkg -f GPKG -t_srs EPSG:4326


class TestVImport(TestCase):

    imported = "test_v_import_imported"

    def tearDown(cls):
        """Remove imported map after each test method"""
        cls.runModule("g.remove", flags="f", type="vector", name=cls.imported)

    def test_import_same_proj_gpkg(self):
        """Import GPKG in same proj, default params"""
        self.assertModule("v.import", input="data/all_types.gpkg", output=self.imported)
        self.assertVectorExists(self.imported)
        self.assertVectorFitsExtendedInfo(
            vector=self.imported,
            reference=dict(
                name=self.imported,
                level=2,
                num_dblinks=1,
                attribute_table=self.imported,
            ),
        )
        self.assertVectorFitsRegionInfo(
            vector=self.imported,
            # Values rounded to one decimal point.
            reference=dict(
                north=227744.8,
                south=215259.6,
                east=644450.6,
                west=631257.4,
                top=0,
                bottom=0,
            ),
            precision=0.2,
        )
        self.assertVectorFitsTopoInfo(
            vector=self.imported,
            reference=dict(
                nodes=5,
                points=2,
                lines=1,
                boundaries=3,
                centroids=3,
                areas=3,
                islands=3,
                primitives=9,
                map3d=0,
            ),
        )

    def test_import_gpkg_wgs84(self):
        """Import GPKG in same proj, default params"""
        self.assertModule(
            "v.import", input="data/all_types_wgs84.gpkg", output=self.imported
        )
        self.assertVectorExists(self.imported)
        self.assertVectorFitsExtendedInfo(
            vector=self.imported,
            reference=dict(
                name=self.imported,
                level=2,
                num_dblinks=1,
                attribute_table=self.imported,
            ),
        )
        self.assertVectorFitsRegionInfo(
            vector=self.imported,
            # Values rounded to one decimal point.
            reference=dict(
                north=227744.8,
                south=215259.6,
                east=644450.6,
                west=631257.4,
                top=0,
                bottom=0,
            ),
            precision=0.2,
        )
        self.assertVectorFitsTopoInfo(
            vector=self.imported,
            reference=dict(
                nodes=5,
                points=2,
                lines=1,
                boundaries=3,
                centroids=3,
                areas=3,
                islands=3,
                primitives=9,
                map3d=0,
            ),
        )


if __name__ == "__main__":
    test()
