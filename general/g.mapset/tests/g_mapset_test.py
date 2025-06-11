import grass.script as gs


def test_plain_list_output(simple_dataset):
    """Test g.mapset with list flag and plain format"""
    mapsets = simple_dataset.mapsets
    text = gs.read_command("g.mapset", format="plain", flags="l")
    parsed_list = text.strip().split()

    assert len(parsed_list) == len(mapsets)
    for mapset in mapsets:
        assert mapset in parsed_list


def test_plain_print_output(simple_dataset):
    """Test g.mapset with print flag and plain format"""
    text = gs.read_command("g.mapset", format="plain", flags="p")
    assert text.strip() == simple_dataset.current_mapset


def test_json_list_ouput(simple_dataset):
    """Test g.mapset with list flag and JSON format"""
    mapsets = simple_dataset.mapsets
    data = gs.parse_command("g.mapset", format="json", flags="l")
    assert list(data.keys()) == ["project", "mapsets"]
    assert isinstance(data["mapsets"], list)
    assert len(data["mapsets"]) == len(mapsets)
    for mapset in mapsets:
        assert mapset in data["mapsets"]


def test_json_print_ouput(simple_dataset):
    """Test g.mapset with print flag and JSON format"""
    data = gs.parse_command("g.mapset", format="json", flags="p")
    assert list(data.keys()) == ["project", "mapset"]
    assert data["mapset"] == simple_dataset.current_mapset
    assert data["project"] == simple_dataset.project
