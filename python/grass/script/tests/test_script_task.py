from grass.script.task import grassTask as gtask


def test_mapcalc_simple_e_name():
    gt = gtask("r.mapcalc.simple")
    assert gt.get_param("e")["name"] == "e"


def test_mapcalc_simple_expession_name():
    gt = gtask("r.mapcalc.simple")
    assert gt.get_param("expression")["name"] == "expression"
