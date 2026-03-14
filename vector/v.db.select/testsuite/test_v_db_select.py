"""
Name:      vdbselect_test
Purpose:   v.db.select decimation test

Author:    Luca Delucchi
Copyright: (C) 2015 by Luca Delucchi and the GRASS Development Team
Licence:   This program is free software under the GNU General Public
           License (>=v2). Read the file COPYING that comes with GRASS
           for details.
"""

from pathlib import Path
from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule

out_group = """CZab
CZam
CZba
CZbb
CZbf
CZbg
CZbl
CZc
CZfg
CZfv
CZfv1
CZfv2
CZg
CZgms
CZig
CZiv
CZlg
CZma
CZma1
CZma2
CZmd
CZmd1
CZmd2
CZmd3
CZmg
CZms
CZmv
CZmv1
CZpg
CZph
CZq
CZtp
CZv
CZve
CZy
Ccl
Ccu
Chg
Cr
Cs
DOg
DOgb
DSc
DSg
DSs
Dqd
Dsc
Jd
Kb
Kc
Km
Kp
Mc
OCg
OCgm
Org
PPg
PPgb
PPmg
PzZg
PzZm
PzZq
PzZu
Qp
SOgg
Sg
TRc
TRcc
TRcp
TRcs
TRd
TRdc
TRdp
TRds
Tec
Tecs
Tob
Tor
Tp
Tpa
Tpy
Tpyw
Tt
Ybam
Ybgg
Ybrg
Yg
Ygg
Ymam
Ymg
Ytg
ZYbA
ZYba
ZYbn
Za
Zaba
Zabg
Zabs
Zata
Zatb
Zatm
Zats
Zatw
Zb
Zbg
Zbt
Zch
Zchs
Zco
Zd
Zf
Zg
Zgma
Zgmf
Zgmg
Zgms
Zgmu
Zgmw
Zgs
Zhha
Zlm
Zm
Zman
Zmb
Zmf
Zml
Znt
Zrb
Zs
Zsl
Zsp
Zsr
Zss
Zsw
Zwc
Zwe
bz
"""

out_where = """1076|366545504|324050.96875|1077|1076|Zwe|366545512.376|324050.97237
1123|1288.555298|254.393951|1124|1123|Zwe|1288.546525|254.393964
1290|63600420|109186.835938|1291|1290|Zwe|63600422.4739|109186.832069
"""

out_extent = """n=201971.859459
s=148158.109538
w=123971.194990
e=209096.266022
"""

out_sep = """1076,366545504,324050.96875,1077,1076,Zwe,366545512.376,324050.97237
1123,1288.555298,254.393951,1124,1123,Zwe,1288.546525,254.393964
1290,63600420,109186.835938,1291,1290,Zwe,63600422.4739,109186.832069
"""

out_json = """\
{"info":
{"columns":[
{"name":"cat","sql_type":"INTEGER","is_number":true},
{"name":"onemap_pro","sql_type":"DOUBLE PRECISION","is_number":true},
{"name":"PERIMETER","sql_type":"DOUBLE PRECISION","is_number":true},
{"name":"GEOL250_","sql_type":"INTEGER","is_number":true},
{"name":"GEOL250_ID","sql_type":"INTEGER","is_number":true},
{"name":"GEO_NAME","sql_type":"CHARACTER","is_number":false},
{"name":"SHAPE_area","sql_type":"DOUBLE PRECISION","is_number":true},
{"name":"SHAPE_len","sql_type":"DOUBLE PRECISION","is_number":true}
]},
"records":[
{"cat":1,"onemap_pro":963738.75,"PERIMETER":4083.97998,"GEOL250_":2,"GEOL250_ID":1,"GEO_NAME":"Zml","SHAPE_area":963738.608571,"SHAPE_len":4083.979839},
{"cat":2,"onemap_pro":22189124,"PERIMETER":26628.261719,"GEOL250_":3,"GEOL250_ID":2,"GEO_NAME":"Zmf","SHAPE_area":22189123.2296,"SHAPE_len":26628.261112},
{"cat":3,"onemap_pro":579286.875,"PERIMETER":3335.55835,"GEOL250_":4,"GEOL250_ID":3,"GEO_NAME":"Zml","SHAPE_area":579286.829631,"SHAPE_len":3335.557182},
{"cat":4,"onemap_pro":20225526,"PERIMETER":33253,"GEOL250_":5,"GEOL250_ID":4,"GEO_NAME":"Zml","SHAPE_area":20225526.6368,"SHAPE_len":33253.000508},
{"cat":5,"onemap_pro":450650720,"PERIMETER":181803.765625,"GEOL250_":6,"GEOL250_ID":5,"GEO_NAME":"Ybgg","SHAPE_area":450650731.029,"SHAPE_len":181803.776199},
{"cat":6,"onemap_pro":6386874.5,"PERIMETER":17978.429688,"GEOL250_":7,"GEOL250_ID":6,"GEO_NAME":"Zml","SHAPE_area":6386875.00182,"SHAPE_len":17978.430892},
{"cat":7,"onemap_pro":969311.1875,"PERIMETER":4096.096191,"GEOL250_":8,"GEOL250_ID":7,"GEO_NAME":"Ybgg","SHAPE_area":969311.720138,"SHAPE_len":4096.098584},
{"cat":8,"onemap_pro":537840.625,"PERIMETER":3213.40625,"GEOL250_":9,"GEOL250_ID":8,"GEO_NAME":"Zmf","SHAPE_area":537840.953797,"SHAPE_len":3213.407197},
{"cat":9,"onemap_pro":3389078.25,"PERIMETER":16346.604492,"GEOL250_":10,"GEOL250_ID":9,"GEO_NAME":"Zml","SHAPE_area":3389077.17888,"SHAPE_len":16346.604884},
{"cat":10,"onemap_pro":906132.375,"PERIMETER":4319.162109,"GEOL250_":11,"GEOL250_ID":10,"GEO_NAME":"Zml","SHAPE_area":906132.945012,"SHAPE_len":4319.163379}
]}"""


class SelectTest(TestCase):
    """Test case for v.db.select"""

    outfile = "select.csv"
    invect = "geology"
    col = "GEO_NAME"
    val = "Zwe"
    cat = 10

    def testRun(self):
        """Module runs with minimal parameters and give output."""
        sel = SimpleModule(
            "v.db.select",
            map=self.invect,
        )
        sel.run()
        self.assertTrue(sel.outputs.stdout, msg="There should be some output")

    def testFileExists(self):
        """This function checks if the output file is written correctly"""
        self.runModule("v.db.select", map=self.invect, file=self.outfile)
        self.assertFileExists(self.outfile)
        Path(self.outfile).unlink(missing_ok=True)

    def testGroup(self):
        """Testing v.db.select with group option"""
        sel = SimpleModule(
            "v.db.select", flags="c", map=self.invect, columns=self.col, group=self.col
        )
        sel.run()
        self.assertLooksLike(reference=out_group, actual=sel.outputs.stdout)

    def testWhere(self):
        """Testing v.db.select with where option"""
        sel = SimpleModule(
            "v.db.select",
            flags="c",
            map=self.invect,
            where="{col}='{val}'".format(col=self.col, val=self.val),
        )
        sel.run()
        self.assertLooksLike(reference=out_where, actual=sel.outputs.stdout)

    def testExtent(self):
        """Testing v.db.select extent output with -r flag"""
        sel = SimpleModule(
            "v.db.select",
            flags="r",
            map=self.invect,
            where="{col}='{val}'".format(col=self.col, val=self.val),
        )
        sel.run()
        self.assertLooksLike(reference=out_extent, actual=sel.outputs.stdout)

    def testSeparator(self):
        sel = SimpleModule(
            "v.db.select",
            flags="c",
            map=self.invect,
            where="{col}='{val}'".format(col=self.col, val=self.val),
            separator="comma",
        )
        sel.run()
        self.assertLooksLike(reference=out_sep, actual=sel.outputs.stdout)

    def testComma(self):
        sel = SimpleModule(
            "v.db.select",
            flags="c",
            map=self.invect,
            where="{col}='{val}'".format(col=self.col, val=self.val),
            separator=",",
        )
        sel.run()
        self.assertLooksLike(reference=out_sep, actual=sel.outputs.stdout)

    def testJSON(self):
        """Test that JSON can be decoded and formatted exactly as expected"""
        import json

        sel = SimpleModule(
            "v.db.select",
            format="json",
            map=self.invect,
            where="cat<={cat}".format(cat=self.cat),
        )
        sel.run()

        try:
            json.loads(sel.outputs.stdout)
        except ValueError:
            self.fail(msg="No JSON object could be decoded:\n" + sel.outputs.stdout)

        self.assertEqual(json.loads(out_json), json.loads(sel.outputs.stdout))


if __name__ == "__main__":
    test()
