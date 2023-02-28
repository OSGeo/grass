"""Test t.register

(C) 2014-2023 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Soeren Gebbert
        Ported to Python by Stefan Blumentrath

This is a test to register and unregister raster maps in
space time raster input.
The raster maps will be registered in different space time raster
inputs

We need to set a specific region in the
@preprocess step of this test. We generate
raster with r.mapcalc and create two space time raster inputs
with absolute time
The region setting should work for UTM and LL test locations

"""

import grass.script as gs

from grass.gunittest.case import TestCase
from grass.gunittest.gmodules import SimpleModule

"""
eval `g.gisenv`
n1=`g.tempfile pid=1 -d` # Only map names
n2=`g.tempfile pid=2 -d` # Map names and start time
n3=`g.tempfile pid=3 -d` # Map names start time and increment
n4=`g.tempfile pid=4 -d` # Full map names, start time, end time and semantic label

cat > "${n1}" << EOF
EOF
cat "${n1}"

cat > "${n2}" << EOF
EOF
cat "${n2}"

cat > "${n3}" << EOF
EOF
cat "${n3}"

cat > "${n4}" << EOF
EOF
cat "${n4}"

# The first @test
# We create the space time raster inputs and register the raster maps with absolute time interval


# Test with input files
# File 1
# File 1
# File 2
# File 2
# File 3
# File 4
t.register --o input=precip_abs8 file="${n4}"
t.info type=strds input=precip_abs8
t.rast.list input=precip_abs8

t.remove --v type=strds input=precip_abs8
t.unregister --v type=raster file="${n1}"
"""


class TestRasterUnivar(TestCase):
    @classmethod
    def setUpClass(cls):
        """Initiate the temporal GIS and set the region"""
        cls.use_temp_region()
        cls.runModule(
            "g.region", s=0, n=80, w=0, e=120, b=0, t=50, res=10, res3=10, flags="p3"
        )

        # Generate data
        cls.runModule(
            "r.mapcalc", flags="s", expression="prec_1 = rand(0, 550)", overwrite=True
        )
        cls.runModule(
            "r.mapcalc", flags="s", expression="prec_2 = rand(0, 450)", overwrite=True
        )
        cls.runModule(
            "r.mapcalc", flags="s", expression="prec_3 = rand(0, 320)", overwrite=True
        )
        cls.runModule(
            "r.mapcalc", flags="s", expression="prec_4 = rand(0, 510)", overwrite=True
        )
        cls.runModule(
            "r.mapcalc", flags="s", expression="prec_5 = rand(0, 300)", overwrite=True
        )
        cls.runModule(
            "r.mapcalc", flags="s", expression="prec_6 = rand(0, 650)", overwrite=True
        )

        cls.runModule(
            "t.create",
            type="strds",
            temporaltype="absolute",
            output="precip_abs8",
            title="A test with input files",
            description="A test with input files",
            overwrite=True,
        )

    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region"""
        cls.runModule("t.remove", flags="df", type="strds", inputs="precip_abs8")
        cls.del_temp_region()

    def test_with_file_and_increment(self):

        tmp_file = gs.tempfile()
        with open(tmp_file, "w") as register_file:
            register_file.write("prec_1\nprec_2\nprec_3\nprec_4\nprec_5\nprec_6")

        register_module = SimpleModule(
            "t.register",
            input="precip_abs8",
            start="2001-01-01",
            increment="1 months",
            file=tmp_file,
            overwrite=True,
            verbose=True,
        )
        self.assertModule(register_module)

        # maplist = SimpleModule("t.rast.list",
        # input="precip_abs8")

    def test_with_file_and_no_increment(self):

        tmp_file = gs.tempfile()
        with open(tmp_file, "w") as register_file:
            register_file.write("prec_1\nprec_2\nprec_3\nprec_4\nprec_5\nprec_6")

        register_module = SimpleModule(
            "t.register",
            input="precip_abs8",
            start="2001-01-01",
            file=tmp_file,
            overwrite=True,
            verbose=True,
        )
        self.assertModule(register_module)

        # t.info type=strds input=precip_abs8
        # t.rast.list input=precip_abs8

    def test_with_start_in_file(self):

        tmp_file = gs.tempfile()
        with open(tmp_file, "w") as register_file:
            register_file.write(
                "\n".join(
                    [
                        "prec_1|2001-01-01",
                        "prec_2|2001-02-01",
                        "prec_3|2001-03-01",
                        "prec_4|2001-04-01",
                        "prec_5|2001-05-01",
                        "prec_6|2001-06-01",
                    ]
                )
            )

        register_module = SimpleModule(
            "t.register",
            input="precip_abs8",
            file=tmp_file,
            overwrite=True,
            verbose=True,
        )
        self.assertModule(register_module)

        # t.info type=strds input=precip_abs8
        # t.rast.list input=precip_abs8

    def test_with_start_in_file_and_increment(self):

        tmp_file = gs.tempfile()
        with open(tmp_file, "w") as register_file:
            register_file.write(
                "\n".join(
                    [
                        "prec_1|2001-01-01",
                        "prec_2|2001-02-01",
                        "prec_3|2001-03-01",
                        "prec_4|2001-04-01",
                        "prec_5|2001-05-01",
                        "prec_6|2001-06-01",
                    ]
                )
            )

        register_module = SimpleModule(
            "t.register",
            input="precip_abs8",
            increment="1 months",
            file=tmp_file,
            overwrite=True,
            verbose=True,
        )
        self.assertModuleFail(register_module)

        # t.info type=strds input=precip_abs8
        # t.rast.list input=precip_abs8

    def test_with_start_and_end_in_file_and_interval(self):

        tmp_file = gs.tempfile()
        with open(tmp_file, "w") as register_file:
            register_file.write(
                "\n".join(
                    [
                        "prec_1|2001-01-01|2001-04-01",
                        "prec_2|2001-04-01|2001-07-01",
                        "prec_3|2001-07-01|2001-10-01",
                        "prec_4|2001-10-01|2002-01-01",
                        "prec_5|2002-01-01|2002-04-01",
                        "prec_6|2002-04-01|2002-07-01",
                    ]
                )
            )

        register_module = SimpleModule(
            "t.register",
            flags="i",
            input="precip_abs8",
            file=tmp_file,
            overwrite=True,
            verbose=True,
        )
        self.assertModuleFail(register_module)

        # t.info type=strds input=precip_abs8
        # t.rast.list input=precip_abs8

    def test_with_mapset_and_semantic_label(self):

        mapset = gs.gisenv()["MAPSET"]
        tmp_file = gs.tempfile()
        with open(tmp_file, "w") as register_file:
            register_file.write(
                "\n".join(
                    [
                        f"prec_1@{mapset}|2001-01-01|2001-04-01|semantic_label",
                        f"prec_2@{mapset}|2001-04-01|2001-07-01|semantic_label",
                        f"prec_3@{mapset}|2001-07-01|2001-10-01|semantic_label",
                        f"prec_4@{mapset}|2001-10-01|2002-01-01|semantic_label",
                        f"prec_5@{mapset}|2002-01-01|2002-04-01|semantic_label",
                        f"prec_6@{mapset}|2002-04-01|2002-07-01|semantic_label",
                    ]
                )
            )

        register_module = SimpleModule(
            "t.register",
            flags="i",
            input="precip_abs8",
            file=tmp_file,
            overwrite=True,
            verbose=True,
        )
        self.assertModuleFail(register_module)

        # t.info type=strds input=precip_abs8
        # t.rast.list input=precip_abs8


if __name__ == "__main__":
    from grass.gunittest.main import test

    test()
