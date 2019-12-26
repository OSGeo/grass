"""
Name:      vdbselect_test
Purpose:   v.db.select decimation test

Author:    Luca Delucchi
Copyright: (C) 2015 by Luca Delucchi and the GRASS Development Team
Licence:   This program is free software under the GNU General Public
           License (>=v2). Read the file COPYING that comes with GRASS
           for details.
"""

import os
from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule

out_group = u"""CZab
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

out_where = u"""1076|366545504|324050.96875|1077|1076|Zwe|366545512.376|324050.97237
1123|1288.555298|254.393951|1124|1123|Zwe|1288.546525|254.393964
1290|63600420|109186.835938|1291|1290|Zwe|63600422.4739|109186.832069
"""

out_sep = u"""1076,366545504,324050.96875,1077,1076,Zwe,366545512.376,324050.97237
1123,1288.555298,254.393951,1124,1123,Zwe,1288.546525,254.393964
1290,63600420,109186.835938,1291,1290,Zwe,63600422.4739,109186.832069
"""


class SelectTest(TestCase):
    """Test case for v.db.select"""

    outfile = 'select.csv'
    invect = 'geology'
    col = 'GEO_NAME'
    val = 'Zwe'

    def testRun(self):
        """Basic test of v.db.select"""
        self.assertModule('v.db.select', map=self.invect)

    def testFileExists(self):
        """This function checks if the output file is written correctly"""
        self.runModule('v.db.select', map=self.invect, file=self.outfile)
        self.assertFileExists(self.outfile)
        if os.path.isfile(self.outfile):
            os.remove(self.outfile)

    def testGroup(self):
        """Testing v.db.select with group option"""
        sel = SimpleModule('v.db.select', flags='c', map=self.invect,
                           columns=self.col, group=self.col)
        sel.run()
        self.assertLooksLike(reference=out_group, actual=sel.outputs.stdout)

    def testWhere(self):
        """Testing v.db.select with where option"""
        sel = SimpleModule('v.db.select', flags='c', map=self.invect,
                           where="{col}='{val}'".format(col=self.col,
                                                        val=self.val))
        sel.run()
        self.assertLooksLike(reference=out_where, actual=sel.outputs.stdout)

    def testSeparator(self):
        sel = SimpleModule('v.db.select', flags='c', map=self.invect,
                           where="{col}='{val}'".format(col=self.col,
                                                        val=self.val),
                           separator='comma')
        sel.run()
        self.assertLooksLike(reference=out_sep, actual=sel.outputs.stdout)

    def testComma(self):
        sel = SimpleModule('v.db.select', flags='c', map=self.invect,
                           where="{col}='{val}'".format(col=self.col,
                                                        val=self.val),
                           separator=',')
        sel.run()
        self.assertLooksLike(reference=out_sep, actual=sel.outputs.stdout)

if __name__ == '__main__':
    test()
