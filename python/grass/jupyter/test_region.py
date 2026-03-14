from unittest.mock import patch

from grass.jupyter.utils import get_region


@patch("grass.jupyter.utils.Tools")
def test_get_region_returns_dict(mock_tools_class):
    fake_map_data = {"n": 50.0, "s": 10.0, "e": 40.0, "w": 20.0}

    mock_instance = mock_tools_class.return_value
    mock_instance.g_region.return_value.json = fake_map_data

    result = get_region()

    assert isinstance(result, dict)
    assert result == fake_map_data
    assert result["n"] == 50.0