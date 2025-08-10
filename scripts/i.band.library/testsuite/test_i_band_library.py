from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import call_module


class TestBandsSystemDefined(TestCase):
    @staticmethod
    def _number_of_bands(**kwargs):
        gbands = call_module("i.band.library", **kwargs)
        return len(gbands.splitlines())

    def test_number_system_defined(self):
        """Test number of valid band identifiers"""
        # get number of valid band identifiers by i.band.library
        nbands = self._number_of_bands()

        # get number of valid band identifiers by Bands lib
        from grass.semantic_label import SemanticLabelReader

        nbands_ref = len(SemanticLabelReader().get_bands())
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
