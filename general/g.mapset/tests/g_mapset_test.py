import grass.script as gs


def test_list_output(simple_dataset):
    """Test g.mapset with list flag"""
    mapsets = simple_dataset.mapsets
    text = gs.read_command("g.mapset", flags="l")
    parsed_list = text.strip().split()

    assert len(parsed_list) == len(mapsets)
    for mapset in mapsets:
        assert mapset in parsed_list


def test_print_output(simple_dataset):
    """Test g.mapset with print flag"""
    text = gs.read_command("g.mapset", flags="p")
    assert text.strip() == simple_dataset.current_mapset
