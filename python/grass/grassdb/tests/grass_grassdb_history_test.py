"""Tests of grass.grassdb.history"""

import json
import os
from datetime import datetime
from pathlib import Path

import pytest

import grass.script as gs
from grass.grassdb import history


def make_entry(command, timestamp="2025-01-01T00:00:00", status="success"):
    """Create a history entry as stored in the JSON history file"""
    return {
        "command": command,
        "command_info": {"timestamp": timestamp, "status": status},
    }


@pytest.fixture
def json_history(tmp_path):
    """Path to a JSON history file with three entries"""
    path = tmp_path / ".wxgui_history.json"
    entries = [
        make_entry("g.region -p", "2025-01-01T00:00:01"),
        make_entry("r.info map=a", "2025-01-01T00:00:02"),
        make_entry("r.info map=b", "2025-01-01T00:00:03"),
    ]
    path.write_text(json.dumps(entries), encoding="utf-8")
    return path


@pytest.fixture
def text_history(tmp_path):
    """Path to a plain text history file with three commands"""
    path = tmp_path / ".wxgui_history"
    path.write_text("g.region -p\nr.info map=a\nr.info map=b\n", encoding="utf-8")
    return path


@pytest.fixture
def session(tmp_path, monkeypatch):
    """Session in a new XY project, also mirrored into os.environ

    Some functions in grass.grassdb.history read the session from os.environ.
    """
    project = tmp_path / "project"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        for name, value in session.env.items():
            monkeypatch.setenv(name, value)
        yield session


def test_status_values():
    """Status values are the strings stored in the history files"""
    assert {status.name: status.value for status in history.Status} == {
        "ABORTED": "aborted",
        "FAILED": "failed",
        "RUNNING": "running",
        "SUCCESS": "success",
        "UNKNOWN": "unknown",
    }


@pytest.mark.parametrize(
    ("name", "extension"),
    [
        (".wxgui_history", ""),
        (".wxgui_history.json", ".json"),
        ("history.txt", ".txt"),
    ],
)
@pytest.mark.parametrize("path_type", [str, Path])
def test_get_history_file_extension(tmp_path, name, extension, path_type):
    """Extension is returned for both str and Path"""
    assert history.get_history_file_extension(path_type(tmp_path / name)) == extension


def test_ensure_history_file_creates_empty_file(tmp_path):
    """A missing file is created and empty"""
    path = tmp_path / ".wxgui_history.json"
    history.ensure_history_file(path)
    assert path.exists()
    assert path.read_text(encoding="utf-8") == ""


def test_ensure_history_file_keeps_content(json_history):
    """An existing file is not modified"""
    content = json_history.read_text(encoding="utf-8")
    history.ensure_history_file(json_history)
    assert json_history.read_text(encoding="utf-8") == content


def test_ensure_history_file_unwritable_location(tmp_path):
    """A missing parent directory is reported as OSError mentioning the path"""
    path = tmp_path / "missing_directory" / ".wxgui_history"
    with pytest.raises(OSError, match=r"^Unable to create history file") as info:
        history.ensure_history_file(path)
    assert str(path) in str(info.value)
    assert isinstance(info.value.__cause__, OSError)


def test_read_plain_text(text_history):
    """Plain text lines become entries without command info"""
    assert history.read(text_history) == [
        {"command": "g.region -p", "command_info": None},
        {"command": "r.info map=a", "command_info": None},
        {"command": "r.info map=b", "command_info": None},
    ]


def test_read_plain_text_strips_whitespace(tmp_path):
    """Surrounding whitespace is removed and blank lines are kept as empty commands"""
    path = tmp_path / ".wxgui_history"
    path.write_text("  g.region -p  \n\n", encoding="utf-8")
    assert [entry["command"] for entry in history.read(path)] == ["g.region -p", ""]


def test_read_plain_text_invalid_utf8(tmp_path):
    """Undecodable bytes are replaced instead of raising"""
    path = tmp_path / ".wxgui_history"
    path.write_bytes(b"g.region \xff\n")
    assert history.read(path) == [{"command": "g.region �", "command_info": None}]


def test_read_plain_text_missing_file(tmp_path):
    """Missing file is reported as OSError mentioning the path"""
    path = tmp_path / ".wxgui_history"
    with pytest.raises(
        OSError, match=r"^Unable to read from plain text history file"
    ) as info:
        history.read(path)
    assert str(path) in str(info.value)


def test_read_json(json_history):
    """JSON entries are returned with their command info"""
    entries = history.read(json_history)
    assert [entry["command"] for entry in entries] == [
        "g.region -p",
        "r.info map=a",
        "r.info map=b",
    ]
    assert entries[1]["command_info"] == {
        "timestamp": "2025-01-01T00:00:02",
        "status": "success",
    }


def test_read_json_empty_file(tmp_path):
    """Empty file (as created by ensure_history_file) has no entries"""
    path = tmp_path / ".wxgui_history.json"
    history.ensure_history_file(path)
    assert history.read(path) == []


def test_read_json_empty_list(tmp_path):
    """Empty JSON list has no entries"""
    path = tmp_path / ".wxgui_history.json"
    path.write_text("[]", encoding="utf-8")
    assert history.read(path) == []


def test_read_json_drops_extra_keys(tmp_path):
    """Only command and command_info are kept from each entry"""
    path = tmp_path / ".wxgui_history.json"
    entry = {"command": "g.region -p", "command_info": {}, "extra": 1}
    path.write_text(json.dumps([entry]), encoding="utf-8")
    assert history.read(path) == [{"command": "g.region -p", "command_info": {}}]


def test_read_json_invalid_content(tmp_path):
    """Invalid JSON is reported as ValueError mentioning the path"""
    path = tmp_path / ".wxgui_history.json"
    path.write_text("[{", encoding="utf-8")
    with pytest.raises(
        ValueError, match=r"^Error decoding content of JSON history file"
    ) as info:
        history.read(path)
    assert str(path) in str(info.value)
    assert isinstance(info.value.__cause__, ValueError)


def test_read_json_missing_file(tmp_path):
    """Missing file is reported as OSError mentioning the path"""
    path = tmp_path / ".wxgui_history.json"
    with pytest.raises(
        OSError, match=r"^Unable to read from JSON history file"
    ) as info:
        history.read(path)
    assert str(path) in str(info.value)


def test_read_json_entry_without_command_info(tmp_path):
    """An entry lacking a required key is not silently accepted"""
    path = tmp_path / ".wxgui_history.json"
    path.write_text(json.dumps([{"command": "g.region -p"}]), encoding="utf-8")
    with pytest.raises(KeyError):
        history.read(path)


def test_filter_finds_entry_by_command_and_timestamp(json_history):
    """Index of the entry matching both command and timestamp is returned"""
    entries = history.read(json_history)
    assert history.filter(entries, "r.info map=a", "2025-01-01T00:00:02") == 1
    assert history.filter(entries, "r.info map=b", "2025-01-01T00:00:03") == 2


def test_filter_requires_both_command_and_timestamp(json_history):
    """Matching only one of command or timestamp is not a match"""
    entries = history.read(json_history)
    assert history.filter(entries, "r.info map=a", "2025-01-01T00:00:03") is None
    assert history.filter(entries, "r.info map=c", "2025-01-01T00:00:02") is None


def test_filter_returns_first_match():
    """With duplicates the first index is returned"""
    entry = make_entry("g.region -p", "2025-01-01T00:00:00")
    assert (
        history.filter(
            [make_entry("x"), entry, entry],
            "g.region -p",
            entry["command_info"]["timestamp"],
        )
        == 1
    )


def test_filter_skips_entries_without_command_info():
    """Entries from plain text history can never match"""
    entries = [{"command": "g.region -p", "command_info": None}]
    assert history.filter(entries, "g.region -p", None) is None


def test_filter_empty():
    """Empty history has no match"""
    assert history.filter([], "g.region -p", "2025-01-01T00:00:00") is None


def test_remove_entry_plain_text(text_history):
    """Line at the index is removed and others keep their order"""
    history.remove_entry(text_history, 1)
    assert text_history.read_text(encoding="utf-8") == "g.region -p\nr.info map=b\n"


@pytest.mark.parametrize("index", [3, 10, -1])
def test_remove_entry_plain_text_out_of_range(text_history, index):
    """Plain text history stays unchanged for an index with no line"""
    content = text_history.read_text(encoding="utf-8")
    history.remove_entry(text_history, index)
    assert text_history.read_text(encoding="utf-8") == content


def test_remove_entry_plain_text_missing_file(tmp_path):
    """Missing file is reported as OSError mentioning the path"""
    path = tmp_path / ".wxgui_history"
    with pytest.raises(
        OSError, match=r"^Unable to remove entry from plain text history file"
    ) as info:
        history.remove_entry(path, 0)
    assert str(path) in str(info.value)


@pytest.mark.parametrize("index", [0, 1, 2])
def test_remove_entry_json(json_history, index):
    """Entry at the index is removed and others keep their order"""
    before = history.read(json_history)
    history.remove_entry(json_history, index)
    assert history.read(json_history) == before[:index] + before[index + 1 :]


@pytest.mark.parametrize("index", [3, 100, -1])
def test_remove_entry_json_out_of_range(json_history, index):
    """JSON history keeps all entries for an index which is out of range"""
    before = history.read(json_history)
    history.remove_entry(json_history, index)
    assert history.read(json_history) == before


def test_remove_entry_json_uses_indentation(json_history):
    """The file is rewritten as indented JSON"""
    history.remove_entry(json_history, 0)
    assert json_history.read_text(encoding="utf-8").startswith('[\n  {\n    "command"')


def test_remove_entry_json_shorter_content_has_no_leftover(json_history):
    """The file is truncated so no trailing text of the old content remains"""
    history.remove_entry(json_history, 0)
    json.loads(json_history.read_text(encoding="utf-8"))


def test_remove_entry_json_invalid_content(tmp_path):
    """Invalid JSON is reported as ValueError mentioning the path"""
    path = tmp_path / ".wxgui_history.json"
    path.write_text("[{", encoding="utf-8")
    with pytest.raises(
        ValueError, match=r"^Error decoding content of JSON history file"
    ) as info:
        history.remove_entry(path, 0)
    assert str(path) in str(info.value)


def test_remove_entry_json_missing_file(tmp_path):
    """Missing file is reported as OSError mentioning the path"""
    path = tmp_path / ".wxgui_history.json"
    with pytest.raises(
        OSError, match=r"^Unable to remove entry from JSON history file"
    ) as info:
        history.remove_entry(path, 0)
    assert str(path) in str(info.value)


def test_convert_plain_text_to_json(text_history):
    """JSON file is created next to the text file with the same entries"""
    history.convert_plain_text_to_JSON(text_history)
    json_path = text_history.parent / (text_history.name + ".json")
    assert json_path.exists()
    assert history.read(json_path) == history.read(text_history)
    assert text_history.exists()


def test_convert_plain_text_to_json_uses_indentation(text_history):
    """The JSON file is indented"""
    history.convert_plain_text_to_JSON(text_history)
    content = (text_history.parent / ".wxgui_history.json").read_text(encoding="utf-8")
    assert content.startswith('[\n  {\n    "command": "g.region -p"')


def test_convert_plain_text_to_json_overwrites_existing(text_history):
    """An existing JSON file is replaced"""
    json_path = text_history.parent / (text_history.name + ".json")
    json_path.write_text("[]", encoding="utf-8")
    history.convert_plain_text_to_JSON(text_history)
    assert len(history.read(json_path)) == 3


def test_convert_plain_text_to_json_missing_file(tmp_path):
    """Missing file is reported as OSError mentioning the path"""
    path = tmp_path / ".wxgui_history"
    with pytest.raises(
        OSError, match=r"^Unable to convert plain text history file"
    ) as info:
        history.convert_plain_text_to_JSON(path)
    assert str(path) in str(info.value)


def test_add_entry_to_new_file(tmp_path):
    """Entry is added to an empty history file"""
    path = tmp_path / ".wxgui_history.json"
    history.ensure_history_file(path)
    entry = make_entry("g.region -p")
    history.add_entry(path, entry)
    assert history.read(path) == [entry]


def test_add_entry_to_missing_file(tmp_path):
    """The file is created when it does not exist"""
    path = tmp_path / ".wxgui_history.json"
    entry = make_entry("g.region -p")
    history.add_entry(path, entry)
    assert history.read(path) == [entry]


def test_add_entry_appends(json_history):
    """Entry is added as the last one"""
    before = history.read(json_history)
    entry = make_entry("r.mask -r")
    history.add_entry(json_history, entry)
    assert history.read(json_history) == [*before, entry]


def test_add_entry_unserializable(json_history):
    """Entry which is not JSON serializable raises TypeError, history is kept"""
    before = json_history.read_text(encoding="utf-8")
    with pytest.raises(TypeError):
        history.add_entry(json_history, {"command": "x", "command_info": {1, 2}})
    assert json_history.read_text(encoding="utf-8") == before


def test_update_entry_unserializable(json_history):
    """Command info which is not JSON serializable raises TypeError, history is kept"""
    before = json_history.read_text(encoding="utf-8")
    with pytest.raises(TypeError):
        history.update_entry(json_history, {"status": {1, 2}})
    assert json_history.read_text(encoding="utf-8") == before


def test_add_entry_requires_json(text_history):
    """Plain text history file does not support adding entries"""
    content = text_history.read_text(encoding="utf-8")
    with pytest.raises(
        ValueError, match=r"^Adding entries is supported only for JSON format"
    ):
        history.add_entry(text_history, make_entry("g.region -p"))
    assert text_history.read_text(encoding="utf-8") == content


def test_add_entry_to_corrupted_file(tmp_path):
    """Content which is not valid JSON is replaced by the new entry"""
    path = tmp_path / ".wxgui_history.json"
    path.write_text("[{", encoding="utf-8")
    entry = make_entry("g.region -p")
    history.add_entry(path, entry)
    assert history.read(path) == [entry]


def test_update_last_entry_by_default(json_history):
    """Command info of the last entry is updated when no index is given"""
    before = history.read(json_history)
    history.update_entry(json_history, {"status": "failed", "runtime": 3})
    after = history.read(json_history)
    assert after[:2] == before[:2]
    assert after[2]["command"] == before[2]["command"]
    assert after[2]["command_info"] == {
        "timestamp": "2025-01-01T00:00:03",
        "status": "failed",
        "runtime": 3,
    }


@pytest.mark.parametrize("index", [0, 1, 2])
def test_update_entry_by_index(json_history, index):
    """Only command info at the index is updated"""
    before = history.read(json_history)
    history.update_entry(json_history, {"status": "failed"}, index=index)
    after = history.read(json_history)
    for i, (old, new) in enumerate(zip(before, after, strict=True)):
        assert new["command"] == old["command"]
        if i == index:
            assert new["command_info"]["status"] == "failed"
            assert new["command_info"]["timestamp"] == old["command_info"]["timestamp"]
        else:
            assert new == old


def test_update_entry_first_with_index_zero(json_history):
    """Index zero is an index, not a request for the last entry"""
    history.update_entry(json_history, {"status": "failed"}, index=0)
    statuses = [entry["command_info"]["status"] for entry in history.read(json_history)]
    assert statuses == ["failed", "success", "success"]


@pytest.mark.parametrize("index", [3, 100, -1])
def test_update_entry_out_of_range(json_history, index):
    """History stays unchanged for an index which is out of range"""
    before = json_history.read_text(encoding="utf-8")
    history.update_entry(json_history, {"status": "failed"}, index=index)
    assert json_history.read_text(encoding="utf-8") == before


def test_update_entry_empty_history(tmp_path):
    """Empty history list has no last entry to update"""
    path = tmp_path / ".wxgui_history.json"
    path.write_text("[]", encoding="utf-8")
    history.update_entry(path, {"status": "failed"})
    assert history.read(path) == []


def test_update_entry_requires_json(text_history):
    """Plain text history file does not support updating entries"""
    with pytest.raises(
        ValueError, match=r"^Updating entries is supported only for JSON format"
    ):
        history.update_entry(text_history, {"status": "failed"})


def test_update_entry_invalid_content(tmp_path):
    """Invalid JSON is reported as ValueError mentioning the path"""
    path = tmp_path / ".wxgui_history.json"
    path.write_text("[{", encoding="utf-8")
    with pytest.raises(
        ValueError, match=r"^Error decoding content of JSON history file"
    ) as info:
        history.update_entry(path, {"status": "failed"})
    assert str(path) in str(info.value)


def test_update_entry_missing_file(tmp_path):
    """Missing file is reported as OSError mentioning the path"""
    path = tmp_path / ".wxgui_history.json"
    with pytest.raises(
        OSError, match=r"^Unable to update entry in JSON history file"
    ) as info:
        history.update_entry(path, {"status": "failed"})
    assert str(path) in str(info.value)


def test_copy(json_history, tmp_path):
    """Content is copied and the original is kept"""
    target = tmp_path / "copy.json"
    assert history.copy(json_history, target) is True
    assert target.read_bytes() == json_history.read_bytes()
    assert json_history.exists()


def test_copy_overwrites_target(json_history, tmp_path):
    """Existing target is replaced"""
    target = tmp_path / "copy.json"
    target.write_text("old", encoding="utf-8")
    history.copy(json_history, target)
    assert target.read_bytes() == json_history.read_bytes()


def test_copy_missing_source(tmp_path):
    """Missing source is reported as OSError mentioning both paths"""
    source = tmp_path / "missing"
    target = tmp_path / "copy"
    with pytest.raises(OSError, match=r"^Unable to copy history file") as info:
        history.copy(source, target)
    assert str(source) in str(info.value)
    assert str(target) in str(info.value)
    assert not target.exists()


def test_current_mapset_history_path_default_is_json(session):
    """JSON path in the current mapset is returned when no file exists"""
    path = history.get_current_mapset_gui_history_path()
    env = gs.gisenv(env=session.env)
    expected = Path(env["GISDBASE"]) / env["LOCATION_NAME"] / env["MAPSET"]
    assert path == expected / ".wxgui_history.json"
    assert not path.exists()


def test_current_mapset_history_path_plain_text(session):
    """Plain text file is used only when it exists and the JSON file does not"""
    env = gs.gisenv(env=session.env)
    mapset = Path(env["GISDBASE"]) / env["LOCATION_NAME"] / env["MAPSET"]
    txt = mapset / ".wxgui_history"
    txt.write_text("g.region -p\n", encoding="utf-8")
    assert history.get_current_mapset_gui_history_path() == txt
    (mapset / ".wxgui_history.json").write_text("[]", encoding="utf-8")
    assert (
        history.get_current_mapset_gui_history_path() == mapset / ".wxgui_history.json"
    )


def test_initial_command_info(session):
    """Info has timestamp, masks, region and running status"""
    before = datetime.now()
    info = history.get_initial_command_info(session.env)
    after = datetime.now()
    assert before <= datetime.fromisoformat(info["timestamp"]) <= after
    assert info["mask2d"] is False
    assert info["mask3d"] is False
    assert info["mask3d_name"] == "RASTER3D_MASK@PERMANENT"
    assert info["status"] == "running"
    assert info["region"] == gs.parse_command(
        "g.region", flags="p", format="json", env=session.env
    )


def test_initial_command_info_masks(session):
    """Info reflects 2D and 3D mask presence"""
    gs.run_command("g.region", rows=2, cols=2, env=session.env)
    gs.mapcalc("area = 1", env=session.env)
    gs.run_command("r.mask", raster="area", env=session.env)
    info = history.get_initial_command_info(session.env)
    assert info["mask2d"] is True
    assert info["mask2d_name"] == "MASK@PERMANENT"
    assert info["mask3d"] is False

    gs.run_command("g.region", t=1, b=0, res3=1, env=session.env)
    gs.run_command("r3.mapcalc", expression="vol = 1", env=session.env)
    gs.run_command("r3.mask", map="vol", env=session.env)
    info = history.get_initial_command_info(session.env)
    assert info["mask3d"] is True
    assert info["mask3d_name"] == "RASTER3D_MASK@PERMANENT"


def test_read_json_invalid_utf8(tmp_path):
    """Undecodable bytes are replaced instead of raising"""
    path = tmp_path / ".wxgui_history.json"
    path.write_bytes(b'[{"command": "g.region \xff", "command_info": null}]')
    assert history.read(path) == [{"command": "g.region �", "command_info": None}]


def test_add_entry_uses_indentation(tmp_path):
    """The file is written as JSON indented by two spaces"""
    path = tmp_path / ".wxgui_history.json"
    history.add_entry(path, {"command": "g.region -p", "command_info": None})
    assert path.read_text(encoding="utf-8") == (
        '[\n  {\n    "command": "g.region -p",\n    "command_info": null\n  }\n]'
    )


def test_update_entry_uses_indentation(tmp_path):
    """The file is rewritten as JSON indented by two spaces"""
    path = tmp_path / ".wxgui_history.json"
    path.write_text(
        '[{"command": "g.region -p", "command_info": {"status": "running"}}]',
        encoding="utf-8",
    )
    history.update_entry(path, {"status": "success"})
    assert path.read_text(encoding="utf-8") == (
        '[\n  {\n    "command": "g.region -p",\n'
        '    "command_info": {\n      "status": "success"\n    }\n  }\n]'
    )


def test_add_entry_unwritable_path(tmp_path):
    """A path which cannot be written is reported as OSError mentioning the path"""
    path = tmp_path / ".wxgui_history.json"
    path.mkdir()
    with pytest.raises(
        OSError, match=r"^Unable to add entry to JSON history file"
    ) as info:
        history.add_entry(path, make_entry("g.region -p"))
    assert str(path) in str(info.value)


def test_add_entry_circular_reference(json_history):
    """An entry which JSON cannot encode is reported as ValueError and file is kept"""
    entry = make_entry("g.region -p")
    entry["self"] = entry
    content = json_history.read_text(encoding="utf-8")
    with pytest.raises(
        ValueError, match=r"^Error decoding content of JSON history file"
    ) as info:
        history.add_entry(json_history, entry)
    assert str(json_history) in str(info.value)
    assert json_history.read_text(encoding="utf-8") == content
