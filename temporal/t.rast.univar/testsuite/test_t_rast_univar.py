"""Test t.rast.univar

(C) 2014 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Soeren Gebbert
"""

from pathlib import Path

from grass.gunittest.case import TestCase
from grass.gunittest.gmodules import SimpleModule
from grass.gunittest.utils import xfail_windows


class TestRasterUnivar(TestCase):
    default_region = {"s": 0, "n": 80, "w": 0, "e": 120, "b": 0, "t": 50}

    @classmethod
    def setUpClass(cls):
        """Initiate the temporal GIS and set the region"""
        cls.use_temp_region()
        for idx, region_extent in enumerate(
            [
                [0, 80, 0, 120],
                [-80, 0, 0, 120],
                [0, 80, -120, 0],
                [-80, 0, -120, 0],
            ]
        ):
            cls.runModule(
                "g.region",
                s=region_extent[0],
                n=region_extent[1],
                w=region_extent[2],
                e=region_extent[3],
                b=0,
                t=50,
                res=1,
            )
            cls.runModule(
                "r.mapcalc", expression=f"d_{idx + 1} = {idx + 1}00", overwrite=True
            )

        cls.runModule("g.region", **cls.default_region, res=1, res3=1)

        cls.runModule(
            "r.mapcalc",
            expression="zones = int(if(row() <= 5, 1, if(row() >= 20, 3, 2)))",
            overwrite=True,
        )

        cls.runModule("r.mapcalc", expression="a_1 = 100", overwrite=True)
        cls.runModule("r.mapcalc", expression="a_2 = 200", overwrite=True)
        cls.runModule("r.mapcalc", expression="a_3 = 300", overwrite=True)
        cls.runModule("r.mapcalc", expression="a_4 = 400", overwrite=True)

        cls.runModule("r.mapcalc", expression="b_1 = 110", overwrite=True)
        cls.runModule("r.mapcalc", expression="b_2 = 220", overwrite=True)
        cls.runModule("r.mapcalc", expression="b_3 = 330", overwrite=True)
        cls.runModule("r.mapcalc", expression="b_4 = 440", overwrite=True)

        cls.runModule("r.mapcalc", expression="c_1 = 111", overwrite=True)
        cls.runModule("r.mapcalc", expression="c_2 = 222", overwrite=True)
        cls.runModule("r.mapcalc", expression="c_3 = 333", overwrite=True)
        cls.runModule("r.mapcalc", expression="c_4 = 444", overwrite=True)

        cls.runModule("r.support", map="b_1", semantic_label="S2_B1", overwrite=True)
        cls.runModule("r.support", map="b_2", semantic_label="S2_B1", overwrite=True)
        cls.runModule("r.support", map="b_3", semantic_label="S2_B1", overwrite=True)
        cls.runModule("r.support", map="b_4", semantic_label="S2_B1", overwrite=True)

        cls.runModule("r.support", map="c_1", semantic_label="S2_C1", overwrite=True)
        cls.runModule("r.support", map="c_2", semantic_label="S2_C1", overwrite=True)
        cls.runModule("r.support", map="c_3", semantic_label="S2_C1", overwrite=True)
        cls.runModule("r.support", map="c_4", semantic_label="S2_C1", overwrite=True)

        cls.runModule(
            "t.create",
            type="strds",
            temporaltype="absolute",
            output="A",
            title="A test",
            description="A test",
            overwrite=True,
        )
        cls.runModule(
            "t.create",
            type="strds",
            temporaltype="absolute",
            output="B",
            title="B test",
            description="B test",
            overwrite=True,
        )
        cls.runModule(
            "t.create",
            type="strds",
            temporaltype="absolute",
            output="C",
            title="C test",
            description="C test",
            overwrite=True,
        )
        cls.runModule(
            "t.register",
            flags="i",
            type="raster",
            input="A",
            maps="a_1,a_2,a_3,a_4",
            start="2001-01-01",
            increment="3 months",
            overwrite=True,
        )
        cls.runModule(
            "t.register",
            flags="i",
            type="raster",
            input="B",
            maps="b_1,b_2,b_3,b_4",
            start="2001-01-01",
            increment="3 months",
            overwrite=True,
        )
        cls.runModule(
            "t.register",
            flags="i",
            type="raster",
            input="B",
            maps="c_1,c_2,c_3,c_4",
            start="2001-01-01",
            increment="3 months",
            overwrite=True,
        )
        cls.runModule(
            "t.register",
            flags="i",
            type="raster",
            input="C",
            maps="d_1,d_2,d_3,d_4",
            start="2001-01-01",
            increment="3 months",
            overwrite=True,
        )

    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region"""
        cls.runModule("t.remove", flags="df", type="strds", inputs="A,B")
        cls.runModule("g.remove", flags="f", type="raster", name="zones")

        cls.del_temp_region()

    @xfail_windows
    def test_with_all_maps(self):
        t_rast_univar = SimpleModule(
            "t.rast.univar",
            input="A",
            where="start_time >= '2001-01-01'",
            overwrite=True,
            verbose=True,
        )
        self.runModule("g.region", **self.default_region, res=1)
        self.assertModule(t_rast_univar)

        univar_text = """id|semantic_label|start|end|mean|min|max|mean_of_abs|stddev|variance|coeff_var|sum|null_cells|cells|non_null_cells
a_1@testing||2001-01-01 00:00:00|2001-04-01 00:00:00|100|100|100|100|0|0|0|960000|0|9600|9600
a_2@testing||2001-04-01 00:00:00|2001-07-01 00:00:00|200|200|200|200|0|0|0|1920000|0|9600|9600
a_3@testing||2001-07-01 00:00:00|2001-10-01 00:00:00|300|300|300|300|0|0|0|2880000|0|9600|9600
a_4@testing||2001-10-01 00:00:00|2002-01-01 00:00:00|400|400|400|400|0|0|0|3840000|0|9600|9600
"""
        for ref, res in zip(
            univar_text.split("\n"), t_rast_univar.outputs.stdout.split("\n")
        ):
            if ref and res:
                ref_line = ref.split("|", 1)[1]
                res_line = res.split("|", 1)[1]
                self.assertLooksLike(ref_line, res_line)

    @xfail_windows
    def test_with_subset_of_maps(self):
        t_rast_univar = SimpleModule(
            "t.rast.univar",
            input="A",
            where="start_time >= '2001-03-01'",
            overwrite=True,
            verbose=True,
        )
        self.runModule("g.region", **self.default_region, res=1)
        self.assertModule(t_rast_univar)

        univar_text = """id|semantic_label|start|end|mean|min|max|mean_of_abs|stddev|variance|coeff_var|sum|null_cells|cells|non_null_cells
a_2@testing||2001-04-01 00:00:00|2001-07-01 00:00:00|200|200|200|200|0|0|0|1920000|0|9600|9600
a_3@testing||2001-07-01 00:00:00|2001-10-01 00:00:00|300|300|300|300|0|0|0|2880000|0|9600|9600
a_4@testing||2001-10-01 00:00:00|2002-01-01 00:00:00|400|400|400|400|0|0|0|3840000|0|9600|9600
"""
        for ref, res in zip(
            univar_text.split("\n"), t_rast_univar.outputs.stdout.split("\n")
        ):
            if ref and res:
                ref_line = ref.split("|", 1)[1]
                res_line = res.split("|", 1)[1]
                self.assertLooksLike(ref_line, res_line)

    @xfail_windows
    def test_coarser_resolution(self):
        t_rast_univar = SimpleModule(
            "t.rast.univar",
            input="A",
            where="start_time >= '2001-03-01'",
            overwrite=True,
            verbose=True,
        )
        self.runModule("g.region", **self.default_region, res=10)
        self.assertModule(t_rast_univar)

        univar_text = """id|semantic_label|start|end|mean|min|max|mean_of_abs|stddev|variance|coeff_var|sum|null_cells|cells|non_null_cells
a_2@testing||2001-04-01 00:00:00|2001-07-01 00:00:00|200|200|200|200|0|0|0|19200|0|96|96
a_3@testing||2001-07-01 00:00:00|2001-10-01 00:00:00|300|300|300|300|0|0|0|28800|0|96|96
a_4@testing||2001-10-01 00:00:00|2002-01-01 00:00:00|400|400|400|400|0|0|0|38400|0|96|96
"""

        for ref, res in zip(
            univar_text.split("\n"), t_rast_univar.outputs.stdout.split("\n")
        ):
            if ref and res:
                ref_line = ref.split("|", 1)[1]
                res_line = res.split("|", 1)[1]
                self.assertLooksLike(ref_line, res_line)

    def test_subset_with_output(self):
        self.runModule("g.region", **self.default_region, res=10)

        self.assertModule(
            "t.rast.univar",
            input="A",
            flags="r",
            output="univar_output.txt",
            where="start_time >= '2001-03-01'",
            overwrite=True,
            verbose=True,
        )

        univar_text = """id|semantic_label|start|end|mean|min|max|mean_of_abs|stddev|variance|coeff_var|sum|null_cells|cells|non_null_cells
a_2@testing||2001-04-01 00:00:00|2001-07-01 00:00:00|200|200|200|200|0|0|0|1920000|0|9600|9600
a_3@testing||2001-07-01 00:00:00|2001-10-01 00:00:00|300|300|300|300|0|0|0|2880000|0|9600|9600
a_4@testing||2001-10-01 00:00:00|2002-01-01 00:00:00|400|400|400|400|0|0|0|3840000|0|9600|9600
"""
        univar_output = Path("univar_output.txt").read_text()

        for ref, res in zip(univar_text.split("\n"), univar_output.split("\n")):
            if ref and res:
                ref_line = ref.split("|", 1)[1]
                res_line = res.split("|", 1)[1]
                self.assertLooksLike(ref_line, res_line)

    def test_subset_with_extended_statistics_and_output(self):
        self.runModule("g.region", res=10)
        self.assertModule(
            "t.rast.univar",
            input="A",
            flags="er",
            output="univar_output.txt",
            where="start_time >= '2001-03-01'",
            percentile=[10.0, 97.5],
            overwrite=True,
            verbose=True,
        )

        univar_text = """id|semantic_label|start|end|mean|min|max|mean_of_abs|stddev|variance|coeff_var|sum|null_cells|cells|non_null_cells|first_quartile|median|third_quartile|percentile_10|percentile_97_5
a_2@m2||2001-04-01 00:00:00|2001-07-01 00:00:00|200|200|200|200|0|0|0|1920000|0|9600|9600|200|200|200|200|200
a_3@m2||2001-07-01 00:00:00|2001-10-01 00:00:00|300|300|300|300|0|0|0|2880000|0|9600|9600|300|300|300|300|300
a_4@m2||2001-10-01 00:00:00|2002-01-01 00:00:00|400|400|400|400|0|0|0|3840000|0|9600|9600|400|400|400|400|400
"""
        univar_output = Path("univar_output.txt").read_text()

        for ref, res in zip(univar_text.split("\n"), univar_output.split("\n")):
            if ref and res:
                ref_line = ref.split("|", 1)[1]
                res_line = res.split("|", 1)[1]
                self.assertLooksLike(ref_line, res_line)

        self.runModule("g.region", **self.default_region, res=10)
        self.assertModule(
            "t.rast.univar",
            input="A",
            flags="ru",
            output="univar_output.txt",
            where="start_time >= '2001-03-01'",
            overwrite=True,
            verbose=True,
        )

        univar_text = """a_2@testing||2001-04-01 00:00:00|2001-07-01 00:00:00|200|200|200|200|0|0|0|1920000|0|9600|9600
a_3@testing||2001-07-01 00:00:00|2001-10-01 00:00:00|300|300|300|300|0|0|0|2880000|0|9600|9600
a_4@testing||2001-10-01 00:00:00|2002-01-01 00:00:00|400|400|400|400|0|0|0|3840000|0|9600|9600
"""
        univar_output = Path("univar_output.txt").read_text()

        for ref, res in zip(univar_text.split("\n"), univar_output.split("\n")):
            if ref and res:
                ref_line = ref.split("|", 1)[1]
                res_line = res.split("|", 1)[1]
                self.assertLooksLike(ref_line, res_line)

    def test_error_handling_empty_strds(self):
        # Empty strds
        self.assertModule(
            "t.rast.univar",
            input="A",
            output="univar_output.txt",
            where="start_time >= '2015-03-01'",
            overwrite=True,
            verbose=True,
        )

    def test_error_handling_no_input(self):
        # No input
        self.assertModuleFail("t.rast.univar", output="out.txt")

    @xfail_windows
    def test_with_zones(self):
        """Test use of zones"""

        t_rast_univar = SimpleModule(
            "t.rast.univar",
            input="A",
            where="start_time >= '2001-01-01'",
            zones="zones",
            overwrite=True,
            verbose=True,
        )
        self.runModule("g.region", **self.default_region, res=1)
        self.assertModule(t_rast_univar)

        univar_text = """id|semantic_label|start|end|zone|mean|min|max|mean_of_abs|stddev|variance|coeff_var|sum|null_cells|cells|non_null_cells
a_1@PERMANENT||2001-01-01 00:00:00|2001-04-01 00:00:00|1|100|100|100|100|0|0|0|60000|0|600|600
a_1@PERMANENT||2001-01-01 00:00:00|2001-04-01 00:00:00|2|100|100|100|100|0|0|0|168000|0|1680|1680
a_1@PERMANENT||2001-01-01 00:00:00|2001-04-01 00:00:00|3|100|100|100|100|0|0|0|732000|0|7320|7320
a_2@PERMANENT||2001-04-01 00:00:00|2001-07-01 00:00:00|1|200|200|200|200|0|0|0|120000|0|600|600
a_2@PERMANENT||2001-04-01 00:00:00|2001-07-01 00:00:00|2|200|200|200|200|0|0|0|336000|0|1680|1680
a_2@PERMANENT||2001-04-01 00:00:00|2001-07-01 00:00:00|3|200|200|200|200|0|0|0|1464000|0|7320|7320
a_3@PERMANENT||2001-07-01 00:00:00|2001-10-01 00:00:00|1|300|300|300|300|0|0|0|180000|0|600|600
a_3@PERMANENT||2001-07-01 00:00:00|2001-10-01 00:00:00|2|300|300|300|300|0|0|0|504000|0|1680|1680
a_3@PERMANENT||2001-07-01 00:00:00|2001-10-01 00:00:00|3|300|300|300|300|0|0|0|2196000|0|7320|7320
a_4@PERMANENT||2001-10-01 00:00:00|2002-01-01 00:00:00|1|400|400|400|400|0|0|0|240000|0|600|600
a_4@PERMANENT||2001-10-01 00:00:00|2002-01-01 00:00:00|2|400|400|400|400|0|0|0|672000|0|1680|1680
a_4@PERMANENT||2001-10-01 00:00:00|2002-01-01 00:00:00|3|400|400|400|400|0|0|0|2928000|0|7320|7320
"""

        for ref, res in zip(
            univar_text.split("\n"), t_rast_univar.outputs.stdout.split("\n")
        ):
            if ref and res:
                ref_line = ref.split("|", 1)[1]
                res_line = res.split("|", 1)[1]
                self.assertLooksLike(ref_line, res_line)

    @xfail_windows
    def test_with_zones_and_r(self):
        """Test use of zones and r-flag"""

        t_rast_univar = SimpleModule(
            "t.rast.univar",
            flags="r",
            input="A",
            where="start_time >= '2001-01-01'",
            zones="zones",
            overwrite=True,
            verbose=True,
        )
        self.runModule("g.region", **self.default_region, res=1)
        self.assertModule(t_rast_univar)

        univar_text = """id|semantic_label|start|end|zone|mean|min|max|mean_of_abs|stddev|variance|coeff_var|sum|null_cells|cells|non_null_cells
a_1@PERMANENT||2001-01-01 00:00:00|2001-04-01 00:00:00|1|100|100|100|100|0|0|0|60000|0|600|600
a_1@PERMANENT||2001-01-01 00:00:00|2001-04-01 00:00:00|2|100|100|100|100|0|0|0|168000|0|1680|1680
a_1@PERMANENT||2001-01-01 00:00:00|2001-04-01 00:00:00|3|100|100|100|100|0|0|0|732000|0|7320|7320
a_2@PERMANENT||2001-04-01 00:00:00|2001-07-01 00:00:00|1|200|200|200|200|0|0|0|120000|0|600|600
a_2@PERMANENT||2001-04-01 00:00:00|2001-07-01 00:00:00|2|200|200|200|200|0|0|0|336000|0|1680|1680
a_2@PERMANENT||2001-04-01 00:00:00|2001-07-01 00:00:00|3|200|200|200|200|0|0|0|1464000|0|7320|7320
a_3@PERMANENT||2001-07-01 00:00:00|2001-10-01 00:00:00|1|300|300|300|300|0|0|0|180000|0|600|600
a_3@PERMANENT||2001-07-01 00:00:00|2001-10-01 00:00:00|2|300|300|300|300|0|0|0|504000|0|1680|1680
a_3@PERMANENT||2001-07-01 00:00:00|2001-10-01 00:00:00|3|300|300|300|300|0|0|0|2196000|0|7320|7320
a_4@PERMANENT||2001-10-01 00:00:00|2002-01-01 00:00:00|1|400|400|400|400|0|0|0|240000|0|600|600
a_4@PERMANENT||2001-10-01 00:00:00|2002-01-01 00:00:00|2|400|400|400|400|0|0|0|672000|0|1680|1680
a_4@PERMANENT||2001-10-01 00:00:00|2002-01-01 00:00:00|3|400|400|400|400|0|0|0|2928000|0|7320|7320
"""

        for ref, res in zip(
            univar_text.split("\n"), t_rast_univar.outputs.stdout.split("\n")
        ):
            if ref and res:
                ref_line = ref.split("|", 1)[1]
                res_line = res.split("|", 1)[1]
                self.assertLooksLike(ref_line, res_line)

    @xfail_windows
    def test_with_semantic_label(self):
        """Test semantic labels"""
        t_rast_univar = SimpleModule(
            "t.rast.univar",
            input="B.S2_B1",
            where="start_time >= '2001-01-01'",
            overwrite=True,
            verbose=True,
        )
        self.runModule("g.region", **self.default_region, res=1)
        self.assertModule(t_rast_univar)

        univar_text = """id|semantic_label|start|end|mean|min|max|mean_of_abs|stddev|variance|coeff_var|sum|null_cells|cells|non_null_cells
b_1@PERMANENT|S2_B1|2001-01-01 00:00:00|2001-04-01 00:00:00|110|110|110|110|0|0|0|1056000|0|9600|9600
b_2@PERMANENT|S2_B1|2001-04-01 00:00:00|2001-07-01 00:00:00|220|220|220|220|0|0|0|2112000|0|9600|9600
b_3@PERMANENT|S2_B1|2001-07-01 00:00:00|2001-10-01 00:00:00|330|330|330|330|0|0|0|3168000|0|9600|9600
b_4@PERMANENT|S2_B1|2001-10-01 00:00:00|2002-01-01 00:00:00|440|440|440|440|0|0|0|4224000|0|9600|9600
"""
        for ref, res in zip(
            univar_text.split("\n"), t_rast_univar.outputs.stdout.split("\n")
        ):
            if ref and res:
                ref_line = ref.split("|", 1)[1]
                res_line = res.split("|", 1)[1]
                self.assertLooksLike(ref_line, res_line)

    @xfail_windows
    def test_with_semantic_label_parallel(self):
        """Test semantic labels"""
        t_rast_univar = SimpleModule(
            "t.rast.univar",
            input="B.S2_B1",
            where="start_time >= '2001-01-01'",
            nprocs=2,
            overwrite=True,
            verbose=True,
        )
        self.runModule("g.region", **self.default_region, res=1)
        self.assertModule(t_rast_univar)

        univar_text = """id|semantic_label|start|end|mean|min|max|mean_of_abs|stddev|variance|coeff_var|sum|null_cells|cells|non_null_cells
b_1@PERMANENT|S2_B1|2001-01-01 00:00:00|2001-04-01 00:00:00|110|110|110|110|0|0|0|1056000|0|9600|9600
b_2@PERMANENT|S2_B1|2001-04-01 00:00:00|2001-07-01 00:00:00|220|220|220|220|0|0|0|2112000|0|9600|9600
b_3@PERMANENT|S2_B1|2001-07-01 00:00:00|2001-10-01 00:00:00|330|330|330|330|0|0|0|3168000|0|9600|9600
b_4@PERMANENT|S2_B1|2001-10-01 00:00:00|2002-01-01 00:00:00|440|440|440|440|0|0|0|4224000|0|9600|9600
"""
        for ref, res in zip(
            univar_text.split("\n"), t_rast_univar.outputs.stdout.split("\n")
        ):
            if ref and res:
                ref_line = ref.split("|", 1)[1]
                res_line = res.split("|", 1)[1]
                self.assertLooksLike(ref_line, res_line)

    @xfail_windows
    def test_with_spatial_filter_intersects(self):
        """Test spatial filter overlaps"""
        t_rast_univar = SimpleModule(
            "t.rast.univar",
            input="C",
            where="start_time >= '2001-01-01'",
            nprocs=2,
            region_relation="overlaps",
            overwrite=True,
            verbose=True,
        )
        self.runModule("g.region", res=1, s=-5, n=85, w=-5, e=125)
        self.assertModule(t_rast_univar)

        univar_text = """id|semantic_label|start|end|mean|min|max|mean_of_abs|stddev|variance|coeff_var|sum|null_cells|cells|non_null_cells
d_1@stbl||2001-01-01 00:00:00|2001-04-01 00:00:00|100|100|100|100|0|0|0|960000|2100|9600|9600
d_2@stbl||2001-04-01 00:00:00|2001-07-01 00:00:00|200|200|200|200|0|0|0|120000|11100|600|600
d_3@stbl||2001-07-01 00:00:00|2001-10-01 00:00:00|300|300|300|300|0|0|0|120000|11300|400|400
d_4@stbl||2001-10-01 00:00:00|2002-01-01 00:00:00|400|400|400|400|0|0|0|10000|11675|25|25
"""
        for ref, res in zip(
            univar_text.split("\n"), t_rast_univar.outputs.stdout.split("\n")
        ):
            if ref and res:
                ref_line = ref.split("|", 1)[1]
                res_line = res.split("|", 1)[1]
                self.assertLooksLike(ref_line, res_line)

    @xfail_windows
    def test_with_spatial_filter_contains(self):
        """Test spatial filter contains"""
        t_rast_univar = SimpleModule(
            "t.rast.univar",
            input="C",
            where="start_time >= '2001-01-01'",
            nprocs=2,
            region_relation="contains",
            overwrite=True,
            verbose=True,
        )
        self.runModule("g.region", res=1, s=5, n=75, w=5, e=115)
        self.assertModule(t_rast_univar)

        print(t_rast_univar.outputs.stdout)
        univar_text = """id|semantic_label|start|end|mean|min|max|mean_of_abs|stddev|variance|coeff_var|sum|null_cells|cells|non_null_cells
d_1@stbl||2001-01-01 00:00:00|2001-04-01 00:00:00|100|100|100|100|0|0|0|770000|0|7700|7700
"""
        for ref, res in zip(
            univar_text.split("\n"), t_rast_univar.outputs.stdout.split("\n")
        ):
            if ref and res:
                ref_line = ref.split("|", 1)[1]
                res_line = res.split("|", 1)[1]
                self.assertLooksLike(ref_line, res_line)

    @xfail_windows
    def test_with_spatial_filter_is_contained(self):
        """Test spatial filter is_contained"""
        t_rast_univar = SimpleModule(
            "t.rast.univar",
            input="C",
            where="start_time >= '2001-01-01'",
            nprocs=2,
            region_relation="is_contained",
            overwrite=True,
            verbose=True,
        )
        self.runModule("g.region", res=1, s=-5, n=85, w=-5, e=125)
        self.assertModule(t_rast_univar)

        univar_text = """id|semantic_label|start|end|mean|min|max|mean_of_abs|stddev|variance|coeff_var|sum|null_cells|cells|non_null_cells
d_1@stbl||2001-01-01 00:00:00|2001-04-01 00:00:00|100|100|100|100|0|0|0|960000|2100|9600|9600
"""
        for ref, res in zip(
            univar_text.split("\n"), t_rast_univar.outputs.stdout.split("\n")
        ):
            if ref and res:
                ref_line = ref.split("|", 1)[1]
                res_line = res.split("|", 1)[1]
                self.assertLooksLike(ref_line, res_line)


if __name__ == "__main__":
    from grass.gunittest.main import test

    test()
