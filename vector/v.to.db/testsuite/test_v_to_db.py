import json
from itertools import zip_longest

from grass.gunittest.case import TestCase
from grass.gunittest.main import test

from grass.gunittest.gmodules import SimpleModule


class TestVToDb(TestCase):

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()

    def _assert_dict_almost_equal(self, d1, d2):
        self.assertEqual(set(d1.keys()), set(d2.keys()))
        for k1 in d1:
            if isinstance(d1[k1], float):
                self.assertAlmostEqual(d1[k1], d2[k1], places=6)
            else:
                self.assertEqual(d1[k1], d2[k1])

    def _assert_json_equal(self, module, reference, has_totals=True):
        self.runModule(module)
        result = json.loads(module.outputs.stdout)

        self.assertCountEqual(list(reference.keys()), list(result.keys()))
        if "unit" in reference:
            self.assertEqual(reference["unit"], result["unit"])
        if has_totals:
            self._assert_dict_almost_equal(reference["totals"], result["totals"])
        for record1, record2 in zip_longest(reference["records"], result["records"]):
            self._assert_dict_almost_equal(record1, record2)

    def test_json_length(self):
        reference = {
            "unit": "feet",
            "totals": {"length": 34208.19507027471},
            "records": [
                {"category": 1, "length": 14944.03890742192},
                {"category": 2, "length": 19264.156162852792},
            ],
        }
        module = SimpleModule(
            "v.to.db",
            "busroute6",
            flags="p",
            option="length",
            units="feet",
            type="line",
            format="json",
        )
        self._assert_json_equal(module, reference)

    def test_json_coor(self):
        reference = {
            "unit": "",
            "records": [
                {
                    "category": 11,
                    "x": 638150.7920150368,
                    "y": 220024.77915312737,
                    "z": 0,
                },
                {
                    "category": 103,
                    "x": 638287.18720843294,
                    "y": 219698.23416404743,
                    "z": 0,
                },
                {
                    "category": 104,
                    "x": 638278.98502463801,
                    "y": 219611.30807667322,
                    "z": 0,
                },
                {
                    "category": 105,
                    "x": 638306.74931247137,
                    "y": 219887.96339615693,
                    "z": 0,
                },
                {
                    "category": 106,
                    "x": 638269.46915021574,
                    "y": 219523.05001002364,
                    "z": 0,
                },
                {
                    "category": 107,
                    "x": 638232.74371348787,
                    "y": 219624.32918462454,
                    "z": 0,
                },
                {
                    "category": 108,
                    "x": 638230.51562103187,
                    "y": 219627.93192782634,
                    "z": 0,
                },
                {
                    "category": 109,
                    "x": 638262.64465328958,
                    "y": 219722.83464563562,
                    "z": 0,
                },
                {
                    "category": 110,
                    "x": 638260.06604013185,
                    "y": 219726.78790954105,
                    "z": 0,
                },
                {
                    "category": 111,
                    "x": 638495.58851612895,
                    "y": 219729.4579598321,
                    "z": 0,
                },
                {
                    "category": 112,
                    "x": 638332.230669952,
                    "y": 219728.48552823684,
                    "z": 0,
                },
                {
                    "category": 113,
                    "x": 638317.71299009491,
                    "y": 219988.20012615179,
                    "z": 0,
                },
                {
                    "category": 114,
                    "x": 638245.4447548897,
                    "y": 219808.26111248325,
                    "z": 0,
                },
                {
                    "category": 117,
                    "x": 638329.73432644235,
                    "y": 220099.23289130288,
                    "z": 0,
                },
            ],
        }
        module = SimpleModule(
            "v.to.db", "P079218", flags="p", option="coor", type="point", format="json"
        )
        self._assert_json_equal(module, reference, has_totals=False)

    def test_json_count(self):
        reference = {
            "unit": "",
            "totals": {"count": 2},
            "records": [{"category": 1, "count": 1}, {"category": 2, "count": 1}],
        }
        module = SimpleModule(
            "v.to.db",
            "busroute6",
            flags="p",
            option="count",
            type="line",
            format="json",
        )
        self._assert_json_equal(module, reference)

    def test_json_start(self):
        reference = {
            "unit": "",
            "records": [
                {
                    "category": 1,
                    "x": 639023.64826068562,
                    "y": 226249.02352245309,
                    "z": 0,
                },
                {
                    "category": 2,
                    "x": 636004.4867560484,
                    "y": 227252.76225420492,
                    "z": 0,
                },
            ],
        }
        module = SimpleModule(
            "v.to.db",
            "busroute6",
            flags="p",
            option="start",
            type="line",
            format="json",
        )
        self._assert_json_equal(module, reference, has_totals=False)

    def test_json_end(self):
        reference = {
            "unit": "",
            "records": [
                {
                    "category": 1,
                    "x": 636004.4867560484,
                    "y": 227252.76225420492,
                    "z": 0,
                },
                {
                    "category": 2,
                    "x": 639023.64826068562,
                    "y": 226249.02352245309,
                    "z": 0,
                },
            ],
        }
        module = SimpleModule(
            "v.to.db",
            "busroute6",
            flags="p",
            option="end",
            type="line",
            format="json",
        )
        self._assert_json_equal(module, reference, has_totals=False)

    def test_json_perimeter(self):
        reference = {
            "unit": "meters",
            "records": [
                {"category": 1, "perimeter": 24598.86577244935},
                {"category": 2, "perimeter": 9468.4353925819196},
                {"category": 3, "perimeter": 35128.971279485551},
                {"category": 4, "perimeter": 11212.132236305841},
                {"category": 5, "perimeter": 9321.3918036502655},
                {"category": 6, "perimeter": 44835.796873804647},
                {"category": 7, "perimeter": 39073.911697927579},
                {"category": 8, "perimeter": 76857.730674167731},
                {"category": 9, "perimeter": 24498.427412088789},
                {"category": 10, "perimeter": 47137.575714678598},
                {"category": 11, "perimeter": 63352.593123459956},
                {"category": 12, "perimeter": 14528.794443747753},
                {"category": 13, "perimeter": 79233.504615285914},
                {"category": 14, "perimeter": 7804.7605523144985},
                {"category": 15, "perimeter": 10088.919010038735},
                {"category": 16, "perimeter": 42185.993420625222},
                {"category": 17, "perimeter": 59629.69189995146},
                {"category": 18, "perimeter": 38392.188493489128},
                {"category": 19, "perimeter": 16140.878937455964},
                {"category": 20, "perimeter": 6131.8264748468282},
                {"category": 21, "perimeter": 5722.2097281652468},
                {"category": 22, "perimeter": 981.84859206546867},
                {"category": 23, "perimeter": 3278.9333706111693},
                {"category": 24, "perimeter": 41171.96861221869},
                {"category": 25, "perimeter": 18430.430498643291},
                {"category": 26, "perimeter": 58923.003554658462},
                {"category": 27, "perimeter": 29549.526342969653},
                {"category": 28, "perimeter": 69802.55947781021},
                {"category": 29, "perimeter": 36058.290848741555},
                {"category": 30, "perimeter": 38449.518135314524},
                {"category": 31, "perimeter": 6250.7631358466524},
                {"category": 32, "perimeter": 55801.999859604068},
                {"category": 33, "perimeter": 70715.622869054729},
                {"category": 34, "perimeter": 76759.741107759342},
                {"category": 35, "perimeter": 26910.754068406088},
                {"category": 36, "perimeter": 39340.409485920383},
                {"category": 37, "perimeter": 145410.62931138012},
                {"category": 38, "perimeter": 50060.653495888575},
                {"category": 39, "perimeter": 55567.607184888642},
                {"category": 40, "perimeter": 87018.525604234936},
                {"category": 41, "perimeter": 51467.454846489643},
                {"category": 42, "perimeter": 64775.249413445606},
                {"category": 43, "perimeter": 82434.409033744654},
                {"category": 44, "perimeter": 5761.9738735064611},
            ],
        }
        module = SimpleModule(
            "v.to.db",
            "zipcodes",
            flags="p",
            option="perimeter",
            type="boundary",
            format="json",
        )
        self._assert_json_equal(module, reference, has_totals=False)

    def test_json_area(self):
        reference = {
            "unit": "square meters",
            "totals": {"area": 2219442027.2203522},
            "records": [
                {"category": 1, "area": 24375323.127803534},
                {"category": 2, "area": 2938964.3204806102},
                {"category": 3, "area": 50536154.294107705},
                {"category": 4, "area": 5456815.4884436112},
                {"category": 5, "area": 1169066.1060796678},
                {"category": 6, "area": 55971618.79167185},
                {"category": 7, "area": 37359908.347983636},
                {"category": 8, "area": 122585299.61230192},
                {"category": 9, "area": 29156100.849060502},
                {"category": 10, "area": 58725489.008975707},
                {"category": 11, "area": 79361312.750253946},
                {"category": 12, "area": 8056494.6019629827},
                {"category": 13, "area": 110910429.39899969},
                {"category": 14, "area": 2368649.5703158271},
                {"category": 15, "area": 4548159.3697960936},
                {"category": 16, "area": 62407571.75144887},
                {"category": 17, "area": 101909096.69766694},
                {"category": 18, "area": 72939658.228119552},
                {"category": 19, "area": 6928400.7578192391},
                {"category": 20, "area": 1301839.0699078622},
                {"category": 21, "area": 1005940.9336275081},
                {"category": 22, "area": 55213.11751843011},
                {"category": 23, "area": 344611.97188792191},
                {"category": 24, "area": 56679125.811831549},
                {"category": 25, "area": 11981993.163624534},
                {"category": 26, "area": 76289561.70373407},
                {"category": 27, "area": 22538181.744323522},
                {"category": 28, "area": 129050077.06118551},
                {"category": 29, "area": 25640780.620766483},
                {"category": 30, "area": 30231334.130349141},
                {"category": 31, "area": 1559389.8359854589},
                {"category": 32, "area": 61973376.312294327},
                {"category": 33, "area": 84938883.430163413},
                {"category": 34, "area": 82929693.762826234},
                {"category": 35, "area": 30243435.083937138},
                {"category": 36, "area": 52403853.53241165},
                {"category": 37, "area": 260756049.76080063},
                {"category": 38, "area": 47323521.112179838},
                {"category": 39, "area": 51313713.890918314},
                {"category": 40, "area": 130875884.22338714},
                {"category": 41, "area": 59798016.861835025},
                {"category": 42, "area": 63169356.527421139},
                {"category": 43, "area": 98606087.58183229},
                {"category": 44, "area": 727592.90231111227},
            ],
        }
        module = SimpleModule(
            "v.to.db",
            "zipcodes",
            flags="p",
            option="area",
            type="centroid",
            columns="area_size",
            format="json",
        )
        self._assert_json_equal(module, reference, has_totals=False)

    def test_json_fd(self):
        reference = {
            "unit": "",
            "records": [
                {"category": 1, "fd": 1.1888302630715635},
                {"category": 2, "fd": 1.2294863225467458},
                {"category": 3, "fd": 1.1801402305121538},
                {"category": 4, "fd": 1.2022338481544725},
                {"category": 5, "fd": 1.3083671855769567},
                {"category": 6, "fd": 1.2007341724788474},
                {"category": 7, "fd": 1.2127947209115231},
                {"category": 8, "fd": 1.208066951839512},
                {"category": 9, "fd": 1.1759671238903131},
                {"category": 10, "fd": 1.2031076141730892},
                {"category": 11, "fd": 1.2156967493833235},
                {"category": 12, "fd": 1.2053696828117564},
                {"category": 13, "fd": 1.2178808404455326},
                {"category": 14, "fd": 1.2212280450720407},
                {"category": 15, "fd": 1.202746607648131},
                {"category": 16, "fd": 1.1866651678844309},
                {"category": 17, "fd": 1.1926412531635737},
                {"category": 18, "fd": 1.1660343427950151},
                {"category": 19, "fd": 1.2302742099545927},
                {"category": 20, "fd": 1.23887623246283},
                {"category": 21, "fd": 1.2519844727873344},
                {"category": 22, "fd": 1.2619223328747082},
                {"category": 23, "fd": 1.2698294530368857},
                {"category": 24, "fd": 1.1903391696095686},
                {"category": 25, "fd": 1.2052039170704054},
                {"category": 26, "fd": 1.2103535642862764},
                {"category": 27, "fd": 1.2159934303235393},
                {"category": 28, "fd": 1.1944311955007796},
                {"category": 29, "fd": 1.2301383257693426},
                {"category": 30, "fd": 1.2258317130564238},
                {"category": 31, "fd": 1.2258875533887401},
                {"category": 32, "fd": 1.2183071846625073},
                {"category": 33, "fd": 1.2232186165966428},
                {"category": 34, "fd": 1.2338205246210083},
                {"category": 35, "fd": 1.1843722268548442},
                {"category": 36, "fd": 1.1904709605382684},
                {"category": 37, "fd": 1.2268185301868757},
                {"category": 38, "fd": 1.2246122047934189},
                {"category": 39, "fd": 1.2307854820899107},
                {"category": 40, "fd": 1.2171238499654589},
                {"category": 41, "fd": 1.2117069453026579},
                {"category": 42, "fd": 1.2336145410258579},
                {"category": 43, "fd": 1.2299644069836664},
                {"category": 44, "fd": 1.2830579477351167},
            ],
        }
        module = SimpleModule(
            "v.to.db",
            "zipcodes",
            flags="p",
            option="fd",
            type="boundary",
            format="json",
        )
        self._assert_json_equal(module, reference, has_totals=False)

    def test_json_compact(self):
        reference = {
            "unit": "",
            "records": [
                {"category": 1, "compact": 3.6753703182945565},
                {"category": 2, "compact": 4.5652125910941628},
                {"category": 3, "compact": 4.1330860302272887},
                {"category": 4, "compact": 2.5515723117659221},
                {"category": 5, "compact": 3.4458068216072211},
                {"category": 6, "compact": 5.4223285951819333},
                {"category": 7, "compact": 2.474707367209521},
                {"category": 8, "compact": 1.7635412847652003},
                {"category": 9, "compact": 7.0107505994404562},
                {"category": 10, "compact": 4.8245329469298692},
                {"category": 11, "compact": 4.7932804440061689},
                {"category": 12, "compact": 4.1307727891921457},
                {"category": 13, "compact": 4.4839421485340925},
                {"category": 14, "compact": 3.0632422687208885},
                {"category": 15, "compact": 3.0090415115087539},
                {"category": 16, "compact": 2.6078537621385931},
                {"category": 17, "compact": 3.2889772616297641},
                {"category": 18, "compact": 2.417965657258295},
                {"category": 19, "compact": 4.0134126218532575},
                {"category": 20, "compact": 2.7796223999073204},
                {"category": 21, "compact": 2.79900443609572},
                {"category": 22, "compact": 2.2049195457558515},
                {"category": 23, "compact": 2.1068305760253234},
                {"category": 24, "compact": 2.2724659946920722},
                {"category": 25, "compact": 2.6002108310185501},
                {"category": 26, "compact": 3.1788654870378794},
                {"category": 27, "compact": 2.4181268737465977},
                {"category": 28, "compact": 2.3965831645238533},
                {"category": 29, "compact": 2.0780129123491937},
                {"category": 30, "compact": 4.8630796359296653},
                {"category": 31, "compact": 2.9823392332600531},
                {"category": 32, "compact": 2.3300076181728975},
                {"category": 33, "compact": 5.1000028977609313},
                {"category": 34, "compact": 5.920502404361156},
                {"category": 35, "compact": 5.5488403740189547},
                {"category": 36, "compact": 3.3907180136934643},
                {"category": 37, "compact": 2.5313870617681546},
                {"category": 38, "compact": 4.73821424080902},
                {"category": 39, "compact": 5.4916836681106842},
                {"category": 40, "compact": 2.2745474591353347},
                {"category": 41, "compact": 2.9809652471754351},
                {"category": 42, "compact": 4.6962590533765969},
                {"category": 43, "compact": 2.1245231830346061},
                {"category": 44, "compact": 3.056012063621615},
                {"category": 45, "compact": 3.3291343464190493},
                {"category": 46, "compact": 5.1933032154109879},
                {"category": 47, "compact": 4.1796131665762353},
                {"category": 48, "compact": 5.2623139999057207},
                {"category": 49, "compact": 1.9600793652891744},
                {"category": 50, "compact": 2.5171374284117043},
                {"category": 51, "compact": 6.2768503000458189},
                {"category": 52, "compact": 4.2431120825068325},
                {"category": 53, "compact": 3.9101032574787369},
                {"category": 54, "compact": 8.9677872869774795},
                {"category": 55, "compact": 6.9027292967097438},
                {"category": 56, "compact": 4.3365111677624961},
                {"category": 57, "compact": 4.0262796900466329},
                {"category": 58, "compact": 7.2958538755198319},
                {"category": 59, "compact": 5.5278721435645668},
                {"category": 60, "compact": 2.5869340127782872},
                {"category": 61, "compact": 6.0408939089999301},
                {"category": 62, "compact": 4.8365780371494163},
                {"category": 63, "compact": 3.917141398042034},
                {"category": 64, "compact": 4.1301496064039123},
                {"category": 65, "compact": 2.8273276432355363},
                {"category": 66, "compact": 2.5923462660986591},
                {"category": 67, "compact": 4.4259345650251367},
                {"category": 68, "compact": 3.2137515594108756},
                {"category": 69, "compact": 5.1014676630159501},
                {"category": 70, "compact": 3.4286661194216475},
                {"category": 71, "compact": 5.2859091659363777},
                {"category": 72, "compact": 3.6359093682608199},
                {"category": 73, "compact": 5.4962989484868796},
                {"category": 74, "compact": 11.452610638247759},
                {"category": 75, "compact": 5.0088687194692012},
                {"category": 76, "compact": 4.0467321897356303},
                {"category": 77, "compact": 4.0089480465834093},
                {"category": 78, "compact": 2.3520946727425192},
                {"category": 79, "compact": 2.6407754739017859},
                {"category": 80, "compact": 2.0934143925538549},
                {"category": 81, "compact": 4.4526475376702024},
                {"category": 82, "compact": 4.6351273439475236},
                {"category": 83, "compact": 1.7158259909427274},
                {"category": 84, "compact": 4.4541759550429267},
                {"category": 85, "compact": 3.5790482554384973},
                {"category": 86, "compact": 5.7877762918161766},
                {"category": 87, "compact": 2.371047292564501},
                {"category": 88, "compact": 7.9942310159547709},
                {"category": 89, "compact": 2.3153381229207834},
                {"category": 90, "compact": 3.6342338670560279},
                {"category": 91, "compact": 2.6648488853708723},
                {"category": 92, "compact": 3.2259779985800883},
                {"category": 93, "compact": 4.8501132588513514},
                {"category": 94, "compact": 4.1743562627031441},
                {"category": 95, "compact": 3.2024459554511591},
                {"category": 96, "compact": 2.3924837573909778},
                {"category": 97, "compact": 3.8614158957079829},
                {"category": 98, "compact": 3.7644090019690868},
                {"category": 99, "compact": 2.7647815274299017},
                {"category": 100, "compact": 1.6069454031200812},
                {"category": 101, "compact": 2.745207032999478},
                {"category": 102, "compact": 3.9012908531821631},
                {"category": 103, "compact": 1.6961214333217043},
                {"category": 104, "compact": 2.9654999593207316},
                {"category": 105, "compact": 5.1303074756554601},
                {"category": 106, "compact": 4.1971143513731475},
                {"category": 107, "compact": 3.0189983714399524},
                {"category": 108, "compact": 4.5821868837403565},
            ],
        }
        module = SimpleModule(
            "v.to.db",
            "urbanarea",
            flags="p",
            option="compact",
            type="point",
            format="json",
        )
        self._assert_json_equal(module, reference, has_totals=False)


if __name__ == "__main__":
    test()
