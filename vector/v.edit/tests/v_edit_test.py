import textwrap
from io import StringIO

import pytest

from grass.tools import Tools


def test_create(xy_dataset_session):
    tools = Tools(session=xy_dataset_session)
    tools.v_edit(map="test", tool="create")
    assert tools.v_info(map="test", format="json")["points"] == 0


def test_create_stdin(xy_dataset_session):
    tools = Tools(session=xy_dataset_session)
    tools.v_edit(
        map="test", tool="create", input=StringIO("VERTI:\nP 1 0\n640794 214874")
    )
    assert tools.v_info(map="test", format="json")["points"] == 1


def test_create_file(xy_dataset_session, tmp_path):
    text_file = tmp_path / "point.txt"
    text_file.write_text(
        textwrap.dedent("""\
        VERTI:
        P 1 0
        640794 214874
    """)
    )
    tools = Tools(session=xy_dataset_session)
    tools.v_edit(map="test", tool="create", input=text_file)
    assert tools.v_info(map="test", format="json")["points"] == 1


def test_add_with_header(xy_dataset_session):
    tools = Tools(session=xy_dataset_session)
    tools.v_edit(
        map="test", tool="create", input=StringIO("VERTI:\nP 1 0\n640794 214874")
    )
    tools.v_edit(map="test", tool="add", input=StringIO("VERTI:\nP 1 0\n640790 214870"))
    assert tools.v_info(map="test", format="json")["points"] == 2


def test_add(xy_dataset_session):
    tools = Tools(session=xy_dataset_session)
    tools.v_edit(
        map="test", tool="create", flags="n", input=StringIO("P 1 0\n640794 214874")
    )
    tools.v_edit(
        map="test", tool="add", flags="n", input=StringIO("P 1 0\n640790 214870")
    )
    assert tools.v_info(map="test", format="json")["points"] == 2


def test_add_to_empty(xy_dataset_session):
    tools = Tools(session=xy_dataset_session)
    tools.v_edit(map="test", tool="create")
    tools.v_edit(
        map="test", tool="add", flags="n", input=StringIO("P 1 0\n640790 214870")
    )
    assert tools.v_info(map="test", format="json")["points"] == 1


def test_add_multiple(xy_dataset_session):
    tools = Tools(session=xy_dataset_session)
    tools.v_edit(
        map="test",
        tool="create",
        flags="n",
        input=StringIO("P 1 0\n640794 214874\nP 1 0\n640793 214873"),
    )
    tools.v_edit(
        map="test",
        tool="add",
        flags="n",
        input=StringIO("P 1 0\n640790 214870\nP 1 0\n640795 214875"),
    )
    tools.v_edit(
        map="test",
        tool="add",
        flags="n",
        input=StringIO("P 1 0\n640791 214871\nP 1 0\n640792 214872"),
    )
    assert tools.v_info(map="test", format="json")["points"] == 6


def test_delete_by_cats(xy_dataset_session):
    tools = Tools(session=xy_dataset_session)
    tools.v_edit(
        map="test",
        tool="create",
        flags="n",
        input=StringIO("P 1 1\n640794 214874\n1 10"),
    )
    tools.v_edit(
        map="test", tool="add", flags="n", input=StringIO("P 1 1\n640790 214870\n1 11")
    )
    tools.v_edit(map="test", tool="delete", cats=10)
    data = tools.v_category(input="test", option="print", format="json")
    assert len(data) == 1
    assert data[0]["category"] == 11
    assert data[0]["layer"] == 1


def test_delete_by_cats_reverse_selection(xy_dataset_session):
    tools = Tools(session=xy_dataset_session)
    tools.v_edit(
        map="test",
        tool="create",
        flags="n",
        input=StringIO("P 1 1\n640794 214874\n1 10"),
    )
    tools.v_edit(
        map="test", tool="add", flags="n", input=StringIO("P 1 1\n640790 214870\n1 11")
    )
    tools.v_edit(map="test", tool="delete", cats=10, flags="r")
    data = tools.v_category(input="test", option="print", format="json")
    assert len(data) == 1
    assert data[0]["category"] == 10
    assert data[0]["layer"] == 1


def test_delete_layers_and_cats(xy_dataset_session):
    tools = Tools(session=xy_dataset_session)

    tools.v_edit(
        map="test",
        tool="create",
        flags="n",
        input=StringIO(
            textwrap.dedent("""\
        P 1 1
        100 200
        20 50
        P 1 1
        100 200
        20 51
        P 1 1
        100 200
        30 50
        P 1 1
        100 200
        30 51
        P 1 3
        100 200
        30 51
        40 51
        50 100
        P 1 2
        100 200
        40 51
        40 100
    """)
        ),
    )
    assert tools.v_info(map="test", format="json")["points"] == 6
    data = tools.v_category(
        input="test", layer=[20, 30, 40, 50], option="print", format="json"
    )
    simplified_data = [
        {"layer": item["layer"], "category": item["category"]} for item in data
    ]
    results = [
        {"layer": 20, "category": 50},
        {"layer": 20, "category": 51},
        {"layer": 30, "category": 50},
        {"layer": 30, "category": 51},
        {"layer": 30, "category": 51},
        {"layer": 40, "category": 51},
        {"layer": 50, "category": 100},
        {"layer": 40, "category": 51},
        {"layer": 40, "category": 100},
    ]
    assert simplified_data == results
    assert len(data) == 9
    assert len(data) == len(results)

    # Only single layer is supported in selection.
    tools.v_edit(map="test", tool="delete", layer=30, cats=51)
    data = tools.v_category(
        input="test", layer=[20, 30, 40, 50], option="print", format="json"
    )
    simplified_data = [
        {"layer": item["layer"], "category": item["category"]} for item in data
    ]
    results = [
        {"layer": 20, "category": 50},
        {"layer": 20, "category": 51},
        {"layer": 30, "category": 50},
        {"layer": 40, "category": 51},
        {"layer": 40, "category": 100},
    ]
    assert simplified_data == results
    assert len(data) == 5
    assert len(data) == len(results)


def test_catadd(xy_dataset_session):
    tools = Tools(session=xy_dataset_session)
    tools.v_edit(
        map="test",
        tool="create",
        flags="n",
        input=StringIO("P 1 1\n640794 214874\n1 20\nP 1 1\n640790 214870\n1 21"),
    )
    # Little strange to select all by reversing selection.
    tools.v_edit(map="test", tool="catadd", cats=30, flags="r")
    data = tools.v_category(input="test", option="print", format="json")
    assert len(data) == 4
    assert data[0]["layer"] == 1
    assert data[0]["category"] == 20
    assert data[1]["layer"] == 1
    assert data[1]["category"] == 30
    assert data[2]["layer"] == 1
    assert data[2]["category"] == 21
    assert data[3]["layer"] == 1
    assert data[3]["category"] == 30


def test_catadd_layer(xy_dataset_session):
    tools = Tools(session=xy_dataset_session)
    tools.v_edit(
        map="test",
        tool="create",
        flags="n",
        input=StringIO("P 1 1\n640794 214874\n2 20\nP 1 1\n640790 214870\n2 21"),
    )
    # Little strange to select all by reversing selection.
    tools.v_edit(map="test", tool="catadd", layer=3, cats=30, flags="r")
    data = tools.v_category(input="test", layer=[2, 3], option="print", format="json")
    assert len(data) == 4
    assert data[0]["layer"] == 2
    assert data[0]["category"] == 20
    assert data[1]["layer"] == 3
    assert data[1]["category"] == 30
    assert data[2]["layer"] == 2
    assert data[2]["category"] == 21
    assert data[3]["layer"] == 3
    assert data[3]["category"] == 30


@pytest.mark.parametrize("layer", [1, 2, 10000])
def test_catdel_layer(xy_dataset_session, layer):
    tools = Tools(session=xy_dataset_session)
    tools.v_edit(
        map="test",
        tool="create",
        flags="n",
        input=StringIO(
            f"P 1 1\n640794 214874\n{layer} 20\nP 1 1\n640790 214870\n{layer} 21"
        ),
    )
    # It is a little strange to select all by reversing selection, but explicitly
    # selecting all is not supported.
    tools.v_edit(map="test", tool="catdel", layer=layer, cats=21, flags="r")
    data = tools.v_category(input="test", layer=layer, option="print", format="json")
    assert len(data) == 1
    assert data[0]["layer"] == layer
    assert data[0]["category"] == 20


@pytest.mark.parametrize("layer", [2, 10000])
def test_select_layer(xy_dataset_session, layer):
    tools = Tools(session=xy_dataset_session)
    tools.v_edit(
        map="test",
        tool="create",
        flags="n",
        input=StringIO(
            textwrap.dedent(f"""\
            P 1 1
            640794 214874
            1 20
            P 1 1
            640790 214870
            {layer} 21
            P 1 1
            640790 214870
            {layer} 21
            P 1 1
            640790 214870
            {layer} 21
            """)
        ),
    )
    # Some selection parameter is required, so another ways of selecting all is to
    # give a wide enough range of category values (not ideal).
    output = tools.v_edit(
        map="test", tool="select", layer=layer, cats="1-99"
    ).comma_items
    assert len(output) == 3
