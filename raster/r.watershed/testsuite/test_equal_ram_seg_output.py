"""
Name:      test_equal_ram_seg_output.py
Purpose:   Ensure equal output produced by the in-memory (ram) and
           segmentation library (seg) versions of r.watershed.

Author:    Michel Wortmann
Copyright: (C) 2022 by Michel Wortmann and the GRASS Development Team
Licence:   This program is free software under the GNU General Public
           License (>=v2). Read the file COPYING that comes with GRASS
           for details.
"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class TestEqualRamSegOutput(TestCase):
    """Test case for watershed module"""

    elevation = "elevation"

    output_precision = {
        "accumulation": 1,
        "tci": 0.01,
        "spi": 0.01,
        "drainage": 0,
        "basin": 0,
        "stream": 0,
        "half_basin": 0,
        "length_slope": 0.01,
        "slope_steepness": 0.01,
    }

    tmp_input_rasters = ["random_fraction", "random_percent"]

    inputs = [
        {},  # required only
        {"flags": "s"},
        {"flags": "4"},
        {"depression": "random_fraction"},
        {"flow": "random_fraction"},
        {"disturbed_land": "random_percent"},
        {"blocking": "random_fraction"},
        {"retention": "random_percent"},
    ]

    @property
    def outputs(self):
        return list(self.output_precision)

    @classmethod
    def setUpClass(cls):
        """Ensures expected computational region and setup"""
        # Always use the computational region of the raster elevation
        cls.use_temp_region()
        cls.runModule("g.region", raster=cls.elevation)

        # random points raster
        cls.runModule(
            "r.random",
            input=cls.elevation,
            npoints=10000,
            raster="random_fraction",
            seed=1234,
            overwrite=True,
        )
        cls.runModule(
            "r.mapcalc",
            expression="random_percent=random_fraction*100",
            overwrite=True,
        )

    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region"""
        cls.runModule("g.remove", flags="f", type="raster", name=cls.tmp_input_rasters)
        cls.del_temp_region()

    def tearDown(self):
        """Remove the outputs created from the watershed module

        This is executed after each test run.
        """
        self.runModule(
            "g.remove",
            flags="f",
            type="raster",
            pattern=",".join([o + "__*" for o in self.outputs]),
        )

    def same_ram_seg_output(self, outputs=None, **input_args):
        """Check if the output of the ram and seg version is the same."""

        outputs = outputs or self.outputs

        flags = {"ram": "", "seg": "m"}
        kw = {
            "elevation": self.elevation,
            "threshold": 1000,
            "overwrite": True,
        }
        kw.update(input_args)
        # run module with/without -m
        for n, f in flags.items():
            # add outputs
            kw.update({o: "%s__%s" % (o, n) for o in outputs})
            kw["flags"] = input_args.get("flags", "") + f
            self.assertModule("r.watershed", **kw)

        # check difference of outputs
        msg = "ram and seg version output %s is not the same"
        msg += " with input " + str(input_args) if input_args else ""
        passes = []
        for o in outputs:
            # subTest unfortunately doesnt work here
            # with self.subTest("Testing difference in ram vs seg output", output=o):
            prec = self.output_precision.get(o, 0)
            try:
                self.assertRastersNoDifference(
                    "%s__ram" % o, "%s__seg" % o, prec, msg=msg % o
                )
                passes.append(True)
            except AssertionError:
                passes.append(False)
        return passes

    def test_same_ram_seg_output(self):
        passes = []
        for oi in self.inputs:
            passes.append(self.same_ram_seg_output(**oi))

        # create nice markdown table of matches
        msg = (
            "Output of ram and seg versions of r.watershed do not match:"
            "\n\n" + self.md_table(passes) + "\n"
        )

        self.assertTrue(all(all(p) for p in passes), msg=msg)

    @staticmethod
    def columns(cells):
        return "| " + (" | ".join(map(str, cells))) + " |"

    def md_table(self, passes):
        strinpts = self.columns(
            [", ".join(["%s=%s" % kw for kw in d.items()]) for d in self.inputs]
        )
        for ir in self.tmp_input_rasters:
            strinpts = strinpts.replace("=" + ir, "")
        msg = "| Output " + strinpts + "\n"
        msg += self.columns(["---"] * (len(self.inputs) + 1)) + "\n"
        symbols = {True: ":white_check_mark:", False: ":red_circle:"}
        for o, p in zip(self.outputs, zip(*passes, strict=False), strict=False):
            sym = [symbols[b] for b in p]
            msg += ("| %s " % o) + self.columns(sym) + "\n"
        return msg


if __name__ == "__main__":
    test()
