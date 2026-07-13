"""Tests of r.mapcalc rand() with a given or automatic seed

A given seed must reproduce the reference rasters regardless of nprocs:
r.mapcalc evaluates expressions with rand() single-threaded even
when more processes are requested.
"""

import io

import numpy as np
import pytest

import grass.script.array as garray
from grass.tools import Tools

cell_seed_500 = """\
north: 20
south: 10
east: 25
west: 15
rows: 10
cols: 10
121 12 183 55 37 96 138 117 182 40
157 70 115 1 149 125 42 193 108 24
83 66 82 84 186 182 179 122 67 113
151 93 144 173 128 196 61 125 64 193
180 175 14 41 44 27 165 27 90 60
97 57 12 104 98 13 87 24 83 107
174 133 146 114 115 60 78 154 49 130
55 138 144 25 32 58 47 137 139 32
143 193 155 190 131 124 87 81 160 154
56 45 48 66 9 182 69 12 154 19
"""

dcell_seed_600 = """\
north: 20
south: 10
east: 25
west: 15
rows: 10
cols: 10
130.790433856418332 101.3319248101491041 33.5781271447787759 37.4064724824657944 98.2794723130458152 73.9118866262841863 185.9530433718733775 74.5210037729812882 166.1178416001017695 90.9915650902159996
109.2478664232956334 25.6499350759712215 150.9024447059825036 125.7544119036241312 66.7235333366722614 167.9375729129454271 123.1009291055983965 12.0922254083554606 59.389026967287819 113.2843489528100349
40.0044184023145277 135.8273774212801186 71.6737798852435049 191.6223505280372876 4.1546013811569615 143.3082794522489962 177.043829294835632 115.0300571162354402 141.8985452774071234 127.8949967123638061
93.2842559637482793 9.7471880052423856 118.1216452002055632 158.1474162140586088 67.2957262519499437 3.6524546350146849 147.0965842667525862 37.060628529871579 47.3408278816968959 66.2219633495724054
175.5638637866295539 67.1399023507611901 162.2058392782793703 198.1586789345953719 36.474049475167746 49.2589028048889617 112.1169663235969836 22.0227597984432535 95.9169228571662131 86.7470895014531322
93.5401613888204935 193.7821104138942587 193.8286564351004699 3.2623643889134684 94.6955247357847725 25.7099391122614307 155.592251526775442 25.3392337002970294 48.3979699868663005 99.6836079482556272
104.16296861365457 190.7865884377180805 6.2841805474238619 49.3731395705159528 100.1903962703459285 116.927654961282343 19.8626348109264264 40.9693022766258466 81.6500759554420057 169.2220572316770131
118.8112518721558217 55.8955021401724039 112.9150308215961331 62.6399760484719081 85.400498505854145 191.0144187084912062 124.2128169358724534 167.9341741649760706 170.6149695243870781 158.3034517206661462
130.0453795775294736 64.1996403829061535 62.9317494959142465 175.1909990236256931 122.9624852869890361 79.9546265736285733 9.6594013716963367 114.0611338072915544 11.9371167643030809 186.9121199748369122
3.2891990250261536 30.9245408751958379 46.4021422454598991 104.2378950097200203 47.424093232347019 73.4801303522840499 22.4778583078695213 132.870185207462697 48.1666164169167388 100.5504714442693057
"""

fcell_seed_700 = """\
north: 20
south: 10
east: 25
west: 15
rows: 10
cols: 10
146.756378 192.682159 2.644822 147.270462 62.178818 192.668198 94.320778 107.710426 98.319664 114.444504
12.995321 18.026272 151.590958 5.249451 197.266708 103.663635 115.424088 28.01062 78.555168 62.912098
164.053619 154.652039 98.536011 44.601639 85.322289 168.383957 44.93845 128.62262 89.910591 107.242188
111.182487 63.080284 177.791473 47.439354 42.451859 72.396568 170.597778 170.622742 141.88858 105.126854
120.76828 148.581085 42.124866 56.432236 164.652176 98.094009 60.741329 66.286987 187.847427 160.120056
50.530689 179.090652 138.114014 138.629211 193.147903 172.861481 133.72728 108.720459 103.508438 28.81559
39.653179 101.948265 35.744762 25.570076 78.767021 154.600616 144.907684 82.370148 116.378654 18.218494
35.587288 66.534409 65.744408 186.476959 137.081116 151.379272 48.261463 8.323328 130.432739 53.346546
152.67189 15.512391 146.049072 185.276245 34.417141 127.522453 124.54998 52.08218 167.141342 87.771118
69.0522 43.57811 63.15279 68.677063 74.202805 97.429077 167.123199 19.892767 120.593437 190.960815
"""

# Region matching the reference rasters (10 rows x 10 cols).
REGION = {"n": 20, "s": 10, "e": 25, "w": 15, "res": 1}


@pytest.mark.parametrize("nprocs", [1, 4])
def test_seed_cell(session_in_mapset, nprocs):
    """A given seed reproduces the CELL reference regardless of nprocs."""
    tools = Tools(session=session_in_mapset)
    tools.g_region(**REGION)
    tools.r_in_ascii(input=io.StringIO(cell_seed_500), output="ref", type="CELL")
    tools.r_mapcalc(expression="rand_cell = rand(1, 200)", seed=500, nprocs=nprocs)
    env = session_in_mapset.env
    assert np.array_equal(
        garray.array("rand_cell", env=env), garray.array("ref", env=env)
    )


@pytest.mark.parametrize("nprocs", [1, 4])
def test_seed_dcell(session_in_mapset, nprocs):
    """A given seed reproduces the DCELL reference regardless of nprocs."""
    tools = Tools(session=session_in_mapset)
    tools.g_region(**REGION)
    tools.r_in_ascii(input=io.StringIO(dcell_seed_600), output="ref", type="DCELL")
    tools.r_mapcalc(expression="rand_dcell = rand(1.0, 200.0)", seed=600, nprocs=nprocs)
    env = session_in_mapset.env
    assert np.allclose(
        garray.array("rand_dcell", env=env),
        garray.array("ref", env=env),
        rtol=0,
        atol=1e-14,
    )


@pytest.mark.parametrize("nprocs", [1, 4])
def test_seed_fcell(session_in_mapset, nprocs):
    """A given seed reproduces the FCELL reference regardless of nprocs."""
    tools = Tools(session=session_in_mapset)
    tools.g_region(**REGION)
    tools.r_in_ascii(input=io.StringIO(fcell_seed_700), output="ref", type="FCELL")
    tools.r_mapcalc(
        expression="rand_fcell = rand(float(1), 200)", seed=700, nprocs=nprocs
    )
    env = session_in_mapset.env
    assert np.allclose(
        garray.array("rand_fcell", env=env),
        garray.array("ref", env=env),
        rtol=0,
        atol=1e-6,
    )


def test_rand_nprocs_single_thread_message(session_in_mapset):
    """r.mapcalc reports that rand() does not support parallel execution.

    The seed tests rely on rand() forcing single-threaded evaluation for
    any nprocs; this pins the message reporting it. The message is emitted
    at verbose level only.
    """
    tools = Tools(session=session_in_mapset, consistent_return_value=True)
    tools.g_region(**REGION)
    result = tools.r_mapcalc(
        expression="rand_nprocs = rand(1, 200)", seed=500, nprocs=4, verbose=True
    )
    assert "Parallel execution is not supported with rand()" in result.stderr


def test_seed_not_required(session_in_mapset):
    """No seed is needed when rand() is not used."""
    tools = Tools(session=session_in_mapset)
    tools.g_region(**REGION)
    tools.r_mapcalc(expression="nonrand_cell = 200")
    assert np.all(garray.array("nonrand_cell", env=session_in_mapset.env) == 200)


def test_auto_seed_differs(session_in_mapset):
    """Two automatically seeded runs produce different rasters."""
    tools = Tools(session=session_in_mapset)
    tools.g_region(**REGION)
    tools.r_mapcalc(expression="rand_auto_1 = rand(1., 2)")
    tools.r_mapcalc(expression="rand_auto_2 = rand(1., 2)")
    env = session_in_mapset.env
    assert not np.array_equal(
        garray.array("rand_auto_1", env=env), garray.array("rand_auto_2", env=env)
    )
