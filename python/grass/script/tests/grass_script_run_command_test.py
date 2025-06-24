"""Tests of run_command family of functions"""

import pytest


import grass.script as gs
from grass.exceptions import CalledModuleError


def test_run_command(session_2x2):
    """Check run_command runs again with overwrite parameter"""
    gs.run_command("r.random.surface", output="surface", seed=42, env=session_2x2.env)
    gs.run_command(
        "r.random.surface",
        output="surface",
        seed=42,
        overwrite=True,
        env=session_2x2.env,
    )


def test_run_command_status(session_2x2):
    """Check that run_command gives returncode (aka status)"""
    assert (
        gs.run_command("r.mask.status", flags="t", env=session_2x2.env, errors="status")
        == 1
    )


def test_run_command_ignore_default(session_2x2):
    """Check that run_command raises by default"""
    with pytest.raises(CalledModuleError, match=r"r\.slope\.aspect"):
        gs.run_command(
            "r.slope.aspect",
            elevation="does_not_exist",
            slope="slope",
            env=session_2x2.env,
        )


def test_run_command_ignore_raise(session_2x2):
    """Check that run_command raises when set to raise"""
    with pytest.raises(CalledModuleError, match=r"r\.slope\.aspect"):
        gs.run_command(
            "r.slope.aspect",
            elevation="does_not_exist",
            slope="slope",
            env=session_2x2.env,
            errors="raise",
        )


def test_run_command_ignore(session_2x2):
    """Check that run_command ignores errors"""
    gs.run_command(
        "r.slope.aspect",
        elevation="does_not_exist",
        slope="slope",
        env=session_2x2.env,
        errors="ignore",
    )


def test_run_command_fatal(session_2x2):
    """Check that run_command results in a fatal error (which calls in sys.exit)"""
    with pytest.raises(SystemExit):
        gs.run_command(
            "r.slope.aspect",
            elevation="does_not_exist",
            slope="slope",
            env=session_2x2.env,
            errors="fatal",
        )


def test_run_command_exit(session_2x2):
    """Check that run_command calls sys.exit"""
    with pytest.raises(SystemExit):
        gs.run_command(
            "r.slope.aspect",
            elevation="does_not_exist",
            slope="slope",
            env=session_2x2.env,
            errors="exit",
        )


def test_read_command_flag(session_2x2):
    """Check a single flag character"""
    assert "north" in gs.read_command(
        "g.region", flags="p", format="json", env=session_2x2.env
    )


def test_read_command_flag_with_dash(session_2x2):
    """Check flag with dash"""
    assert "north" in gs.read_command(
        "g.region", flags="-p", format="json", env=session_2x2.env
    )


def test_read_command_multiple_flags(session_2x2):
    """Check multiple flag characters included a number"""
    assert "north" in gs.read_command(
        "g.region", flags="p3u", format="json", env=session_2x2.env
    )


def test_read_command_multiple_flags_with_dash(session_2x2):
    """Check multiple flag characters included a number with dash"""
    assert "north" in gs.read_command(
        "g.region", flags="-p3u", format="json", env=session_2x2.env
    )


def test_read_command_multiple_flags_with_multiple_dashes(session_2x2):
    with pytest.raises(CalledModuleError, match=r"g\.region"):
        gs.read_command("g.region", flags="-p-u", format="json", env=session_2x2.env)


def test_read_command_multiple_flags_with_multiple_dashes_and_spaces(session_2x2):
    with pytest.raises(CalledModuleError, match=r"g\.region"):
        gs.read_command("g.region", flags="-p -u", format="json", env=session_2x2.env)


def test_read_command_number_as_a_string_flag(session_2x2):
    """Number is a character just as any letter is"""
    assert "top" in gs.read_command(
        "g.region", flags="-3", format="json", env=session_2x2.env
    )


def test_read_command_number_as_a_int_flag(session_2x2):
    """Integer is supported, because we support ints and floats for options"""
    assert "top" in gs.read_command(
        "g.region", flags=3, format="json", env=session_2x2.env
    )


def test_read_command_number_as_a_float_flag(session_2x2):
    """Float is not supported as a flag even if its integer part is a valid flag"""
    with pytest.raises(CalledModuleError, match=r"g\.region"):
        gs.read_command("g.region", flags=3.0, env=session_2x2.env)


def test_read_command_number_as_a_negative_int_flag(session_2x2):
    """
    Negative number works only as as a side effect of other processing,
    but we test for it anyway since it is expected from the current implementation.
    However, it is not how users should use it.
    """
    assert "top" in gs.read_command("g.region", flags=-3, env=session_2x2.env)


def test_parse_command_key_value(session_2x2):
    """Check parse_command parses shell-script style key-value pairs

    Values are always strings even for numbers.
    """
    assert (
        gs.parse_command("g.region", flags="g", env=session_2x2.env)["nsres"] == "0.5"
    )


def test_parse_command_json(session_2x2):
    """Check parse_command parses JSON output"""
    assert (
        gs.parse_command("g.region", flags="g", format="json", env=session_2x2.env)[
            "region"
        ]["ns-res"]
        == 0.5
    )
