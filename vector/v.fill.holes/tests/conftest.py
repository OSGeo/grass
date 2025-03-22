"""Fixture for v.fill.holes test"""

import os

from types import SimpleNamespace

import pytest

import grass.script as gs

DATA = """\
VERTI:
C  1 1
 26.4740753   19.30631132
 1     1
B  3
 35.7009422   5.30814651
 39.02877384  19.25061356
 35.75831861  33.47996264
B  3
 35.75831861  33.47996264
 26.6354698   35.71764254
 20.03718292  32.73406934
C  1 1
 42.19882036  26.59479373
 1     3
B  8 1
 31.98581981  29.69311974
 27.51046001  31.70129401
 26.07604982  27.45543984
 29.23175224  23.78334976
 33.82186485  23.20958568
 35.02676941  26.76692295
 34.51038174  29.52099052
 31.98581981  29.69311974
 1     4
B  7 1
 42.02669114  33.25045701
 39.55950561  28.602968
 43.00209007  28.37346236
 45.64140482  31.6439176
 45.18239356  35.48813691
 41.96931473  33.99635031
 42.02669114  33.25045701
 1     5
B  7 1
 23.83836993  17.2998157
 21.83019566  21.60304627
 18.27285839  20.45551812
 17.46958868  17.18506288
 21.14167877  12.65232668
 24.3547576   14.48837172
 23.83836993  17.2998157
 1     6
B  7 1
 31.52680855  17.64407414
 28.60061176  12.8244559
 33.36285359  9.66875349
 36.63330882  13.5129728
 36.69068523  17.24243929
 32.33007825  18.21783822
 31.52680855  17.64407414
 1     7
B  7
 46.35860991  38.87334496
 33.25872825  43.10254145
 27.62998922  41.83894698
 17.32020703  35.66456488
 11.83505828  29.34659251
 8.81966238   23.22964653
 11.43072178  20.57027093
B  2
 20.03718292  32.73406934
 11.43072178  20.57027093
B  2
 35.75831861  33.47996264
 46.35860991  38.87334496
C  1 1
 30.9900018   38.79483301
 1     9
B  12 1
 20.38144137  3.7589835
 2.42754667   6.02567852
 0.12531832   16.15548326
 2.65776951   31.69552462
 12.90268566  49.07734866
 30.86006679  52.18535694
 51.58012194  49.53779433
 57.91124991  36.76042699
 58.71702983  19.14838012
 50.88945344  6.02567852
 41.91076287  0.38521906
 38.64148309  4.21799476
 1     10
B  2
 11.43072178  20.57027093
 20.38144137  3.7589835
B  2
 20.38144137  3.7589835
 35.7009422   5.30814651
B  5
 46.35860991  38.87334496
 46.81762117  30.95540071
 46.2725453   14.68918915
 41.45292706  12.10725081
 38.64148309  4.21799476
B  2
 38.64148309  4.21799476
 35.7009422   5.30814651
C  1 1
 21.00365167  45.50889472
 1     11
"""

AREAS_WITH_SPACE_GEOMETRY = """\
VERTI:
B  5 1
 8.45818088   33.40191653
 0.60764185   25.08168713
 0.33924735   9.78320082
 10.26984373  1.06037967
 26.37351354  0.12099893
 1     1
B  7 1
 26.37351354  0.12099893
 42.61138059  4.28111363
 43.81915583  22.26354492
 35.16343331  33.26771928
 23.01858233  34.07290277
 13.62477494  34.1400014
 8.45818088   33.40191653
 1     1
C  1 1
 10.40404098  15.75497837
 1     3
C  1 1
 28.85616264  22.12934767
 1     4
B  6 1
 13.62477494  25.282983
 22.95148371  26.42365962
 28.05097915  18.90861371
 31.8085021   10.38708843
 30.26523374  2.93914115
 25.03154106  3.74432464
 1     5
B  2 1
 8.45818088   33.40191653
 13.62477494  25.282983
 1     2
B  3 1
 13.62477494  25.282983
 22.68308921  15.0168935
 25.03154106  3.74432464
 1     2
B  2 1
 25.03154106  3.74432464
 26.37351354  0.12099893
 1     2
"""

AREAS_WITH_SPACE_ATTRIBUTES = """\
cat,name
3,"Left plot"
4,"Right plot"
"""

AREAS_WITH_SPACE_ATTRIBUTE_TYPES = """\
"Integer","String"
"""


def import_data(path, areas_name, areas_with_space_in_between, env):
    gs.write_command(
        "v.in.ascii",
        input="-",
        output=areas_name,
        stdin=DATA,
        format="standard",
        env=env,
    )
    attributes = path / "test.csv"
    attributes.write_text(AREAS_WITH_SPACE_ATTRIBUTES)
    attribute_types = path / "test.csvt"
    attribute_types.write_text(AREAS_WITH_SPACE_ATTRIBUTE_TYPES)
    # Attributes need to be created first because no vector map of the same name
    # can exist when table is imported (internally using v.in.ogr and vector part
    # is deleted).
    gs.run_command(
        "db.in.ogr", input=attributes, output=areas_with_space_in_between, env=env
    )
    gs.write_command(
        "v.in.ascii",
        input="-",
        output=areas_with_space_in_between,
        stdin=AREAS_WITH_SPACE_GEOMETRY,
        format="standard",
        env=env,
    )
    # Our old cat column is now called cat_, so we need to rename it to cat,
    # but that's possible only on vector map level, so connect, rename, and
    # reconnect to create indices.
    gs.run_command(
        "v.db.connect",
        map=areas_with_space_in_between,
        table=areas_with_space_in_between,
        env=env,
    )
    gs.run_command(
        "v.db.renamecolumn",
        map=areas_with_space_in_between,
        column=("cat_", "cat"),
        env=env,
    )
    gs.run_command(
        "v.db.connect",
        map=areas_with_space_in_between,
        table=areas_with_space_in_between,
        overwrite=True,
        env=env,
    )


@pytest.fixture(scope="module")
def area_dataset(tmp_path_factory):
    """Create a session and fill mapset with data"""
    tmp_path = tmp_path_factory.mktemp("area_dataset")
    location = "test"

    areas_name = "test_areas"
    areas_with_space_in_between = "areas_with_space_in_between"

    gs.core._create_location_xy(tmp_path, location)  # pylint: disable=protected-access
    with gs.setup.init(tmp_path / location, env=os.environ.copy()) as session:
        import_data(
            path=tmp_path,
            areas_name=areas_name,
            areas_with_space_in_between=areas_with_space_in_between,
            env=session.env,
        )
        yield SimpleNamespace(
            name=areas_name,
            areas_with_space_in_between=areas_with_space_in_between,
            session=session,
        )
