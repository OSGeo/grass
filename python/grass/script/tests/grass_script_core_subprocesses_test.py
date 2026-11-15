"""Tests for grass.script.core basic subprocess functionality"""

from __future__ import annotations

import pytest

import grass.script as gs
from grass.exceptions import CalledModuleError


def test_handle_errors_handler_default():
    """Test handler which raises by default"""
    with pytest.raises(CalledModuleError):
        gs.handle_errors(1, None, ["g.region"], {})


def test_handle_errors_handler_raise():
    """Test explicit raise handler"""
    with pytest.raises(CalledModuleError):
        gs.handle_errors(1, None, ["g.region"], {}, handler="raise")


@pytest.mark.parametrize("returncode", [0, 1, 2, 3])
def test_handle_errors_handler_status(returncode):
    """Test the status handler which returns the return code"""
    result = gs.handle_errors(
        returncode,
        "mock output",
        ["g.region"],
        {},
        handler="status",
        stderr="mock message",
    )
    assert result == returncode


@pytest.mark.parametrize("returncode", [0, 1])
def test_handle_errors_handler_ignore(returncode):
    """Test the ignore handler which returns the result regardless of the return code"""
    output = "mock output"
    result = gs.handle_errors(
        returncode,
        result=output,
        args=["g.region"],
        kwargs={},
        handler="ignore",
        stderr="mock message",
    )
    assert result == output


def test_handle_errors_handler_exit():
    """Test the plain exit handler which calls sys.exit"""
    with pytest.raises(SystemExit):
        gs.handle_errors(1, None, ["g.region"], {}, handler="exit")


def test_handle_errors_handler_exit_with_stderr(capsys: pytest.CaptureFixture[str]):
    """Test that stderr argument is printed to stderr with the exit handler"""
    stderr = "mock message"
    with pytest.raises(SystemExit):
        gs.handle_errors(1, None, ["g.region"], {}, handler="exit", stderr=stderr)
    captured = capsys.readouterr()
    assert captured.err == stderr + "\n"


def test_handle_errors_handler_fatal(empty_session):
    """Test the default behavior of fatal which is to eventually call exit"""
    with pytest.raises(SystemExit):
        gs.handle_errors(
            1, None, ["g.region"], {}, handler="fatal", env=empty_session.env
        )


def test_handle_errors_handler_errors_from_kwargs_with_raise():
    """Test getting the handler form errors from kwargs with raise"""
    with pytest.raises(CalledModuleError):
        gs.handle_errors(1, None, ["g.region"], {"errors": "raise"})


@pytest.mark.parametrize("returncode", [0, 1])
def test_handle_errors_handler_errors_from_kwargs_with_status(returncode):
    """Test getting the handler form errors from kwargs with status"""
    result = gs.handle_errors(
        returncode,
        "mock output",
        ["g.region"],
        {},
        handler="status",
        stderr="mock message",
    )
    assert result == returncode


def test_handle_errors_handler_raise_with_stderr():
    """Test that stderr is passed to the exception"""
    stderr = "mock message"
    with pytest.raises(CalledModuleError, match=stderr):
        gs.handle_errors(1, None, ["g.region"], {}, handler="raise", stderr=stderr)


@pytest.mark.parametrize("stderr", [None, "", "mock message"])
def test_handle_errors_tool_name_in_exception(stderr):
    """Test that a tool name is passed to the exception regardless of stderr"""
    with pytest.raises(CalledModuleError, match=r"g.region"):
        gs.handle_errors(1, None, ["g.region"], {}, handler="raise", stderr=stderr)
