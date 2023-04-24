import pytest

import grass.script as gs
from grass.experimental.tools import Tools


def test_key_value_parser(xy_dataset_session):
    tools = Tools(session=xy_dataset_session)
    assert tools.g_region(flags="g").keyval["nsres"] == 1


# def test_json_parser(xy_dataset_session):
#     print(
#         tools.v_db_univar(map="bridges", column="YEAR_BUILT", format="json").json[
#             "statistics"
#         ]["mean"]
#     )

# def test_direct_overwrite(xy_dataset_session):
#     tools = Tools(session=xy_dataset_session)
#     tools.r_slope_aspect(elevation="elevation", slope="slope")
#     tools.r_slope_aspect(elevation="elevation", slope="slope", overwrite=True)


def test_stdin(xy_dataset_session):
    tools = Tools(session=xy_dataset_session)
    tools.feed_input_to("13.45,29.96,200").v_in_ascii(
        input="-", output="point", separator=","
    )


def test_raises(xy_dataset_session):
    tools = Tools(session=xy_dataset_session)
    with pytest.raises(gs.CalledModuleError, match="xstandard"):
        tools.feed_input_to("13.45,29.96,200").v_in_ascii(
            input="-",
            output="point",
            format="xstandard",
        )
