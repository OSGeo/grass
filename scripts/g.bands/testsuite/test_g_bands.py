import os

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import call_module


class TestBandsSystemDefined(TestCase):
    @staticmethod
    def _number_of_bands(**kwargs):
        gbands = call_module("g.bands", **kwargs)
        return len(gbands.rstrip(os.linesep).split(os.linesep))

    def test_number_system_defined(self):
        """Test number of valid band identifiers"""
        # get number of valid band identifiers by g.bands
        nbands = self._number_of_bands()

        # get number of valid band identifiers by Bands lib
        from grass.bandref import BandReferenceReader

        nbands_ref = len(BandReferenceReader().get_bands())
        self.assertEqual(nbands, nbands_ref)

    def test_number_s2(self):
        """Test number of S2 band identifiers (hardcoded, no changes expected)"""
        nbands = self._number_of_bands(pattern="S2")
        self.assertEqual(nbands, 13)

    def test_number_s2_1(self):
        """Test if S2_1 is defined (lower + upper case)"""
        band = "S2_1"
        for iband in [band, band.upper()]:
            nbands = self._number_of_bands(pattern=iband)
            self.assertEqual(nbands, 1)


if __name__ == "__main__":
    test()
