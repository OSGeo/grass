#!/usr/bin/env python3
import math
import re

# fmt: off
fonts = ("cyrilc", "gothgbt", "gothgrt", "gothitt", "greekc", "greekcs",
         "greekp", "greeks", "italicc", "italiccs", "italict", "romanc",
         "romancs", "romand", "romans", "romant", "scriptc", "scripts",
         "cyrilc", "gothgbtlu")
colors = ("red", "orange", "yellow", "green", "blue", "indigo", "violet",
          "black", "gray", "brown", "magenta", "aqua", "grey", "cyan",
          "purple")
# fmt: on


def rc(*args):
    print(f".X {args[0]}\n.Y {args[1]}")


def xy(*args):
    print(f".X {args[0]}\n.Y {args[1]}")


def font(*args):
    print(f".F {args[0]}")


def size(*args):
    print(f".S {args[0]}")


def color(*args):
    print(f".C {args[0]}")


def rotate(*args):
    print(f".R {args[0]}")


def align(*args):
    print(f".A {args[0]}")


def text(*args):
    print(f"{args[0]}")


for i in range(36):
    font(fonts[int(i % len(fonts))])
    size(((36 - i if (i >= 9 and i <= 18 or i > 27) else i) % 9))
    rotate(i * 10)
    color(colors[i % len(colors)])
    xy(
        (80 + 10 * math.cos(i * 10 / 180 * 3.141593)),
        (50 + 10 * 640 / 480 * math.sin(i * 10 / 180 * 3.141593)),
    )
    text(fonts[int(i % len(fonts))])

size(2)
rotate(0)
font("romans")
color("gray")
rc(1, 1)

with open(__file__) as f:
    src = "\n.L 1\n.L 0\n".join(f.read().split("\n")[1:])
    f.close()

print(re.sub('(".*?")', "\n.C red\n\\g<0>\n.C gray\n", src))
