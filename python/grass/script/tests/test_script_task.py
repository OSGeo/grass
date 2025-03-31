from grass.script.task import grassTask


def test_mapcalc_simple_e_name():
    gt = grassTask("r.mapcalc.simple")
    assert gt.get_param("e")["name"] == "e"


def test_mapcalc_simple_expession_name():
    gt = grassTask("r.mapcalc.simple")
    assert gt.get_param("expression")["name"] == "expression"
