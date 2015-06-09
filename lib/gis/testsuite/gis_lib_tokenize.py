"""Test of gis library tokenizing of text

@author Vaclav Petras
"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test

import grass.lib.gis as libgis

# TODO: add tests for content (now testing only number of items)


class TokenizeTestCase(TestCase):
    """Test C function G_tokenize() from gis library"""

    def test_tokenize_comma(self):
        """Test G_tokenize with comma as delim"""
        tokens = libgis.G_tokenize("a,b,c,d", ",")
        num_of_tokens = libgis.G_number_of_tokens(tokens)
        self.assertEqual(num_of_tokens, 4, msg="Got wrong number of tokens")

    def test_tokenize_alternative_delim(self):
        """Test G_tokenize with semi colon as delim"""
        tokens = libgis.G_tokenize("a;b;c", ";")
        num_of_tokens = libgis.G_number_of_tokens(tokens)
        self.assertEqual(num_of_tokens, 3, msg="Got wrong number of tokens")

    def test_tokenize_with_text_delim(self):
        """Test G_tokenize with comma as delim and single quote text delim

        Expecting the 'wrong' number of tokens here.
        """
        tokens = libgis.G_tokenize("a,'b,c',d", ",")
        num_of_tokens = libgis.G_number_of_tokens(tokens)
        self.assertEqual(
            num_of_tokens, 4,
            msg="Got wrong number of tokens (expecting that the text"
                "delimiter is ignored)")
        # alternatively this can be done using test with expected failure


class Tokenize2TestCase(TestCase):
    """Test C function G_tokenize2() from gis library"""

    def test_tokenize2_comma(self):
        """Test G_tokenize2 without any text delim"""
        tokens = libgis.G_tokenize2("a,b,c,d", ",", "'")
        num_of_tokens = libgis.G_number_of_tokens(tokens)
        self.assertEqual(num_of_tokens, 4, msg="Got wrong number of tokens")

    def test_tokenize2_with_text_delim(self):
        """Test G_tokenize2 with , as delim and single quote text delim"""
        tokens = libgis.G_tokenize2("a,'b,c',d", ",", "'")
        num_of_tokens = libgis.G_number_of_tokens(tokens)
        self.assertEqual(num_of_tokens, 3, msg="Got wrong number of tokens")

    def test_tokenize2_with_alternative_text_delim(self):
        """Test G_tokenize2 with ; as delim and double quote text delim"""
        tokens = libgis.G_tokenize2('a;"b;c";d', ';', '"')
        num_of_tokens = libgis.G_number_of_tokens(tokens)
        self.assertEqual(num_of_tokens, 3, msg="Got wrong number of tokens")

    def test_tokenize2_with_text_delim_more_text_tokens(self):
        """Test G_tokenize2 with comma as delim and hash as text delim"""
        tokens = libgis.G_tokenize2("a,#b,c#,#5,d#,#7,2#", ",", "#")
        num_of_tokens = libgis.G_number_of_tokens(tokens)
        self.assertEqual(num_of_tokens, 4, msg="Got wrong number of tokens")

    def test_tokenize2_with_real_text(self):
        """Test G_tokenize2 with real world text"""
        tokens = libgis.G_tokenize2(
            '440,617722.81,3464034.494,951.987,'
            '"Low Erosion (1,5)","High Deposition (8,6)"',
            ',', '"')
        num_of_tokens = libgis.G_number_of_tokens(tokens)
        self.assertEqual(num_of_tokens, 6, msg="Got wrong number of tokens")


if __name__ == '__main__':
    test()
