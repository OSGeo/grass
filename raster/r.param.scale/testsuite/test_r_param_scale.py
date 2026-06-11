"""Test of r.param.scale parallelization (disk-strip path).

Verifies that r.param.scale produces the same output when run threaded
(nprocs=4) as it does serially (nprocs=1), and locks in reference
univariate statistics for a sweep of methods, window sizes and the
constrained (-c) flag.

Self-contained: the input DEM is generated in setUpClass with r.mapcalc
from a deterministic polynomial in row()/col() (pure +, -, * with decimal
constants, no trig and no rand), so it is bit-identical on every platform
and GRASS version. A small cubic term makes local curvature vary across
the raster while keeping the fit well conditioned.

The region (400 x 400) together with memory=1 forces the module past its
memory budget, so it uses the multi-band per-thread file-descriptor
disk-strip path (brows < proc_rows) rather than holding the map in RAM.
The same memory setting is used for the serial and threaded runs.

The reference values below were generated from a genuine single-threaded
run (OMP_NUM_THREADS=1, nprocs=1): the one-time LU decomposition relies on
a library pivot search that is only deterministic on a single thread, so
the reference must come from a single-threaded run.

Used dataset: nc_spm_08_grass7 (only as a host location; the DEM is
generated, the NC rasters are not used).

Author: Kaushik Raja
"""

from grass.gunittest.case import TestCase
import grass.script as gs


REFERENCE = {
    "elev_s3": {
        "n": 158404,
        "null_cells": 1596,
        "cells": 160000,
        "min": -564.762512,
        "max": 1491.000666,
        "range": 2055.763178,
        "mean": 426.151076,
        "mean_of_abs": 480.263031516931,
        "stddev": 442.861043442243,
        "variance": 196125.903798752,
        "coeff_var": 103.921136982473,
        "sum": 67504035.042704,
    },
    "elev_s3_c": {
        "n": 158404,
        "null_cells": 1596,
        "cells": 160000,
        "min": -564.762512,
        "max": 1491.000666,
        "range": 2055.763178,
        "mean": 426.151076,
        "mean_of_abs": 480.263031516931,
        "stddev": 442.861043442243,
        "variance": 196125.903798752,
        "coeff_var": 103.921136982473,
        "sum": 67504035.042704,
    },
    "elev_s9": {
        "n": 153664,
        "null_cells": 6336,
        "cells": 160000,
        "min": -546.478838,
        "max": 1467.508536,
        "range": 2013.987374,
        "mean": 425.280891,
        "mean_of_abs": 476.390592098136,
        "stddev": 434.633318407327,
        "variance": 188906.121469765,
        "coeff_var": 102.199117713786,
        "sum": 65350362.834624,
    },
    "elev_s9_c": {
        "n": 153664,
        "null_cells": 6336,
        "cells": 160000,
        "min": -546.478838,
        "max": 1467.508536,
        "range": 2013.987374,
        "mean": 425.280891,
        "mean_of_abs": 476.390592098136,
        "stddev": 434.633318407327,
        "variance": 188906.121469765,
        "coeff_var": 102.199117713786,
        "sum": 65350362.834624,
    },
    "elev_s15": {
        "n": 148996,
        "null_cells": 11004,
        "cells": 160000,
        "min": -528.372536,
        "max": 1444.287922,
        "range": 1972.660458,
        "mean": 424.423924,
        "mean_of_abs": 472.603709161615,
        "stddev": 426.473749373164,
        "variance": 181879.858904405,
        "coeff_var": 100.482966500532,
        "sum": 63237466.980304,
    },
    "elev_s15_c": {
        "n": 148996,
        "null_cells": 11004,
        "cells": 160000,
        "min": -528.372536,
        "max": 1444.287922,
        "range": 1972.660458,
        "mean": 424.423924,
        "mean_of_abs": 472.603709161615,
        "stddev": 426.473749373164,
        "variance": 181879.858904405,
        "coeff_var": 100.482966500532,
        "sum": 63237466.980304,
    },
    "slope_s3": {
        "n": 158404,
        "null_cells": 1596,
        "cells": 160000,
        "min": 31.143096450782,
        "max": 83.9114645026956,
        "range": 52.7683680519136,
        "mean": 72.4250151761828,
        "mean_of_abs": 72.4250151761828,
        "stddev": 10.9080822146547,
        "variance": 118.986257601667,
        "coeff_var": 15.0612080482406,
        "sum": 11472412.1039681,
    },
    "slope_s3_c": {
        "n": 158404,
        "null_cells": 1596,
        "cells": 160000,
        "min": 31.143096450782,
        "max": 83.9114645026956,
        "range": 52.7683680519136,
        "mean": 72.4250151761828,
        "mean_of_abs": 72.4250151761828,
        "stddev": 10.9080822146547,
        "variance": 118.986257601667,
        "coeff_var": 15.0612080482406,
        "sum": 11472412.1039681,
    },
    "slope_s9": {
        "n": 153664,
        "null_cells": 6336,
        "cells": 160000,
        "min": 31.944182807223,
        "max": 83.8375409801301,
        "range": 51.8933581729071,
        "mean": 72.457518347825,
        "mean_of_abs": 72.457518347825,
        "stddev": 10.7142059615324,
        "variance": 114.794209386136,
        "coeff_var": 14.7868795479579,
        "sum": 11134112.0994002,
    },
    "slope_s9_c": {
        "n": 153664,
        "null_cells": 6336,
        "cells": 160000,
        "min": 31.944182807223,
        "max": 83.8375409801301,
        "range": 51.8933581729071,
        "mean": 72.457518347825,
        "mean_of_abs": 72.457518347825,
        "stddev": 10.7142059615324,
        "variance": 114.794209386136,
        "coeff_var": 14.7868795479579,
        "sum": 11134112.0994002,
    },
    "slope_s15": {
        "n": 148996,
        "null_cells": 11004,
        "cells": 160000,
        "min": 32.7563367059122,
        "max": 83.7623712622949,
        "range": 51.0060345563827,
        "mean": 72.4906681285151,
        "mean_of_abs": 72.4906681285151,
        "stddev": 10.5190698109164,
        "variance": 110.650829686932,
        "coeff_var": 14.5109295892647,
        "sum": 10800819.5884762,
    },
    "slope_s15_c": {
        "n": 148996,
        "null_cells": 11004,
        "cells": 160000,
        "min": 32.7563367059122,
        "max": 83.7623712622949,
        "range": 51.0060345563827,
        "mean": 72.4906681285151,
        "mean_of_abs": 72.4906681285151,
        "stddev": 10.5190698109164,
        "variance": 110.650829686932,
        "coeff_var": 14.5109295892647,
        "sum": 10800819.5884762,
    },
    "profc_s3": {
        "n": 158404,
        "null_cells": 1596,
        "cells": 160000,
        "min": -0.00393749267654076,
        "max": 0.00164711282024063,
        "range": 0.00558460549678139,
        "mean": -0.000342552958886785,
        "mean_of_abs": 0.000491699464986382,
        "stddev": 0.000829231153728635,
        "variance": 6.87624306314123e-07,
        "coeff_var": -242.073855214516,
        "sum": -54.2617588995023,
    },
    "profc_s3_c": {
        "n": 158404,
        "null_cells": 1596,
        "cells": 160000,
        "min": -0.00393749267654189,
        "max": 0.00164711282024106,
        "range": 0.00558460549678296,
        "mean": -0.000342552958886785,
        "mean_of_abs": 0.000491699464986382,
        "stddev": 0.000829231153728635,
        "variance": 6.87624306314123e-07,
        "coeff_var": -242.073855214516,
        "sum": -54.2617588995023,
    },
    "profc_s9": {
        "n": 153664,
        "null_cells": 6336,
        "cells": 160000,
        "min": -0.00393703319719625,
        "max": 0.00156307905481015,
        "range": 0.0055001122520064,
        "mean": -0.000343203532196393,
        "mean_of_abs": 0.000487504566970183,
        "stddev": 0.000819692530679842,
        "variance": 6.71895844852323e-07,
        "coeff_var": -238.835691880579,
        "sum": -52.7380275714266,
    },
    "profc_s9_c": {
        "n": 153664,
        "null_cells": 6336,
        "cells": 160000,
        "min": -0.00393703319719643,
        "max": 0.00156307905480931,
        "range": 0.00550011225200574,
        "mean": -0.000343203532196394,
        "mean_of_abs": 0.000487504566970183,
        "stddev": 0.000819692530679842,
        "variance": 6.71895844852324e-07,
        "coeff_var": -238.835691880579,
        "sum": -52.7380275714266,
    },
    "profc_s15": {
        "n": 148996,
        "null_cells": 11004,
        "cells": 160000,
        "min": -0.00393547758337053,
        "max": 0.0014817471036016,
        "range": 0.00541722468697213,
        "mean": -0.000343511486125908,
        "mean_of_abs": 0.000483002036366709,
        "stddev": 0.000809295470184593,
        "variance": 6.54959158061301e-07,
        "coeff_var": -235.594879027701,
        "sum": -51.1818373868158,
    },
    "profc_s15_c": {
        "n": 148996,
        "null_cells": 11004,
        "cells": 160000,
        "min": -0.00393547758337018,
        "max": 0.00148174710360166,
        "range": 0.00541722468697183,
        "mean": -0.000343511486125908,
        "mean_of_abs": 0.000483002036366709,
        "stddev": 0.000809295470184592,
        "variance": 6.54959158061301e-07,
        "coeff_var": -235.594879027701,
        "sum": -51.1818373868158,
    },
    "crosc_s3": {
        "n": 158404,
        "null_cells": 1596,
        "cells": 160000,
        "min": -0.00798326486875052,
        "max": 0.0140198635034536,
        "range": 0.0220031283722041,
        "mean": 0.00523063372892867,
        "mean_of_abs": 0.00700064364287339,
        "stddev": 0.0057384315690963,
        "variance": 3.2929596873201e-05,
        "coeff_var": 109.708151372924,
        "sum": 828.553305197217,
    },
    "crosc_s3_c": {
        "n": 158404,
        "null_cells": 1596,
        "cells": 160000,
        "min": -0.00798326486872778,
        "max": 0.0140198635036052,
        "range": 0.0220031283723329,
        "mean": 0.00523063372892867,
        "mean_of_abs": 0.00700064364287339,
        "stddev": 0.0057384315690963,
        "variance": 3.2929596873201e-05,
        "coeff_var": 109.708151372924,
        "sum": 828.553305197217,
    },
    "crosc_s9": {
        "n": 153664,
        "null_cells": 6336,
        "cells": 160000,
        "min": -0.00790623053867785,
        "max": 0.0139600812849915,
        "range": 0.0218663118236694,
        "mean": 0.00530129926236255,
        "mean_of_abs": 0.00703005215756205,
        "stddev": 0.00570183973099957,
        "variance": 3.25109763180052e-05,
        "coeff_var": 107.555515144763,
        "sum": 814.618849851679,
    },
    "crosc_s9_c": {
        "n": 153664,
        "null_cells": 6336,
        "cells": 160000,
        "min": -0.00790623053867993,
        "max": 0.0139600812849679,
        "range": 0.0218663118236478,
        "mean": 0.00530129926236254,
        "mean_of_abs": 0.00703005215756204,
        "stddev": 0.00570183973099955,
        "variance": 3.25109763180051e-05,
        "coeff_var": 107.555515144763,
        "sum": 814.618849851678,
    },
    "crosc_s15": {
        "n": 148996,
        "null_cells": 11004,
        "cells": 160000,
        "min": -0.00782884301345861,
        "max": 0.0139001591796006,
        "range": 0.0217290021930592,
        "mean": 0.00537273496983023,
        "mean_of_abs": 0.0070588784164276,
        "stddev": 0.00566360869239518,
        "variance": 3.20764634205742e-05,
        "coeff_var": 105.413885557324,
        "sum": 800.516019564825,
    },
    "crosc_s15_c": {
        "n": 148996,
        "null_cells": 11004,
        "cells": 160000,
        "min": -0.00782884301345516,
        "max": 0.0139001591796018,
        "range": 0.021729002193057,
        "mean": 0.00537273496983024,
        "mean_of_abs": 0.00705887841642761,
        "stddev": 0.00566360869239518,
        "variance": 3.20764634205743e-05,
        "coeff_var": 105.413885557324,
        "sum": 800.516019564826,
    },
    "feature_s3": {
        "n": 158404,
        "null_cells": 1596,
        "cells": 160000,
        "min": 1.0,
        "max": 5.0,
        "range": 4.0,
        "mean": 4.56869776015757,
        "mean_of_abs": 4.56869776015757,
        "stddev": 0.841755158740511,
        "variance": 0.708551747266264,
        "coeff_var": 18.4244001886323,
        "sum": 723700.0,
    },
    "feature_s3_c": {
        "n": 158404,
        "null_cells": 1596,
        "cells": 160000,
        "min": 1.0,
        "max": 5.0,
        "range": 4.0,
        "mean": 4.56869776015757,
        "mean_of_abs": 4.56869776015757,
        "stddev": 0.841755158740511,
        "variance": 0.708551747266264,
        "coeff_var": 18.4244001886323,
        "sum": 723700.0,
    },
    "feature_s9": {
        "n": 153664,
        "null_cells": 6336,
        "cells": 160000,
        "min": 1.0,
        "max": 5.0,
        "range": 4.0,
        "mean": 4.57504685547688,
        "mean_of_abs": 4.57504685547688,
        "stddev": 0.837584186678913,
        "variance": 0.701547269774577,
        "coeff_var": 18.3076635745539,
        "sum": 703020.0,
    },
    "feature_s9_c": {
        "n": 153664,
        "null_cells": 6336,
        "cells": 160000,
        "min": 1.0,
        "max": 5.0,
        "range": 4.0,
        "mean": 4.57504685547688,
        "mean_of_abs": 4.57504685547688,
        "stddev": 0.837584186678913,
        "variance": 0.701547269774577,
        "coeff_var": 18.3076635745539,
        "sum": 703020.0,
    },
    "feature_s15": {
        "n": 148996,
        "null_cells": 11004,
        "cells": 160000,
        "min": 1.0,
        "max": 5.0,
        "range": 4.0,
        "mean": 4.58155923648957,
        "mean_of_abs": 4.58155923648957,
        "stddev": 0.833230429104926,
        "variance": 0.69427294798638,
        "coeff_var": 18.1866125939988,
        "sum": 682634.0,
    },
    "feature_s15_c": {
        "n": 148996,
        "null_cells": 11004,
        "cells": 160000,
        "min": 1.0,
        "max": 5.0,
        "range": 4.0,
        "mean": 4.58155923648957,
        "mean_of_abs": 4.58155923648957,
        "stddev": 0.833230429104926,
        "variance": 0.69427294798638,
        "coeff_var": 18.1866125939988,
        "sum": 682634.0,
    },
}


class TestParamScale(TestCase):
    """Parallel-vs-serial and reference-value regression for r.param.scale
    on a generated DEM, exercising the disk-strip (multi-band) path.
    """

    # Generated input DEM and the fixed, reproducible region.
    input_dem = "rps_test_dem"
    region = {"n": 400, "s": 0, "e": 400, "w": 0, "res": 1}
    dem_expr = (
        "double(100.0 + 0.5*col() + 0.4*row() "
        "+ 0.003*col()*col() - 0.002*row()*row() + 0.0015*col()*row() "
        "+ 0.00001*col()*col()*col() - 0.000008*row()*row()*row())"
    )
    # Low memory budget forces the multi-band disk-strip path; identical for
    # the serial and threaded runs so the comparison stays valid.
    memory = 1

    test_options = REFERENCE
    to_remove = []

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        cls.runModule("g.region", **cls.region)
        cls.runModule(
            "r.mapcalc",
            expression="{} = {}".format(cls.input_dem, cls.dem_expr),
            overwrite=True,
        )
        cls.to_remove.append(cls.input_dem)

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()
        if cls.to_remove:
            cls.runModule(
                "g.remove", flags="f", type="raster", name=",".join(cls.to_remove)
            )

    @staticmethod
    def _key(method, size, constrained):
        return "{}_s{}{}".format(method, size, "_c" if constrained else "")

    def _assert_identical(self, a, b, label):
        """Null-aware bit-exact equality of two rasters.

        The diff is 1 wherever the null pattern differs OR (both non-null)
        the values differ, and 0 otherwise. It is never null, so r.univar
        does not skip cells. The differing-cell sum must be exactly 0, and
        the two rasters must report the same non-null count.
        """
        diff = "rps_diff_{}".format(label)
        self.to_remove.append(diff)
        expr = (
            "{d} = if(isnull({a}) != isnull({b}), 1, "
            "if(isnull({a}), 0, if({a} != {b}, 1, 0)))"
        ).format(d=diff, a=a, b=b)
        self.runModule("r.mapcalc", expression=expr, overwrite=True)
        dstats = gs.parse_command("r.univar", map=diff, flags="g")
        differing = int(float(dstats.get("sum", 0) or 0))
        self.assertEqual(
            differing,
            0,
            msg="{} and {} differ in {} cells ({})".format(a, b, differing, label),
        )
        a_n = int(gs.parse_command("r.univar", map=a, flags="g")["n"])
        b_n = int(gs.parse_command("r.univar", map=b, flags="g")["n"])
        self.assertEqual(
            a_n,
            b_n,
            msg="non-null count mismatch ({} vs {}) for {}".format(a_n, b_n, label),
        )

    def _check_combo(self, method, size, constrained):
        key = self._key(method, size, constrained)
        serial = "rps_{}_serial".format(key)
        threaded = "rps_{}_threaded".format(key)
        self.to_remove.extend([serial, threaded])

        common = {
            "input": self.input_dem,
            "method": method,
            "size": size,
            "memory": self.memory,
            "overwrite": True,
        }
        serial_kw = dict(common, output=serial, nprocs=1)
        threaded_kw = dict(common, output=threaded, nprocs=4)
        if constrained:
            serial_kw["flags"] = "c"
            threaded_kw["flags"] = "c"

        self.assertModule("r.param.scale", **serial_kw)
        self.assertModule("r.param.scale", **threaded_kw)

        # (a) reference-value regression guard (includes cell/null counts).
        reference = self.test_options[key]
        self.assertRasterFitsUnivar(raster=serial, reference=reference, precision=1e-5)
        self.assertRasterFitsUnivar(
            raster=threaded, reference=reference, precision=1e-5
        )
        # (b) null-aware bit-exact serial vs threaded.
        self._assert_identical(serial, threaded, key)

    def _check_method(self, method):
        for size in (3, 9, 15):
            for constrained in (False, True):
                self._check_combo(method, size, constrained)

    def test_elev(self):
        """Method elev at sizes 3, 9, 15, with and without -c."""
        self._check_method("elev")

    def test_slope(self):
        """Method slope at sizes 3, 9, 15, with and without -c."""
        self._check_method("slope")

    def test_profc(self):
        """Method profc (profile curvature) at sizes 3, 9, 15, +/- -c."""
        self._check_method("profc")

    def test_crosc(self):
        """Method crosc (cross-sectional curvature) at sizes 3, 9, 15, +/- -c."""
        self._check_method("crosc")

    def test_feature(self):
        """Method feature (morphometric features) at sizes 3, 9, 15, +/- -c."""
        self._check_method("feature")

    def test_single_band_and_memory_invariance(self):
        """Single-band path (memory=300) and invariance to band count.

        At size 15 the whole 400x400 map fits in memory=300 (one band),
        whereas memory=1 forces multiple bands. For every method (default
        and -c): the single-band serial and threaded outputs must be
        identical, and the single-band serial output must be identical to
        the multi-band (memory=1) serial output, i.e. the result is
        invariant to how many bands the module uses.
        """
        for method in ("elev", "slope", "profc", "crosc", "feature"):
            for constrained in (False, True):
                tag = "{}{}".format(method, "_c" if constrained else "")
                s_m1 = "rps_inv_{}_s_m1".format(tag)
                s_m300 = "rps_inv_{}_s_m300".format(tag)
                t_m300 = "rps_inv_{}_t_m300".format(tag)
                self.to_remove.extend([s_m1, s_m300, t_m300])

                base = {
                    "input": self.input_dem,
                    "method": method,
                    "size": 15,
                    "overwrite": True,
                }
                runs = [
                    dict(base, output=s_m1, nprocs=1, memory=1),
                    dict(base, output=s_m300, nprocs=1, memory=300),
                    dict(base, output=t_m300, nprocs=4, memory=300),
                ]
                if constrained:
                    for r in runs:
                        r["flags"] = "c"
                for r in runs:
                    self.assertModule("r.param.scale", **r)

                # single-band serial == single-band threaded
                self._assert_identical(s_m300, t_m300, "inv_{}_par".format(tag))
                # band-count invariance: one band (mem300) == multi band (mem1)
                self._assert_identical(s_m300, s_m1, "inv_{}_mem".format(tag))
                # lock single-band output to the stored size-15 reference
                reference = self.test_options[self._key(method, 15, constrained)]
                self.assertRasterFitsUnivar(
                    raster=s_m300, reference=reference, precision=1e-5
                )
                self.assertRasterFitsUnivar(
                    raster=t_m300, reference=reference, precision=1e-5
                )

    def test_mask(self):
        """Masked sub-region: serial and threaded must match with a mask
        active (exercises Rast_disable_omp_on_mask); masked cells are null
        in both. Method elev, size 9, memory=1.
        """
        maskmap = "rps_maskmap"
        serial = "rps_mask_serial"
        threaded = "rps_mask_threaded"
        self.to_remove.extend([maskmap, serial, threaded])
        self.runModule(
            "r.mapcalc",
            expression=(
                "{} = if(row() > 100 && row() < 300 "
                "&& col() > 100 && col() < 300, 1, null())".format(maskmap)
            ),
            overwrite=True,
        )
        self.runModule("r.mask", raster=maskmap)
        try:
            base = {
                "input": self.input_dem,
                "method": "elev",
                "size": 9,
                "overwrite": True,
                "memory": 1,
            }
            self.assertModule("r.param.scale", output=serial, nprocs=1, **base)
            self.assertModule("r.param.scale", output=threaded, nprocs=4, **base)
            self._assert_identical(serial, threaded, "mask")
        finally:
            self.runModule("r.mask", flags="r")


if __name__ == "__main__":
    from grass.gunittest.main import test

    test()
