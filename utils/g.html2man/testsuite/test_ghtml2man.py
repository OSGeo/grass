"""Test g.html2man.py module

@author: Tomas Zigo
"""

import glob
import importlib
import os
import sys

from grass.gunittest.case import TestCase


class TestGhtml2man(TestCase):
    """Test the g.html2man.py module, especially the conversion of the
    manual HTML page to the CLI manual page format"""

    def setUp(self):
        self.gisbase = os.getenv("GISBASE")
        self.ghtml_module_dir = os.path.join(
            os.path.dirname(__file__),
            "..",
        )
        sys.path.append(self.ghtml_module_dir)
        self.ghtml = importlib.import_module("ghtml")

    def tearDown(self):
        sys.path.pop(sys.path.index(self.ghtml_module_dir))

    def _remove_nestings(self, data, output):
        """Make list flat

        :param data list: input list with nested list|tuple
        :param output list: output list hold flat list result
        """
        for i in data:
            if isinstance(i, (list, tuple)):
                self._remove_nestings(i, output)
            else:
                output.append(i)

    def test_check_cli_man(self):
        """Test if the CLI man page does not contain the <script> HTML tag"""
        entities = {"nbsp": " ", "bull": "*"}
        os.chdir(os.path.join(self.gisbase, "docs", "html"))
        man_files = glob.glob("*.html")
        for man in man_files:
            parser = self.ghtml.HTMLParser(entities)
            with open(man) as f:
                for line in f:
                    parser.feed(line)
            content = []
            self._remove_nestings(parser.data, content)
            self.assertNotIn("script", content)


if __name__ == "__main__":
    from grass.gunittest.main import test

    test()
