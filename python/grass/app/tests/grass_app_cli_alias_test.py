from grass.app.cli import resolve_tool_name


def test_resolve_tool_name_aliases():
    assert resolve_tool_name("list") == "g.list"
    assert resolve_tool_name("slope") == "r.slope.aspect"
    assert resolve_tool_name("r.info") == "r.info"
