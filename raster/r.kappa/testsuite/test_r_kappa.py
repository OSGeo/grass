"""
Name:      r.kappa tests
Purpose:   Validates accuracy calculations and output

Author:    Maris Nartiss
Copyright: (C) 2022 by Maris Nartiss and the GRASS Development Team
Licence:   This program is free software under the GNU General Public
           License (>=v2). Read the file COPYING that comes with GRASS
           for details.
"""

import os
import pathlib
import json

from tempfile import NamedTemporaryFile

from grass.script import read_command
from grass.script import decode
from grass.script.core import tempname
from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.checkers import keyvalue_equals
from grass.gunittest.utils import xfail_windows


class MatrixCorrectnessTest(TestCase):
    """Test correctness of calculated matrix"""

    @classmethod
    def setUpClass(cls):
        """Import sample maps with known properties"""
        cls.use_temp_region()
        cls.runModule("g.region", n=5, s=0, e=5, w=0, res=1)

        cls.data_dir = os.path.join(pathlib.Path(__file__).parent.absolute(), "data")
        cls.ref_1 = tempname(10)
        cls.runModule(
            "r.in.ascii",
            input=os.path.join(cls.data_dir, "ref_1.ascii"),
            output=cls.ref_1,
        )
        cls.class_1 = tempname(10)
        cls.runModule(
            "r.in.ascii",
            input=os.path.join(cls.data_dir, "class_1.ascii"),
            output=cls.class_1,
        )

    @classmethod
    def tearDownClass(cls):
        """Remove temporary data"""
        cls.del_temp_region()
        cls.runModule(
            "g.remove", flags="f", type="raster", name=(cls.ref_1, cls.class_1)
        )

    def test_m(self):
        """Test printing matrix only
        Implicitly tests calculation of matrix
        """

        ref = """cat#\t1\t2\t3\t4\t5\t6\tRowSum
1\t4\t0\t0\t0\t0\t0\t4
2\t0\t0\t0\t0\t0\t0\t0
3\t0\t1\t4\t0\t0\t0\t5
4\t3\t0\t0\t0\t0\t1\t4
5\t0\t0\t0\t0\t1\t0\t1
6\t0\t2\t0\t0\t0\t2\t4
ColSum\t7\t3\t4\t0\t1\t3\t18

"""
        out = read_command(
            "r.kappa",
            reference=self.ref_1,
            classification=self.class_1,
            flags="m",
            quiet=True,
        )
        self.assertEqual(out, ref)


class CalculationCorrectness1Test(TestCase):
    """Test correctness of values in printout mode
    First version of reference data"""

    @classmethod
    def setUpClass(cls):
        """Import sample maps with known properties"""
        cls.use_temp_region()
        cls.runModule("g.region", n=5, s=0, e=5, w=0, res=1)

        cls.data_dir = os.path.join(pathlib.Path(__file__).parent.absolute(), "data")
        cls.ref_1 = tempname(10)
        cls.runModule(
            "r.in.ascii",
            input=os.path.join(cls.data_dir, "ref_1.ascii"),
            output=cls.ref_1,
        )
        cls.class_1 = tempname(10)
        cls.runModule(
            "r.in.ascii",
            input=os.path.join(cls.data_dir, "class_1.ascii"),
            output=cls.class_1,
        )
        cls.per_class = {
            "producer": [
                100 - 57.143,
                100 - 0.0,
                100 - 100.0,
                "NA",
                100 - 100.0,
                100 - 66.666,
            ],
            "user": [100 - 100.0, "NA", 100 - 80.0, 100 - 0.0, 100 - 100.0, 100 - 50.0],
            "cond.kappa": [1.0, "NA", 0.743, 0.0, 1.0, 0.4],
        }

    @classmethod
    def tearDownClass(cls):
        """Remove temporary data"""
        cls.del_temp_region()
        cls.runModule(
            "g.remove", flags="f", type="raster", name=(cls.ref_1, cls.class_1)
        )

    def match(self, pat, ref):
        if pat == "NA" or ref == "NA":
            return pat == ref
        try:
            val = float(pat)
        except ValueError:
            return False
        return ref - val < 0.001

    # Characteristics of the matrix
    # All values were obtained according to formulas given in
    # Rossiter 2004. Technical Note: Statistical methods for accuracy
    # assessment of classified thematic maps.
    # Only exception is Kappa variance as attempts to follow formulas given
    # by Rossiter lead to a spectacular failure (most likely due to
    # poor math skills of the author of this testing code). Thus Kappa
    # variance is not validated as "good" value is not known.
    # Overall Cohen Kappa value: 0.52091
    # Observation count: 18
    # Correct count: 11
    # Overall accuracy: 0.61111
    # CAT TP TN FP FN Prod  User  pii   pi+   p+i   conditional kappa
    # 1   4  11 0  3  0.571 1.000 0.222 0.222 0.389 1.000
    # 2   0  15 0  3  0.000 -     0.000 0.000 0.167 -
    # 3   4  13 1  0  1.000 0.800 0.222 0.278 0.222 0.743
    # 4   0  14 4  0  -     0.000 0.000 0.222 0.000 0.000
    # 5   1  17 0  0  1.000 1.000 0.056 0.056 0.056 1.000
    # 6   2  13 2  1  0.667 0.500 0.111 0.222 0.167 0.400
    # Correct MCC value was calculated manually and validated with
    # mcc function of R package mltools.

    def test_standard_output(self):
        out = read_command(
            "r.kappa",
            reference=self.ref_1,
            classification=self.class_1,
            flags="w",
            quiet=True,
        )
        rows = out.splitlines()

        # Matrix part
        self.assertEqual(rows[10], " M     1\t4\t0\t0\t0\t0\t0\t4")
        self.assertEqual(rows[11], " A     2\t0\t0\t0\t0\t0\t0\t0")
        self.assertEqual(rows[12], " P     3\t0\t1\t4\t0\t0\t0\t5")
        self.assertEqual(rows[13], " 2     4\t3\t0\t0\t0\t0\t1\t4")
        self.assertEqual(rows[14], "       5\t0\t0\t0\t0\t1\t0\t1")
        self.assertEqual(rows[15], "       6\t0\t2\t0\t0\t0\t2\t4")
        self.assertEqual(rows[16], "Col Sum\t\t7\t3\t4\t0\t1\t3\t18")

        # Per class coefficients
        for c in range(len(self.per_class["producer"])):
            vals = rows[20 + c].split()
            # The test on the next line is valid only for this data
            self.assertEqual(vals[0], str(c + 1))
            self.assertTrue(self.match(vals[1], self.per_class["user"][c]))
            self.assertTrue(self.match(vals[2], self.per_class["producer"][c]))
            self.assertTrue(self.match(vals[3], self.per_class["cond.kappa"][c]))

        # Kappa value
        vals = rows[28].split()
        self.assertTrue(self.match(vals[0], 0.52091))
        self.assertTrue(self.match(vals[2], 0.55930))

        # Overall characteristics
        vals = rows[31].split()
        self.assertEqual(vals[0], "11")
        self.assertEqual(vals[1], "18")
        self.assertTrue(self.match(vals[2], 61.111))


class CalculationCorrectness2Test(TestCase):
    """Test correctness of values in printout mode
    A pathological case when no values match"""

    @classmethod
    def setUpClass(cls):
        """Import sample maps with known properties"""
        cls.use_temp_region()
        cls.runModule("g.region", n=5, s=0, e=5, w=0, res=1)

        cls.data_dir = os.path.join(pathlib.Path(__file__).parent.absolute(), "data")
        cls.ref_1 = tempname(10)
        cls.runModule(
            "r.in.ascii",
            input=os.path.join(cls.data_dir, "ref_2.ascii"),
            output=cls.ref_1,
        )
        cls.class_1 = tempname(10)
        cls.runModule(
            "r.in.ascii",
            input=os.path.join(cls.data_dir, "class_2.ascii"),
            output=cls.class_1,
        )
        cls.per_class = {
            "producer": [
                100.0,
                100.0,
                100.0,
                100.0,
                100.0,
                "NA",
            ],
            "user": ["NA", "NA", "NA", "NA", "NA", 100.0],
            "cond.kappa": ["NA", "NA", "NA", "NA", "NA", 0.0],
        }

    @classmethod
    def tearDownClass(cls):
        """Remove temporary data"""
        cls.del_temp_region()
        cls.runModule(
            "g.remove", flags="f", type="raster", name=(cls.ref_1, cls.class_1)
        )

    def match(self, pat, ref):
        if pat == "NA" or ref == "NA":
            return pat == ref
        try:
            val = float(pat)
        except ValueError:
            return False
        return ref - val < 0.001

    # Characteristics of the matrix
    # Overall Cohen Kappa value: 0.0
    # Observation count: 25
    # Correct count: 0
    # Overall accuracy: 0.0
    # CAT TP  TN  FP  FN  User Producer
    # 0   0   17  0   8   -    0
    # 1   0   17  0   8   -    0
    # 2   0   21  0   4   -    0
    # 3   0   24  0   1   -    0
    # 4   0   21  0   4   -    0
    # 9   0   0   25  0   0    -

    def test_standard_output(self):
        out = read_command(
            "r.kappa",
            reference=self.ref_1,
            classification=self.class_1,
            flags="w",
            quiet=True,
        )
        rows = out.splitlines()

        # Matrix part
        self.assertEqual(rows[10], " M     0\t0\t0\t0\t0\t0\t0\t0")
        self.assertEqual(rows[11], " A     1\t0\t0\t0\t0\t0\t0\t0")
        self.assertEqual(rows[12], " P     2\t0\t0\t0\t0\t0\t0\t0")
        self.assertEqual(rows[13], " 2     3\t0\t0\t0\t0\t0\t0\t0")
        self.assertEqual(rows[14], "       4\t0\t0\t0\t0\t0\t0\t0")
        self.assertEqual(rows[15], "       9\t4\t8\t8\t4\t1\t0\t25")
        self.assertEqual(rows[16], "Col Sum\t\t4\t8\t8\t4\t1\t0\t25")

        # Per class coefficients
        for c in range(len(self.per_class["producer"])):
            vals = rows[20 + c].split()
            self.assertTrue(self.match(vals[1], self.per_class["user"][c]))
            self.assertTrue(self.match(vals[2], self.per_class["producer"][c]))
            self.assertTrue(self.match(vals[3], self.per_class["cond.kappa"][c]))

        # Kappa value
        vals = rows[28].split()
        self.assertTrue(self.match(vals[0], 0.0))
        self.assertTrue(self.match(vals[2], "NA"))

        # Overall characteristics
        vals = rows[31].split()
        self.assertEqual(vals[0], "0")
        self.assertEqual(vals[1], "25")
        self.assertTrue(self.match(vals[2], 0.0))


class JSONOutputTest(TestCase):
    """Test printing of parameters in JSON format"""

    @classmethod
    def setUpClass(cls):
        """Import sample maps with known properties"""
        cls.use_temp_region()
        cls.runModule("g.region", n=5, s=0, e=5, w=0, res=1)

        cls.data_dir = os.path.join(pathlib.Path(__file__).parent.absolute(), "data")
        cls.references = []
        cls.classifications = []
        cls.expected_outputs = []
        # Normal case
        cls.references.append(tempname(10))
        cls.runModule(
            "r.in.ascii",
            input=os.path.join(cls.data_dir, "ref_1.ascii"),
            output=cls.references[0],
        )
        cls.classifications.append(tempname(10))
        cls.runModule(
            "r.in.ascii",
            input=os.path.join(cls.data_dir, "class_1.ascii"),
            output=cls.classifications[0],
        )

        cls.expected_outputs.append(
            {
                "reference": cls.references[0],
                "classification": cls.classifications[0],
                "observations": 18,
                "correct": 11,
                "overall_accuracy": 61.111111,
                "kappa": 0.52091,
                "kappa_variance": 0.016871,
                "cats": [1, 2, 3, 4, 5, 6],
                "matrix": [
                    [4, 0, 0, 0, 0, 0],
                    [0, 0, 0, 0, 0, 0],
                    [0, 1, 4, 0, 0, 0],
                    [3, 0, 0, 0, 0, 1],
                    [0, 0, 0, 0, 1, 0],
                    [0, 2, 0, 0, 0, 2],
                ],
                "row_sum": [4, 0, 5, 4, 1, 4],
                "col_sum": [7, 3, 4, 0, 1, 3],
                "producers_accuracy": [57.1429, 0.0, 100.0, None, 100.0, 66.66666],
                "users_accuracy": [100.0, None, 80.0, 0.0, 100.0, 50.0],
                "conditional_kappa": [1.0, None, 0.742857, 0.0, 1.0, 0.400],
                "mcc": 0.55930,
            }
        )

        # Bad case with no correct matches
        cls.references.append(tempname(10))
        cls.runModule(
            "r.in.ascii",
            input=os.path.join(cls.data_dir, "ref_2.ascii"),
            output=cls.references[1],
        )
        cls.classifications.append(tempname(10))
        cls.runModule(
            "r.in.ascii",
            input=os.path.join(cls.data_dir, "class_2.ascii"),
            output=cls.classifications[1],
        )
        cls.expected_outputs.append(
            {
                "reference": cls.references[1],
                "classification": cls.classifications[1],
                "observations": 25,
                "correct": 0,
                "overall_accuracy": 0.0,
                "kappa": 0.0,
                "kappa_variance": 0.0,
                "cats": [0, 1, 2, 3, 4, 9],
                "matrix": [
                    [0, 0, 0, 0, 0, 0],
                    [0, 0, 0, 0, 0, 0],
                    [0, 0, 0, 0, 0, 0],
                    [0, 0, 0, 0, 0, 0],
                    [0, 0, 0, 0, 0, 0],
                    [8, 8, 4, 1, 4, 0],
                ],
                "row_sum": [0, 0, 0, 0, 0, 25],
                "col_sum": [8, 8, 4, 1, 4, 0],
                "producers_accuracy": [0.0, 0.0, 0.0, 0.0, 0.0, None],
                "users_accuracy": [None, None, None, None, None, 0.0],
                "conditional_kappa": [None, None, None, None, None, 0.0],
                "mcc": None,
            }
        )

        # Degenerate case #1
        cls.references.append(tempname(10))
        cls.classifications.append(tempname(10))
        cls.runModule(
            "r.mapcalc",
            expression=f"{cls.references[2]}=null()",
        )
        cls.runModule(
            "r.mapcalc",
            expression=f"{cls.classifications[2]}=null()",
        )
        cls.expected_outputs.append(
            {
                "reference": cls.references[2],
                "classification": cls.classifications[2],
                "observations": 0,
                "correct": 0,
                "overall_accuracy": 0.0,
                "kappa": None,
                "kappa_variance": None,
                "cats": [],
                "matrix": [[]],
                "row_sum": [],
                "col_sum": [],
                "producers_accuracy": [],
                "users_accuracy": [],
                "conditional_kappa": [],
                "mcc": None,
            }
        )

        # Degenerate case #2
        cls.references.append(tempname(10))
        cls.classifications.append(tempname(10))
        cls.runModule(
            "r.mapcalc",
            expression=f"{cls.references[3]}=1",
        )
        cls.runModule(
            "r.mapcalc",
            expression=f"{cls.classifications[3]}=null()",
        )
        cls.expected_outputs.append(
            {
                "reference": cls.references[3],
                "classification": cls.classifications[3],
                "observations": 0,
                "correct": 0,
                "overall_accuracy": 0.0,
                "kappa": None,
                "kappa_variance": None,
                "cats": [],
                "matrix": [[]],
                "row_sum": [],
                "col_sum": [],
                "producers_accuracy": [],
                "users_accuracy": [],
                "conditional_kappa": [],
                "mcc": None,
            }
        )

    @classmethod
    def tearDownClass(cls):
        """Remove temporary data"""
        cls.del_temp_region()
        for reference in cls.references:
            cls.runModule("g.remove", flags="f", type="raster", name=reference)
        for classification in cls.classifications:
            cls.runModule("g.remove", flags="f", type="raster", name=classification)

    def test_stdout(self):
        for i in range(len(self.references)):
            out = read_command(
                "r.kappa",
                reference=self.references[i],
                classification=self.classifications[i],
                format="json",
                quiet=True,
            )
            json_out = json.loads(decode(out))
            self.assertTrue(
                keyvalue_equals(self.expected_outputs[i], json_out, precision=4)
            )

    @xfail_windows
    def test_file(self):
        for i in range(len(self.references)):
            f = NamedTemporaryFile()
            self.runModule(
                "r.kappa",
                reference=self.references[i],
                classification=self.classifications[i],
                output=f.name,
                format="json",
                overwrite=True,
            )
            json_out = json.loads(f.read())
            self.assertTrue(
                keyvalue_equals(self.expected_outputs[i], json_out, precision=4)
            )


if __name__ == "__main__":
    test()
