from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.script.raster import raster_info
from grass.script import tempfile


class TestNeighbors(TestCase):
    """

    Used dataset: nc_spm_08_grass7

    Test cases:
    test_standard_options: Test output with standard options
        (size=3, quadratic moving window, standard statistics).
    test_standard_options_quantile: Test output with standard options
        (size=3, quadratic moving window, quantile statistics).
    test_standard_options_circular: Test output with circular neighborhood
        (size=7, standard statistics).
    test_standard_options_selection: Test output with selection raster
        (standard options otherwise).
    test_weighting_function: Test results with gaussian weighting
        (standard options otherwise).
    test_weighting_file: Test results with file for weighting
        (standard options otherwise).
    """

    test_options = {
        "test_standard_options": {
            "minimum": {
                "n": 2025000,
                "null_cells": 0,
                "cells": 2025000,
                "min": 55.5787925720215,
                "max": 156.201751708984,
                "range": 100.622959136963,
                "mean": 109.482040656279,
                "mean_of_abs": 109.482040656279,
                "stddev": 20.3075456214408,
                "variance": 412.3964091669,
                "coeff_var": 18.5487459858342,
                "sum": 221701132.328964,
            },
            "perc90": {
                "n": 2022154,
                "null_cells": 2846,
                "cells": 2025000,
                "min": 50.5460597991944,
                "max": 153.0166015625,
                "range": 102.470541763306,
                "mean": 100.145484105243,
                "mean_of_abs": 100.145484105243,
                "stddev": 18.2647774894326,
                "variance": 333.602096738483,
                "coeff_var": 18.2382437437099,
                "sum": 202509591.265354,
            },
            "average": {
                "n": 2025000,
                "null_cells": 0,
                "cells": 2025000,
                "min": 56.0054066975911,
                "max": 156.26158481174,
                "range": 100.256178114149,
                "mean": 110.375489343136,
                "mean_of_abs": 110.375489343136,
                "stddev": 20.302357806975,
                "variance": 412.18573252244,
                "coeff_var": 18.3939006094541,
                "sum": 223510365.91985,
            },
            "maximum": {
                "n": 2025000,
                "null_cells": 0,
                "cells": 2025000,
                "min": 56.0526123046875,
                "max": 156.329864501953,
                "range": 100.277252197266,
                "mean": 111.262932473037,
                "mean_of_abs": 111.262932473037,
                "stddev": 20.2987699017146,
                "variance": 412.040059522756,
                "coeff_var": 18.2439644997077,
                "sum": 225307438.2579,
            },
            "stddev": {
                "n": 2025000,
                "null_cells": 0,
                "cells": 2025000,
                "min": 0,
                "max": 6.39749880494391,
                "range": 6.39749880494391,
                "mean": 0.576904240112151,
                "mean_of_abs": 0.576904240112151,
                "stddev": 0.440364969041587,
                "variance": 0.193921305958998,
                "coeff_var": 76.3324202567793,
                "sum": 1168231.08622711,
            },
            "sum": {
                "n": 2025000,
                "null_cells": 0,
                "cells": 2025000,
                "min": 233.24821472168,
                "max": 1406.35424804688,
                "range": 1173.1060333252,
                "mean": 992.482567853469,
                "mean_of_abs": 992.482567853469,
                "stddev": 183.486667152836,
                "variance": 33667.3570228558,
                "coeff_var": 18.4876463422103,
                "sum": 2009777199.90327,
            },
        },
        "test_standard_options_quantile": {
            0.01: {
                "n": 2025000,
                "null_cells": 0,
                "cells": 2025000,
                "min": 55.5858826446533,
                "max": 156.204046478271,
                "range": 100.618163833618,
                "mean": 109.505643774445,
                "mean_of_abs": 109.505643774445,
                "stddev": 20.3076172385227,
                "variance": 412.399317906343,
                "coeff_var": 18.5448133434578,
                "sum": 221748928.643252,
            },
            0.03: {
                "n": 2025000,
                "null_cells": 0,
                "cells": 2025000,
                "min": 55.600062789917,
                "max": 156.208636016846,
                "range": 100.608573226929,
                "mean": 109.552850010781,
                "mean_of_abs": 109.552850010781,
                "stddev": 20.3078616792495,
                "variance": 412.409245983529,
                "coeff_var": 18.5370455239192,
                "sum": 221844521.271832,
            },
            0.95: {
                "n": 2022154,
                "null_cells": 2846,
                "cells": 2025000,
                "min": 25.2730298995972,
                "max": 153.0166015625,
                "range": 127.743571662903,
                "mean": 50.141999640313,
                "mean_of_abs": 50.141999640313,
                "stddev": 9.33982798423495,
                "variance": 87.2323867750984,
                "coeff_var": 18.6267561150991,
                "sum": 101394845.140658,
            },
        },
        "test_standard_options_circular": {
            "minimum": {
                "n": 2025000,
                "null_cells": 0,
                "cells": 2025000,
                "min": 55.5787925720215,
                "max": 155.872573852539,
                "range": 100.293781280518,
                "mean": 108.457334170361,
                "mean_of_abs": 108.457334170361,
                "stddev": 20.2561980947474,
                "variance": 410.313561253647,
                "coeff_var": 18.67665128384,
                "sum": 219626101.694981,
            },
            "perc90": {
                "n": 2025000,
                "null_cells": 0,
                "cells": 2025000,
                "min": 56.4984405517578,
                "max": 156.306396484375,
                "range": 99.8079559326172,
                "mean": 111.82648180616,
                "mean_of_abs": 111.82648180616,
                "stddev": 20.2829112001524,
                "variance": 411.396486753269,
                "coeff_var": 18.1378425508466,
                "sum": 226448625.657474,
            },
            "average": {
                "n": 2025000,
                "null_cells": 0,
                "cells": 2025000,
                "min": 56.1745609707303,
                "max": 156.191996607287,
                "range": 100.017435636557,
                "mean": 110.375576827845,
                "mean_of_abs": 110.375576827845,
                "stddev": 20.2777760438563,
                "variance": 411.188201284794,
                "coeff_var": 18.371615013604,
                "sum": 223510543.076386,
            },
            "maximum": {
                "n": 2025000,
                "null_cells": 0,
                "cells": 2025000,
                "min": 56.6209869384766,
                "max": 156.329864501953,
                "range": 99.7088775634766,
                "mean": 112.257478616111,
                "mean_of_abs": 112.257478616111,
                "stddev": 20.268387328814,
                "variance": 410.807524910826,
                "coeff_var": 18.0552668549827,
                "sum": 227321394.197624,
            },
            "stddev": {
                "n": 2025000,
                "null_cells": 0,
                "cells": 2025000,
                "min": 0,
                "max": 9.34457118441325,
                "range": 9.34457118441325,
                "mean": 1.02915670094965,
                "mean_of_abs": 1.02915670094965,
                "stddev": 0.682319147525201,
                "variance": 0.465559419079517,
                "coeff_var": 66.2988587545119,
                "sum": 2084042.31942303,
            },
        },
        "test_standard_options_selection": {
            "minimum": {
                "n": 2025000,
                "null_cells": 0,
                "cells": 2025000,
                "min": 55.5787925720215,
                "max": 156.329864501953,
                "range": 100.751071929932,
                "mean": 110.375440275606,
                "mean_of_abs": 110.375440275606,
                "stddev": 20.3153233205981,
                "variance": 412.712361620436,
                "coeff_var": 18.4056555243368,
                "sum": 223510266.558102,
            },
            "perc90": {
                "n": 2025000,
                "null_cells": 0,
                "cells": 2025000,
                "min": 55.5787925720215,
                "max": 156.329864501953,
                "range": 100.751071929932,
                "mean": 110.375440275606,
                "mean_of_abs": 110.375440275606,
                "stddev": 20.3153233205981,
                "variance": 412.712361620436,
                "coeff_var": 18.4056555243368,
                "sum": 223510266.558102,
            },
            "average": {
                "n": 2025000,
                "null_cells": 0,
                "cells": 2025000,
                "min": 55.5787925720215,
                "max": 156.329864501953,
                "range": 100.751071929932,
                "mean": 110.375440275606,
                "mean_of_abs": 110.375440275606,
                "stddev": 20.3153233205981,
                "variance": 412.712361620436,
                "coeff_var": 18.4056555243368,
                "sum": 223510266.558102,
            },
            "maximum": {
                "n": 2025000,
                "null_cells": 0,
                "cells": 2025000,
                "min": 55.5787925720215,
                "max": 156.329864501953,
                "range": 100.751071929932,
                "mean": 110.375440275606,
                "mean_of_abs": 110.375440275606,
                "stddev": 20.3153233205981,
                "variance": 412.712361620436,
                "coeff_var": 18.4056555243368,
                "sum": 223510266.558102,
            },
            "stddev": {
                "n": 2025000,
                "null_cells": 0,
                "cells": 2025000,
                "min": 55.5787925720215,
                "max": 156.329864501953,
                "range": 100.751071929932,
                "mean": 110.375440275606,
                "mean_of_abs": 110.375440275606,
                "stddev": 20.3153233205981,
                "variance": 412.712361620436,
                "coeff_var": 18.4056555243368,
                "sum": 223510266.558102,
            },
        },
        "test_weighting_function": {
            "perc90": {
                "n": 2025000,
                "null_cells": 0,
                "cells": 2025000,
                "min": 56.4677543640137,
                "max": 156.30549621582,
                "range": 99.8377418518066,
                "mean": 111.789766253374,
                "mean_of_abs": 111.789766253374,
                "stddev": 20.2825397924116,
                "variance": 411.381420430759,
                "coeff_var": 18.1434673961486,
                "sum": 226374276.663082,
            },
            "average": {
                "n": 2025000,
                "null_cells": 0,
                "cells": 2025000,
                "min": 56.235548348967,
                "max": 156.164602692463,
                "range": 99.9290543434962,
                "mean": 110.375593417834,
                "mean_of_abs": 110.375593417834,
                "stddev": 20.2736626611521,
                "variance": 411.021397698193,
                "coeff_var": 18.3678855382501,
                "sum": 223510576.671115,
            },
            "stddev": {
                "n": 2025000,
                "null_cells": 0,
                "cells": 2025000,
                "min": 0,
                "max": 9.43832203774046,
                "range": 9.43832203774046,
                "mean": 1.09653550798646,
                "mean_of_abs": 1.09653550798646,
                "stddev": 0.699381561482314,
                "variance": 0.48913456854144,
                "coeff_var": 63.7810227200549,
                "sum": 2220484.40367258,
            },
            "sum": {
                "n": 2025000,
                "null_cells": 0,
                "cells": 2025000,
                "min": 18.3855979069574,
                "max": 133.048299539197,
                "range": 114.66270163224,
                "mean": 93.868177703987,
                "mean_of_abs": 93.868177703987,
                "stddev": 17.4006236823834,
                "variance": 302.781704535922,
                "coeff_var": 18.5372978447033,
                "sum": 190083059.850574,
            },
        },
        "test_weighting_file": {
            "perc90": {
                "n": 2025000,
                "null_cells": 0,
                "cells": 2025000,
                "min": 56.2366943359375,
                "max": 156.314498901367,
                "range": 100.07780456543,
                "mean": 111.666863002851,
                "mean_of_abs": 111.666863002851,
                "stddev": 20.2890537023546,
                "variance": 411.64570013703,
                "coeff_var": 18.1692698771673,
                "sum": 226125397.580772,
            },
            "average": {
                "n": 2025000,
                "null_cells": 0,
                "cells": 2025000,
                "min": 56.0924235752651,
                "max": 156.219111124674,
                "range": 100.126687549409,
                "mean": 110.375616311748,
                "mean_of_abs": 110.375616311748,
                "stddev": 20.2776463025833,
                "variance": 411.182939572671,
                "coeff_var": 18.3714908964228,
                "sum": 223510623.03129,
            },
            "stddev": {
                "n": 2025000,
                "null_cells": 0,
                "cells": 2025000,
                "min": 0,
                "max": 10.0342267239471,
                "range": 10.0342267239471,
                "mean": 1.01708477919276,
                "mean_of_abs": 1.01708477919276,
                "stddev": 0.70410774305486,
                "variance": 0.495767713829808,
                "coeff_var": 69.2280287208402,
                "sum": 2059596.67786534,
            },
            "sum": {
                "n": 2025000,
                "null_cells": 0,
                "cells": 2025000,
                "min": 233.716758728027,
                "max": 1874.62933349609,
                "range": 1640.91257476807,
                "mean": 1322.11428605925,
                "mean_of_abs": 1322.11428605925,
                "stddev": 245.497028073259,
                "variance": 60268.7907928026,
                "coeff_var": 18.568517915724,
                "sum": 2677281429.26999,
            },
        },
    }

    # TODO: replace by unified handing of maps
    to_remove = []

    def check_univar(self, test_case):
        """Checks multiple output against unviariate reference from dict"""
        for method in self.test_options[test_case]:
            self.assertRasterFitsUnivar(
                raster="{}_raster_{}".format(test_case, method),
                reference=self.test_options[test_case][method],
                precision=1e-5,
            )

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        cls.runModule("g.region", raster="elevation")

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()
        if cls.to_remove:
            cls.runModule(
                "g.remove", flags="f", type="raster", name=",".join(cls.to_remove)
            )

    def test_standard_options(self):
        """Test output with standard options (size=3, quadratic moving window,
        standard statistics).
        """
        test_case = "test_standard_options"
        outputs = [
            "{}_raster_{}".format(test_case, method)
            for method in self.test_options[test_case]
        ]
        self.to_remove.extend(outputs)
        self.assertModule(
            "r.neighbors",
            input="elevation",
            output=",".join(outputs),
            method=list(self.test_options[test_case].keys()),
        )
        self.check_univar(test_case)

    def test_standard_options_quantile(self):
        """Test output with standard options (size=3, quadratic moving window,
        quantile statistics).
        """
        test_case = "test_standard_options_quantile"
        outputs = [
            "{}_raster_{}".format(test_case, method)
            for method in self.test_options[test_case]
        ]
        self.to_remove.extend(outputs)
        self.assertModule(
            "r.neighbors",
            input="elevation",
            output=",".join(outputs),
            method=["quantile"] * len(self.test_options[test_case]),
            quantile=list(self.test_options[test_case].keys()),
        )
        self.check_univar(test_case)

    def test_standard_options_circular(self):
        """Test output with circular neighborhood (size=7,
        standard statistics)."""
        test_case = "test_standard_options_circular"
        outputs = [
            "{}_raster_{}".format(test_case, method)
            for method in self.test_options[test_case]
        ]
        self.to_remove.extend(outputs)
        self.assertModule(
            "r.neighbors",
            flags="c",
            input="elevation",
            size=7,
            output=",".join(outputs),
            method=list(self.test_options[test_case].keys()),
        )
        self.check_univar(test_case)

    def test_standard_options_selection(self):
        """Test output with selection raster (standard options otherwise)."""
        test_case = "test_standard_options_selection"
        outputs = [
            "{}_raster_{}".format(test_case, method)
            for method in self.test_options[test_case]
        ]
        self.to_remove.extend(outputs)
        selection = "test_neighbors_selection"
        self.to_remove.append(selection)
        self.runModule(
            "r.mapcalc", expression="{}=if(y()==15,1,null())".format(selection),
        )
        self.assertModule(
            "r.neighbors",
            input="elevation",
            output=",".join(outputs),
            selection=selection,
            method=list(self.test_options[test_case].keys()),
        )
        self.check_univar(test_case)

    def test_weighting_function(self):
        """Test results with gaussian weighting (standard options otherwise)."""
        test_case = "test_weighting_function"
        outputs = [
            "{}_raster_{}".format(test_case, method)
            for method in self.test_options[test_case]
        ]
        self.to_remove.extend(outputs)
        self.assertModule(
            "r.neighbors",
            flags="c",
            input="elevation",
            size=7,
            gauss=2.0,
            output=",".join(outputs),
            method=list(self.test_options[test_case].keys()),
        )
        self.check_univar(test_case)

    def test_weighting_file(self):
        """Test results with file for weighting (standard options otherwise)."""

        test_case = "test_weighting_file"
        outputs = [
            "{}_raster_{}".format(test_case, method)
            for method in self.test_options[test_case]
        ]
        self.to_remove.extend(outputs)

        weights = tempfile()
        with open(weights, "w") as w:
            w.write("0 1 1 1 0\n1 0 0 0 1\n1 0 0 0 1\n1 0 0 0 1\n0 1 1 1 0")

        self.assertModule(
            "r.neighbors",
            input="elevation",
            size=5,
            output=",".join(outputs),
            method=list(self.test_options[test_case].keys()),
            weight=weights,
        )
        self.check_univar(test_case)

        # Module fails with mutual exclusive options
        self.assertModuleFail(
            "r.neighbors",
            input="elevation",
            flags="c",
            size=5,
            output="{}_fails".format(test_case),
            method="sum",
            weight=weights,
        )

    def test_standard_options_datatype(self):
        """Test if result is of integer or float data type depending on
        input datatype and method"""
        # landclass96 (values 1-7)

        test_case = "test_standard_options_datatype"

        methods = ["average", "variance", "diversity", "range", "mode"]
        outputs = ["{}_{}".format(test_case, method) for method in methods]
        self.to_remove.extend(outputs)
        self.assertModule(
            "r.neighbors",
            input="landclass96",
            output=",".join(outputs),
            method=methods,
        )
        for rmap in outputs:
            rinfo = raster_info(rmap)
            self.assertTrue(
                rinfo["datatype"] == "CELL"
                if rinfo["datatype"] in ["diversity", "range", "mode"]
                else "DCELL"
            )


if __name__ == "__main__":
    test()
