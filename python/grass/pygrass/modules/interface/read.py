"""
Created on Tue Apr  2 18:30:34 2013

@author: pietro
"""

from __future__ import annotations

from typing import TYPE_CHECKING


if TYPE_CHECKING:
    from collections.abc import Callable
    from typing import TypeVar, Never
    from xml.etree.ElementTree import Element

    T = TypeVar(name="T")


def do_nothing(p: T) -> T:
    return p


def get_None(p: Never) -> None:
    return None


def get_dict(p: Element[str]) -> dict[str, str]:
    return dict(p.items())


def get_values(p: Element[str]) -> list[str]:
    return [e.text.strip() for e in p.findall(path="value/name")]


def read_text(p: Element[str]) -> str:
    return p.text.strip()


def read_keydesc(par: Element[str]) -> tuple[str, tuple[str, ...] | None]:
    name: str = par.text.strip()
    items: list[str] = [e.text.strip() for e in par.findall("item")]
    return name, tuple(items) if len(items) > 1 else None


GETFROMTAG: dict[
    str,
    Callable[Element[str], str]
    | Callable[Element[str], tuple[str, tuple[str, ...] | None]]
    | Callable[Element[str], dict[str, str]]
    | Callable[Element[str], list[str]]
    | Callable[Never, None],
] = {
    "description": read_text,
    "keydesc": read_keydesc,
    "gisprompt": get_dict,
    "default": read_text,
    "values": get_values,
    "value": get_None,
    "guisection": read_text,
    "label": read_text,
    "suppress_required": get_None,
    "keywords": read_text,
    "guidependency": read_text,
    "rules": get_None,
}

GETTYPE = {
    "string": str,
    "integer": int,
    "float": float,
    "double": float,
    "file": str,
    "all": do_nothing,
}


def element2dict(xparameter: Element[str]):
    diz: dict[str, str] = dict(xparameter.items())
    for p in xparameter:
        if p.tag in GETFROMTAG:
            diz[p.tag] = GETFROMTAG[p.tag](p)
        else:
            print("New tag: %s, ignored" % p.tag)
    return diz


# dictionary used to create docstring for the objects
DOC: dict[str, str] = {
    # ------------------------------------------------------------
    # head
    "head": """{cmd_name}({cmd_params})

Parameters
----------

""",
    # ------------------------------------------------------------
    # param
    "param": """{name}: {default}{required}{multi}{ptype}
    {description}{values}{keydescvalues}""",
    # ------------------------------------------------------------
    # flag_head
    "flag_head": """
Flags
------
""",
    # ------------------------------------------------------------
    # flag
    "flag": """{name}: {default}, {suppress}
    {description}""",
    # ------------------------------------------------------------
    # foot
    "foot": """
Special Parameters
------------------

The Module class have some optional parameters which are distinct using a final
underscore.

run_: True, optional
    If True execute the module.
finish_: True, optional
    If True wait until the end of the module execution, and store the module
    outputs into stdout, stderr attributes of the class.
stdin_: PIPE, optional
    Set the standard input.
env_: dictionary, optional
    Set the environment variables.
""",
}
