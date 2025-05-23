"""
Name:       r.texture test
Purpose:    Tests r.texture and its flags/options.

Author:     Sunveer Singh, Google Code-in 2017
            Chung-Yuan Liang, modified in 2024
Copyright:  (C) 2017 by Sunveer Singh and the GRASS Development Team
Licence:    This program is free software under the GNU General Public
            License (>=v2). Read the file COPYING that comes with GRASS
            for details.
"""

from grass.gunittest.case import TestCase
import sys

IS_MAC = sys.platform.startswith("darwin")


class TestRasterreport(TestCase):
    input = "lsat7_2002_80"

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        cls.runModule("g.region", raster=cls.input)

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()
        maps = [
            "asm_ASM",
            "contrast_Contr",
            "corr_Corr",
            "var_Var",
            "idm_IDM",
            "sa_SA",
            "sv_SV",
            "se_SE",
            "entr_Entr",
            "dv_DV",
            "de_DE",
            "moc1_MOC-1",
            "moc2_MOC-2",
        ]
        cls.runModule("g.remove", flags="f", type="raster", name=maps)

    def test_asm(self):
        """Testing method asm"""
        basename = "ASM"
        method = "asm"
        output = f"{method}_{basename}"
        values = """min=0.104167
        max=1
        mean=0.112871783083256
        variance=0.000158101620160753
        n=996244"""
        self.assertModule("r.texture", input=self.input, output=method, method=method)
        self.assertRasterFitsUnivar(output, reference=values, precision=1e-2)

    def test_contrast(self):
        """Testing method contrast"""
        basename = "Contr"
        method = "contrast"
        output = f"{method}_{basename}"
        values = """min=0
        max=14680.771484375
        mean=176.569350873014
        variance=185767.349118713
        n=996244"""
        self.assertModule("r.texture", input=self.input, output=method, method=method)
        self.assertRasterFitsUnivar(output, reference=values, precision=1e-2)

    def test_corr(self):
        """Testing method corr"""
        basename = "Corr"
        method = "corr"
        output = f"{method}_{basename}"
        values = """min=-0.533553719520569
        max=0.443134188652039
        mean=-0.0511981885382368
        variance=0.0306650179549719
        n=996244"""
        self.assertModule("r.texture", input=self.input, output=method, method=method)
        self.assertRasterFitsUnivar(output, reference=values, precision=1e-2)

    def test_var(self):
        """Testing method var"""
        basename = "Var"
        method = "var"
        output = f"{method}_{basename}"
        values = """min=0
        max=8776.787109375
        mean=90.3218668390769
        variance=58439.1071892438
        n=996244"""
        self.assertModule("r.texture", input=self.input, output=method, method=method)
        self.assertRasterFitsUnivar(output, reference=values, precision=1e-2)

    def test_idm(self):
        """Testing method idm"""
        basename = "IDM"
        method = "idm"
        output = f"{method}_{basename}"
        values = """min=0.000490661768708378
        max=1
        mean=0.12308521457551
        variance=0.00497110161251246
        n=996244"""
        self.assertModule("r.texture", input=self.input, output=method, method=method)
        self.assertRasterFitsUnivar(output, reference=values, precision=1e-2)

    def test_sa(self):
        """Testing method sa"""
        basename = "SA"
        method = "sa"
        output = f"{method}_{basename}"
        values = """min=95.9583358764648
        max=2942.79150390625
        mean=742.367934271396
        variance=31616.3527726298
        n=996244"""
        self.assertModule("r.texture", input=self.input, output=method, method=method)
        self.assertRasterFitsUnivar(output, reference=values, precision=1e-2)

    def test_sv(self):
        """Testing method sv"""
        basename = "SV"
        method = "sv"
        output = f"{method}_{basename}"
        values = """min=0
        max=45368492
        mean=2248724.35829364
        variance=2332049431762.5
        n=996244"""
        self.assertModule("r.texture", input=self.input, output=method, method=method)
        self.assertRasterFitsUnivar(output, reference=values, precision=1e-4)

    def test_se(self):
        """Testing method se"""
        basename = "SE"
        method = "se"
        output = f"{method}_{basename}"
        values = """min=0
        max=2.29248142242432
        mean=1.99162619886133
        variance=0.0255830167563798
        n=996244"""
        self.assertModule("r.texture", input=self.input, output=method, method=method)
        self.assertRasterFitsUnivar(output, reference=values, precision=1e-2)

    def test_entr(self):
        """Testing method entr"""
        basename = "Entr"
        method = "entr"
        output = f"{method}_{basename}"
        values = """min=-0
        max=3.29248118400574
        mean=3.20825728104855
        variance=0.0106857128131089
        n=996244"""
        self.assertModule("r.texture", input=self.input, output=method, method=method)
        self.assertRasterFitsUnivar(output, reference=values, precision=1e-2)

    def test_dv(self):
        """Testing method dv"""
        basename = "DV"
        method = "dv"
        output = f"{method}_{basename}"
        values = """min=-4.33281384175643e-05
        max=0.115451395511627
        mean=0.00110390526476111
        variance=2.37568891441793e-06
        n=996244"""
        self.assertModule("r.texture", input=self.input, output=method, method=method)
        self.assertRasterFitsUnivar(output, reference=values, precision=1e-2)

    def test_de(self):
        """Testing method de"""
        basename = "DE"
        method = "de"
        output = f"{method}_{basename}"
        values = """min=-0
        max=2.29248142242432
        mean=1.6115293021953
        variance=0.055056566141371
        n=996244"""
        self.assertModule("r.texture", input=self.input, output=method, method=method)
        self.assertRasterFitsUnivar(output, reference=values, precision=1e-2)

    def test_moc1(self):
        """Testing method moc1"""
        basename = "MOC-1"
        method = "moc1"
        output = f"{method}_{basename}"
        values = """min=-0.910445094108582
        max=0
        mean=-0.782939935071241
        variance=0.00736236364469843
        n=996244"""
        self.assertModule("r.texture", input=self.input, output=method, method=method)
        self.assertRasterFitsUnivar(output, reference=values, precision=1e-2)

    def test_moc2(self):
        """Testing method moc2"""
        basename = "MOC-2"
        method = "moc2"
        output = f"{method}_{basename}"
        values = """min=-0
        max=0.996889352798462
        mean=0.98765725693893
        variance=0.000334346005602857
        n=996244"""
        self.assertModule("r.texture", input=self.input, output=method, method=method)
        self.assertRasterFitsUnivar(output, reference=values, precision=1e-2)


if __name__ == "__main__":
    from grass.gunittest.main import test

    test()
