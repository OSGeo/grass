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
prec_1|2001-01-01
prec_2|2001-02-01
prec_3|2001-03-01
prec_4|2001-04-01
prec_5|2001-05-01
prec_6|2001-06-01
EOF
cat "${n2}"

cat > "${n3}" << EOF
prec_1|2001-01-01|2001-04-01
prec_2|2001-04-01|2001-07-01
prec_3|2001-07-01|2001-10-01
prec_4|2001-10-01|2002-01-01
prec_5|2002-01-01|2002-04-01
prec_6|2002-04-01|2002-07-01
EOF
cat "${n3}"

cat > "${n4}" << EOF
prec_1@${MAPSET}|2001-01-01|2001-04-01|semantic_label
prec_2@${MAPSET}|2001-04-01|2001-07-01|semantic_label
prec_3@${MAPSET}|2001-07-01|2001-10-01|semantic_label
prec_4@${MAPSET}|2001-10-01|2002-01-01|semantic_label
prec_5@${MAPSET}|2002-01-01|2002-04-01|semantic_label
prec_6@${MAPSET}|2002-04-01|2002-07-01|semantic_label
EOF
cat "${n4}"

# The first @test
# We create the space time raster inputs and register the raster maps with absolute time interval


# Test with input files
# File 1
# File 1
t.register --o input=precip_abs8 file="${n1}" start="2001-01-01"
t.info type=strds input=precip_abs8
t.rast.list input=precip_abs8
# File 2
t.register --o input=precip_abs8 file="${n2}"
t.info type=strds input=precip_abs8
t.rast.list input=precip_abs8
# File 2
t.register --o input=precip_abs8 file="${n2}" increment="1 months"
t.info type=strds input=precip_abs8
t.rast.list input=precip_abs8
# File 3
t.register --o -i input=precip_abs8 file="${n3}"
t.info type=strds input=precip_abs8
t.rast.list input=precip_abs8
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
        cls.runModule("g.region", s=0, n=80, w=0, e=120, b=0, t=50, res=10, res3=10, flags="p3")

        # Generate data
        cls.runModule("r.mapcalc", expression="prec_1 = rand(0, 550)", overwrite=True)
        cls.runModule("r.mapcalc", expression="prec_2 = rand(0, 450)", overwrite=True)
        cls.runModule("r.mapcalc", expression="prec_3 = rand(0, 320)", overwrite=True)
        cls.runModule("r.mapcalc", expression="prec_4 = rand(0, 510)", overwrite=True)
        cls.runModule("r.mapcalc", expression="prec_5 = rand(0, 300)", overwrite=True)
        cls.runModule("r.mapcalc", expression="prec_6 = rand(0, 650)", overwrite=True)

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

    def test_with_all_maps(self):

        tmp_file =
        with open(tmp_file, "w") as register_file:
            register_file.write("prec_1\nprec_2\nprec_3\nprec_4\nprec_5\nprec_6")

        register_module = SimpleModule(
            "t.register",
            input="precip_abs8",
            start="2001-01-01",
            where="start_time >= '2001-01-01'",
            increment="1 months",
            file=tmp_file,
            overwrite=True,
            verbose=True,
        )
        self.assertModule(register_module)

        maplist = SimpleModule("t.rast.list",
        input="precip_abs8")

        #univar_text = """id|semantic_label|start|end|mean|min|max|mean_of_abs|stddev|variance|coeff_var|sum|null_cells|cells|non_null_cells
#a_1@testing||2001-01-01 00:00:00|2001-04-01 00:00:00|100|100|100|100|0|0|0|960000|0|9600|9600
#a_2@testing||2001-04-01 00:00:00|2001-07-01 00:00:00|200|200|200|200|0|0|0|1920000|0|9600|9600
#a_3@testing||2001-07-01 00:00:00|2001-10-01 00:00:00|300|300|300|300|0|0|0|2880000|0|9600|9600
#a_4@testing||2001-10-01 00:00:00|2002-01-01 00:00:00|400|400|400|400|0|0|0|3840000|0|9600|9600
#"""
        #for ref, res in zip(
            #univar_text.split("\n"), t_rast_univar.outputs.stdout.split("\n")
        #):
            #if ref and res:
                #ref_line = ref.split("|", 1)[1]
                #res_line = res.split("|", 1)[1]
                #self.assertLooksLike(ref_line, res_line)

    #def test_with_subset_of_maps(self):

        #t_rast_univar = SimpleModule(
            #"t.rast.univar",
            #input="A",
            #where="start_time >= '2001-03-01'",
            #overwrite=True,
            #verbose=True,
        #)
        #self.runModule("g.region", res=1)
        #self.assertModule(t_rast_univar)

        #univar_text = """id|semantic_label|start|end|mean|min|max|mean_of_abs|stddev|variance|coeff_var|sum|null_cells|cells|non_null_cells
#a_2@testing||2001-04-01 00:00:00|2001-07-01 00:00:00|200|200|200|200|0|0|0|1920000|0|9600|9600
#a_3@testing||2001-07-01 00:00:00|2001-10-01 00:00:00|300|300|300|300|0|0|0|2880000|0|9600|9600
#a_4@testing||2001-10-01 00:00:00|2002-01-01 00:00:00|400|400|400|400|0|0|0|3840000|0|9600|9600
#"""
        #for ref, res in zip(
            #univar_text.split("\n"), t_rast_univar.outputs.stdout.split("\n")
        #):
            #if ref and res:
                #ref_line = ref.split("|", 1)[1]
                #res_line = res.split("|", 1)[1]
                #self.assertLooksLike(ref_line, res_line)

    #def test_coarser_resolution(self):

        #t_rast_univar = SimpleModule(
            #"t.rast.univar",
            #input="A",
            #where="start_time >= '2001-03-01'",
            #overwrite=True,
            #verbose=True,
        #)
        #self.runModule("g.region", res=10)
        #self.assertModule(t_rast_univar)

        #univar_text = """id|semantic_label|start|end|mean|min|max|mean_of_abs|stddev|variance|coeff_var|sum|null_cells|cells|non_null_cells
#a_2@testing||2001-04-01 00:00:00|2001-07-01 00:00:00|200|200|200|200|0|0|0|19200|0|96|96
#a_3@testing||2001-07-01 00:00:00|2001-10-01 00:00:00|300|300|300|300|0|0|0|28800|0|96|96
#a_4@testing||2001-10-01 00:00:00|2002-01-01 00:00:00|400|400|400|400|0|0|0|38400|0|96|96
#"""
        #for ref, res in zip(
            #univar_text.split("\n"), t_rast_univar.outputs.stdout.split("\n")
        #):
            #if ref and res:
                #ref_line = ref.split("|", 1)[1]
                #res_line = res.split("|", 1)[1]
                #self.assertLooksLike(ref_line, res_line)

    #def test_subset_with_output(self):

        #self.runModule("g.region", res=10)
        #self.assertModule(
            #"t.rast.univar",
            #input="A",
            #flags="r",
            #output="univar_output.txt",
            #where="start_time >= '2001-03-01'",
            #overwrite=True,
            #verbose=True,
        #)

        #univar_text = """id|semantic_label|start|end|mean|min|max|mean_of_abs|stddev|variance|coeff_var|sum|null_cells|cells|non_null_cells
#a_2@testing||2001-04-01 00:00:00|2001-07-01 00:00:00|200|200|200|200|0|0|0|1920000|0|9600|9600
#a_3@testing||2001-07-01 00:00:00|2001-10-01 00:00:00|300|300|300|300|0|0|0|2880000|0|9600|9600
#a_4@testing||2001-10-01 00:00:00|2002-01-01 00:00:00|400|400|400|400|0|0|0|3840000|0|9600|9600
#"""
        #univar_output = open("univar_output.txt", "r").read()

        #for ref, res in zip(univar_text.split("\n"), univar_output.split("\n")):
            #if ref and res:
                #ref_line = ref.split("|", 1)[1]
                #res_line = res.split("|", 1)[1]
                #print(type(ref_line))
                #print(type(res_line))
                #self.assertLooksLike(ref_line, res_line)

    #def test_subset_with_output_coarse_resolution(self):

        #self.runModule("g.region", res=10)
        #self.assertModule(
            #"t.rast.univar",
            #input="A",
            #flags="ru",
            #output="univar_output.txt",
            #where="start_time >= '2001-03-01'",
            #overwrite=True,
            #verbose=True,
        #)

        #univar_text = """a_2@testing||2001-04-01 00:00:00|2001-07-01 00:00:00|200|200|200|200|0|0|0|1920000|0|9600|9600
#a_3@testing||2001-07-01 00:00:00|2001-10-01 00:00:00|300|300|300|300|0|0|0|2880000|0|9600|9600
#a_4@testing||2001-10-01 00:00:00|2002-01-01 00:00:00|400|400|400|400|0|0|0|3840000|0|9600|9600
#"""
        #univar_output = open("univar_output.txt", "r").read()

        #for ref, res in zip(univar_text.split("\n"), univar_output.split("\n")):
            #if ref and res:
                #ref_line = ref.split("|", 1)[1]
                #res_line = res.split("|", 1)[1]
                #self.assertLooksLike(ref_line, res_line)

    #def test_error_handling_empty_strds(self):
        ## Empty strds
        #self.assertModuleFail(
            #"t.rast.univar",
            #input="A",
            #output="univar_output.txt",
            #where="start_time >= '2015-03-01'",
            #overwrite=True,
            #verbose=True,
        #)

    #def test_error_handling_no_input(self):
        ## No input
        #self.assertModuleFail("t.rast.univar", output="out.txt")

    #def test_with_zones(self):
        #"""Test use of zones"""

        #t_rast_univar = SimpleModule(
            #"t.rast.univar",
            #input="A",
            #where="start_time >= '2001-01-01'",
            #zones="zones",
            #overwrite=True,
            #verbose=True,
        #)
        #self.runModule("g.region", res=1)
        #self.assertModule(t_rast_univar)

        #print(t_rast_univar.outputs.stdout.split("\n"))

        #univar_text = """id|semantic_label|start|end|zone|mean|min|max|mean_of_abs|stddev|variance|coeff_var|sum|null_cells|cells|non_null_cells
#a_1@PERMANENT||2001-01-01 00:00:00|2001-04-01 00:00:00|1|100|100|100|100|0|0|0|60000|0|600|600
#a_1@PERMANENT||2001-01-01 00:00:00|2001-04-01 00:00:00|2|100|100|100|100|0|0|0|168000|0|1680|1680
#a_1@PERMANENT||2001-01-01 00:00:00|2001-04-01 00:00:00|3|100|100|100|100|0|0|0|732000|0|7320|7320
#a_2@PERMANENT||2001-04-01 00:00:00|2001-07-01 00:00:00|1|200|200|200|200|0|0|0|120000|0|600|600
#a_2@PERMANENT||2001-04-01 00:00:00|2001-07-01 00:00:00|2|200|200|200|200|0|0|0|336000|0|1680|1680
#a_2@PERMANENT||2001-04-01 00:00:00|2001-07-01 00:00:00|3|200|200|200|200|0|0|0|1464000|0|7320|7320
#a_3@PERMANENT||2001-07-01 00:00:00|2001-10-01 00:00:00|1|300|300|300|300|0|0|0|180000|0|600|600
#a_3@PERMANENT||2001-07-01 00:00:00|2001-10-01 00:00:00|2|300|300|300|300|0|0|0|504000|0|1680|1680
#a_3@PERMANENT||2001-07-01 00:00:00|2001-10-01 00:00:00|3|300|300|300|300|0|0|0|2196000|0|7320|7320
#a_4@PERMANENT||2001-10-01 00:00:00|2002-01-01 00:00:00|1|400|400|400|400|0|0|0|240000|0|600|600
#a_4@PERMANENT||2001-10-01 00:00:00|2002-01-01 00:00:00|2|400|400|400|400|0|0|0|672000|0|1680|1680
#a_4@PERMANENT||2001-10-01 00:00:00|2002-01-01 00:00:00|3|400|400|400|400|0|0|0|2928000|0|7320|7320
#"""

        #for ref, res in zip(
            #univar_text.split("\n"), t_rast_univar.outputs.stdout.split("\n")
        #):
            #if ref and res:
                #ref_line = ref.split("|", 1)[1]
                #res_line = res.split("|", 1)[1]
                #self.assertLooksLike(ref_line, res_line)

    #def test_with_semantic_label(self):
        #"""Test semantic labels"""
        #t_rast_univar = SimpleModule(
            #"t.rast.univar",
            #input="B.S2_B1",
            #where="start_time >= '2001-01-01'",
            #overwrite=True,
            #verbose=True,
        #)
        #self.runModule("g.region", res=1)
        #self.assertModule(t_rast_univar)

        #univar_text = """id|semantic_label|start|end|mean|min|max|mean_of_abs|stddev|variance|coeff_var|sum|null_cells|cells|non_null_cells
#b_1@PERMANENT|S2_B1|2001-01-01 00:00:00|2001-04-01 00:00:00|110|110|110|110|0|0|0|1056000|0|9600|9600
#b_2@PERMANENT|S2_B1|2001-04-01 00:00:00|2001-07-01 00:00:00|220|220|220|220|0|0|0|2112000|0|9600|9600
#b_3@PERMANENT|S2_B1|2001-07-01 00:00:00|2001-10-01 00:00:00|330|330|330|330|0|0|0|3168000|0|9600|9600
#b_4@PERMANENT|S2_B1|2001-10-01 00:00:00|2002-01-01 00:00:00|440|440|440|440|0|0|0|4224000|0|9600|9600
#"""
        #for ref, res in zip(
            #univar_text.split("\n"), t_rast_univar.outputs.stdout.split("\n")
        #):
            #if ref and res:
                #ref_line = ref.split("|", 1)[1]
                #res_line = res.split("|", 1)[1]
                #self.assertLooksLike(ref_line, res_line)

    #def test_with_semantic_label_parallel(self):
        #"""Test semantic labels"""
        #t_rast_univar = SimpleModule(
            #"t.rast.univar",
            #input="B.S2_B1",
            #where="start_time >= '2001-01-01'",
            #nprocs=2,
            #overwrite=True,
            #verbose=True,
        #)
        #self.runModule("g.region", res=1)
        #self.assertModule(t_rast_univar)

        #univar_text = """id|semantic_label|start|end|mean|min|max|mean_of_abs|stddev|variance|coeff_var|sum|null_cells|cells|non_null_cells
#b_1@PERMANENT|S2_B1|2001-01-01 00:00:00|2001-04-01 00:00:00|110|110|110|110|0|0|0|1056000|0|9600|9600
#b_2@PERMANENT|S2_B1|2001-04-01 00:00:00|2001-07-01 00:00:00|220|220|220|220|0|0|0|2112000|0|9600|9600
#b_3@PERMANENT|S2_B1|2001-07-01 00:00:00|2001-10-01 00:00:00|330|330|330|330|0|0|0|3168000|0|9600|9600
#b_4@PERMANENT|S2_B1|2001-10-01 00:00:00|2002-01-01 00:00:00|440|440|440|440|0|0|0|4224000|0|9600|9600
#"""
        #for ref, res in zip(
            #univar_text.split("\n"), t_rast_univar.outputs.stdout.split("\n")
        #):
            #if ref and res:
                #ref_line = ref.split("|", 1)[1]
                #res_line = res.split("|", 1)[1]
                #self.assertLooksLike(ref_line, res_line)


if __name__ == "__main__":
    from grass.gunittest.main import test

    test()
