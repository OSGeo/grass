#!/usr/bin/env python3
# Author: Owen Smith - Rewritten from test.pl by Huidae Cho
# Run: d.mon start=wx0 && ./test.py | d.text at=0,100
import math
from pathlib import Path
import re

# Quiet black syntax checking for fonts and colors to keep the code printed to
# the display vertically short.
# fmt: off
fonts = ("cyrilc", "gothgbt", "gothgrt", "gothitt", "greekc", "greekcs",
         "greekp", "greeks", "italicc", "italiccs", "italict", "romanc",
         "romancs", "romand", "romans", "romant", "scriptc", "scripts",
         "cyrilc", "gothgbtlu")
colors = ("red", "orange", "yellow", "green", "blue", "indigo", "violet",
          "black", "gray", "brown", "magenta", "aqua", "grey", "cyan",
          "purple")
# fmt: on


def rc(r, c):
    print(f".X {r}\n.Y {c}")


def xy(x, y):
    print(f".X {x}%\n.Y {y}%")


def font(f):
    print(f".F {f}")


def size(s):
    print(f".S {s}")


def color(c):
    print(f".C {c}")


def rotate(r):
    print(f".R {r}")


def align(a):
    print(f".A {a}")


def text(in_text):
    print(f"{in_text}")


for i in range(36):
    font(fonts[int(i % len(fonts))])
    size((36 - i if ((9 <= i <= 18) or i > 27) else i) % 9)
    rotate(i * 10)
    color(colors[i % len(colors)])
    xy(
        (80 + 10 * math.cos(i * 10 / 180 * math.pi)),
        (50 + 10 * 640 / 480 * math.sin(i * 10 / 180 * math.pi)),
    )
    text(fonts[int(i % len(fonts))])

size(2)
rotate(0)
font("romans")
color("gray")
rc(1, 1)


src = Path(__file__).read_text()

print(
    ".L 0\n"
    + re.sub(
        r'(".*?")',
        "\n.C red\n,\\g<0>\n.C gray\n",
        re.sub(r"\n", "\n.L 1\n.L 0\n", re.sub(r"(?m)^#.*\n?", "", src)),
    )
)
