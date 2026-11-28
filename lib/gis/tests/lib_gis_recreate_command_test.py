# SPDX-FileCopyrightText: 2026 GRASS Development Team
# SPDX-License-Identifier: GPL-2.0-or-later

"""Tests of G_recreate_command(), the command line saved to map history"""

import subprocess
import sys

import pytest

# The parser keeps global state and the program name, so each case runs in a
# separate process in the session. Arguments: program name, then tool arguments.
SCRIPT = """
import sys
from ctypes import POINTER, c_char_p

from grass.lib.ctypes_preamble import String
from grass.lib.gis import (
    G_define_flag, G_define_module, G_define_option, G_gisinit, G_parser,
    G_recreate_command, G_set_program_name, TYPE_STRING,
)

G_gisinit(sys.argv[1].encode())
# G_gisinit() keeps the name already set when the library is loaded.
G_set_program_name(sys.argv[1].encode())
# The fields hold pointers, so the bytes must stay alive as long as the parser.
DESCRIPTION = b"test"
KEY = b"text"
OTHER_KEY = b"other"
G_define_module().contents.description = String(DESCRIPTION)
option = G_define_option().contents
option.key = String(KEY)
option.type = TYPE_STRING
option.multiple = 1
option.description = String(DESCRIPTION)
other = G_define_option().contents
other.key = String(OTHER_KEY)
other.type = TYPE_STRING
other.description = String(DESCRIPTION)
for key in (b"a", b"b"):
    flag = G_define_flag().contents
    flag.key = key
    flag.description = String(DESCRIPTION)
args = [sys.argv[1], *sys.argv[2:]]
argv = (c_char_p * len(args))(*[a.encode() for a in args])
assert G_parser(len(args), argv) == 0
sys.stdout.write(G_recreate_command().data.decode())
"""


def recreate(session, *arguments, program="testmod"):
    return subprocess.run(
        [sys.executable, "-c", SCRIPT, program, *arguments],
        capture_output=True,
        text=True,
        env=session.env,
        check=True,
    ).stdout


QUOTES_NOT_ESCAPED = pytest.mark.xfail(
    strict=True,
    reason="G_recreate_command() does not escape double quotes in values",
)


@pytest.mark.parametrize(
    ("arguments", "expected"),
    [
        (["text=abc"], 'testmod text="abc"'),
        (["text=a,b"], 'testmod text="a,b"'),
        (["text=abc", "other=def"], 'testmod text="abc" other="def"'),
        (["-a"], "testmod -a"),
        (["-a", "-b"], "testmod -a -b"),
        (["-a", "text=abc"], 'testmod -a text="abc"'),
        (["--overwrite"], "testmod --overwrite"),
        (["--quiet"], "testmod --quiet"),
        (["--verbose"], "testmod --verbose"),
        (
            ["--overwrite", "--quiet", "-a", "-b", "text=a,b", "other=c"],
            'testmod --overwrite --quiet -a -b text="a,b" other="c"',
        ),
    ],
)
def test_recreated_command(xy_session_for_module, arguments, expected):
    assert recreate(xy_session_for_module, *arguments) == expected


def test_long_value_is_kept(xy_session_for_module):
    """A value much longer than the initial buffer is not cut or overrun"""
    value = "x" * 3000
    assert recreate(xy_session_for_module, f"text={value}") == (
        f'testmod text="{value}"'
    )


@QUOTES_NOT_ESCAPED
def test_quote_in_value_is_escaped(xy_session_for_module):
    """A double quote in a value must not end the quoted value early"""
    result = recreate(xy_session_for_module, 'text=say "hi"')
    assert result == 'testmod text="say \\"hi\\""'


@QUOTES_NOT_ESCAPED
def test_quote_in_later_value_is_escaped(xy_session_for_module):
    result = recreate(xy_session_for_module, 'text=a,"b"')
    assert result == 'testmod text="a,\\"b\\""'


@QUOTES_NOT_ESCAPED
def test_many_quotes_in_long_value_are_escaped(xy_session_for_module):
    """Escaping doubles the length of the value, which the buffer must hold"""
    value = '"' * 3000
    result = recreate(xy_session_for_module, f"text={value}")
    assert result == f'testmod text="{value.replace(chr(34), chr(92) + chr(34))}"'


# Not strict: the stale pointer is only written to freed memory when realloc
# moves the buffer, so the test can pass by luck before the fix.
@pytest.mark.xfail(
    reason="G_recreate_command() writes --overwrite to the buffer it just freed",
)
def test_overwrite_after_long_program_name(xy_session_for_module):
    """Adding --overwrite after the buffer grows keeps it in the result

    The program name is long enough that adding --overwrite reallocates the
    buffer. A stale pointer to the old buffer would be reliably noticed only
    by a memory checker such as AddressSanitizer.
    """
    program = "m" * 1015
    result = recreate(xy_session_for_module, "text=abc", "--overwrite", program=program)
    assert result == f'{program} --overwrite text="abc"'
