#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2026 GRASS Development Team
# SPDX-License-Identifier: GPL-2.0-or-later

"""Recreate the figures and the numbers of the page on random number layouts

The page is random_streams.dox.md in this directory. Its figures are the
random_streams*.svg files, and the numbers it quotes are values of the
generator behind G_random_double(), computed here with the same recurrence.

Usage:
    python3 random_streams_figures.py [directory]
        Write the figures into the directory (by default, this one).
    python3 random_streams_figures.py --numbers
        Print the numbers quoted on the page and in the figures.

Angles are in degrees, clockwise from the positive x axis as SVG draws them.
"""

import math
import sys
from pathlib import Path

# The generator: the next state is (MULTIPLIER * state + INCREMENT) modulo
# MODULUS, and the value drawn is the new state divided by MODULUS.
MODULUS = 1 << 48
MULTIPLIER = 0x5DEECE66D
INCREMENT = 0xB
SPAN = 1 << 46

# The names which depend on the API.
SEED_FN = "G_random_state_from_seed()"
WHOLE_FN = "G_random_init_layout()"
EXACT_FN = "G_random_init_layout_exact()"
BOUNDED_FN = "G_random_init_layout_bounded()"
BATCH = "batch"

INK, GREY, RED, PALE_RED = "#222", "#999", "#c33", "#dba5a5"
ARROW = [
    "  <defs>",
    '    <marker id="arrow" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="7" markerHeight="7" orient="auto-start-reverse">',
    '      <path d="M 0 0 L 10 5 L 0 10 z" fill="#222"/>',
    "    </marker>",
    "  </defs>",
]


def seed_state(seed):
    """State at which a seed starts, as G_srand48() sets it"""
    return ((seed & 0xFFFFFFFF) << 16) | 0x330E


def jump(state, draws):
    """State after a number of draws, without taking them one at a time"""
    a_total, c_total, a, c = 1, 0, MULTIPLIER, INCREMENT
    while draws:
        if draws & 1:
            a_total, c_total = (a * a_total) % MODULUS, (a * c_total + c) % MODULUS
        a, c = (a * a) % MODULUS, (a * c + c) % MODULUS
        draws >>= 1
    return (a_total * state + c_total) % MODULUS


def values(state, count):
    """The next values drawn from a state"""
    result = []
    for _ in range(count):
        state = (MULTIPLIER * state + INCREMENT) % MODULUS
        result.append(state / MODULUS)
    return result


class Svg:
    """Lines of an SVG file with the page's font and ink"""

    def __init__(self, title, width, height, view_box=None, defs=()):
        view_box = view_box or f"0 0 {width} {height}"
        self.head = [
            '<?xml version="1.0" encoding="UTF-8"?>',
            (
                f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" '
                f'height="{height}" viewBox="{view_box}" font-family="sans-serif" '
                'font-size="13" fill="#222">'
            ),
            f"  <title>{title}</title>",
            *defs,
        ]
        self.out = []

    def add(self, line):
        self.out.append("  " + line)

    def comment(self, *lines):
        self.add("<!-- " + "\n       ".join(lines) + " -->")

    def text(self, x, y, content, anchor=None, fill=None, size=None, rotate=False):
        attrs = f' text-anchor="{anchor}"' if anchor else ""
        attrs += f' fill="{fill}"' if fill else ""
        attrs += f' font-size="{size}"' if size else ""
        attrs += f' transform="rotate(-90 {x:.1f} {y:.1f})"' if rotate else ""
        self.add(f'<text x="{x:.1f}" y="{y:.1f}"{attrs}>{content}</text>')

    def write(self, path):
        path.write_text("\n".join([*self.head, *self.out, "</svg>"]) + "\n")


def on_ring(center, angle, radius):
    """Point on a circle around the center"""
    a = math.radians(angle)
    return center[0] + radius * math.cos(a), center[1] + radius * math.sin(a)


def arc(center, start, end, radius):
    """Path of a clockwise arc"""
    (x0, y0), (x1, y1) = on_ring(center, start, radius), on_ring(center, end, radius)
    large = 1 if (end - start) % 360 > 180 else 0
    return f"M {x0:.1f} {y0:.1f} A {radius} {radius} 0 {large} 1 {x1:.1f} {y1:.1f}"


def ring_figure():
    """The ring of states with the seed, the span and three units"""
    center, radius, seed, stride = (380, 200), 140, 135, 25
    drawn = [20, 14, 18]  # degrees each unit has drawn of its stride
    svg = Svg(
        "The ring of generator states with the seed, the span and the strides "
        "and draws of three units",
        640,
        340,
        view_box="0 30 640 340",
        defs=ARROW,
    )
    svg.comment("The three quarters of the ring outside the span, drawn light.")
    svg.add(
        f'<path d="{arc(center, seed + 90, seed + 360, radius)}" fill="none" '
        'stroke="#bbb" stroke-width="3" stroke-dasharray="6 5"/>'
    )
    svg.comment("The span: the quarter of the ring after the seed, clockwise.")
    svg.add(
        f'<path d="{arc(center, seed, seed + 90, radius)}" fill="none" '
        'stroke="#ddd" stroke-width="14"/>'
    )
    svg.comment("What units 0, 1 and 2 have drawn, each from the start of its stride.")
    for unit, degrees in enumerate(drawn):
        start = seed + unit * stride
        svg.add(
            f'<path d="{arc(center, start, start + degrees, radius)}" fill="none" '
            f'stroke="{INK}" stroke-width="3"/>'
        )
    svg.comment(
        "Ticks one stride apart from the seed, where the units start, and the",
        "end of the span.",
    )
    for angle in [seed + i * stride for i in range(4)] + [seed + 90]:
        (x0, y0), (x1, y1) = (
            on_ring(center, angle, radius - 9),
            on_ring(center, angle, radius + 9),
        )
        svg.add(
            f'<line x1="{x0:.1f}" y1="{y0:.1f}" x2="{x1:.1f}" y2="{y1:.1f}" '
            f'stroke="{INK}" stroke-width="2"/>'
        )
    svg.comment("The units, named inside the ring at the middle of their strides.")
    for unit in range(3):
        x, y = on_ring(center, seed + (unit + 0.5) * stride, radius - 16)
        svg.text(x, y + 4.5, f"unit {unit}")
    svg.comment("One stride, from the start of unit 2 to the start of the next unit.")
    path = arc(center, seed + 2 * stride + 1.5, seed + 3 * stride - 1.5, radius + 20)
    svg.add(
        f'<path d="{path}" fill="none" stroke="{INK}" stroke-width="1.5" '
        'marker-start="url(#arrow)" marker-end="url(#arrow)"/>'
    )
    x, y = on_ring(center, seed + 2.5 * stride, radius + 28)
    svg.text(x, y + 4.5, "stride", anchor="end")
    svg.comment("The draws of unit 0: the part of its stride it has drawn.")
    x, y = on_ring(center, seed + drawn[0] / 2, radius + 14)
    svg.text(x, y + 4.5, "draws", anchor="end")
    svg.comment("The span with the direction of drawing.")
    svg.add(
        f'<path d="{arc(center, seed + 3, seed + 87, radius + 75)}" fill="none" '
        f'stroke="{INK}" stroke-width="1.5" marker-end="url(#arrow)"/>'
    )
    svg.text(center[0] - radius - 88, center[1] - 4, "span:", anchor="end")
    svg.text(center[0] - radius - 88, center[1] + 12, "2^46 draws", anchor="end")
    svg.text(center[0] + 40, center[1] - 4, "ring of", anchor="middle")
    svg.text(center[0] + 40, center[1] + 14, "2^48 states", anchor="middle")
    x, y = on_ring(center, seed, radius)
    svg.add(f'<circle cx="{x:.1f}" cy="{y:.1f}" r="5" fill="{RED}"/>')
    svg.text(x, y + 28, "seed", anchor="middle")
    svg.comment("The state of unit 1: the position at which it draws next.")
    x, y = on_ring(center, seed + stride + drawn[1], radius)
    svg.add(f'<circle cx="{x:.1f}" cy="{y:.1f}" r="5" fill="{INK}"/>')
    svg.text(x - 14, y + 4.5, "state", anchor="end")
    return svg


def twins_figure():
    """The first values of seed 42 at the four quarters of the ring"""
    center, radius, seed = (320, 205), 130, 135
    svg = Svg(
        "The first values of seed 42 at the seed and a quarter, a half and "
        "three quarters of the ring after it",
        640,
        300,
        view_box="0 55 640 300",
        defs=ARROW,
    )
    svg.comment("The three quarters of the ring outside the span, drawn light.")
    svg.add(
        f'<path d="{arc(center, seed + 90, seed + 360, radius)}" fill="none" '
        'stroke="#bbb" stroke-width="3" stroke-dasharray="6 5"/>'
    )
    svg.comment("The span: the quarter of the ring after the seed, clockwise.")
    svg.add(
        f'<path d="{arc(center, seed, seed + 90, radius)}" fill="none" '
        'stroke="#ddd" stroke-width="14"/>'
    )
    svg.add(
        f'<path d="{arc(center, seed + 6, seed + 84, radius - 26)}" fill="none" '
        f'stroke="{INK}" stroke-width="1.5" marker-end="url(#arrow)"/>'
    )
    svg.text(232, 209, "span")
    svg.comment(
        "The seed and the positions a quarter, a half and three quarters of",
        "the ring after it, where the generator draws the seed's values plus",
        "1/4, 1/2 and 3/4, wrapping past 1.",
    )
    labels = ["seed 42:", "+ 1/4:", "+ 1/2:", "+ 3/4:"]
    for quarter, label in enumerate(labels):
        x, y = on_ring(center, seed + 90 * quarter, radius)
        fill = RED if quarter == 0 else "#fff"
        stroke = "" if quarter == 0 else f' stroke="{RED}" stroke-width="2"'
        svg.add(f'<circle cx="{x:.1f}" cy="{y:.1f}" r="5" fill="{fill}"{stroke}/>')
        drawn = values(jump(seed_state(42), quarter * SPAN), 3)
        left, low = quarter in {0, 1}, quarter in {0, 3}
        text_x = x - 16 if left else x + 16
        text_y = y + 15 if low else y - 21
        anchor = "end" if left else None
        svg.text(text_x, text_y, label, anchor=anchor)
        svg.text(
            text_x, text_y + 16, ", ".join(f"{v:.4f}" for v in drawn), anchor=anchor
        )
    return svg


class Bars(Svg):
    """Layouts drawn as bars along the span"""

    left, right = 80, 600
    drawn = (14, 22, 9, 27, 19, 12)  # what each unit of a whole-span layout draws
    drawn_short = (8, 13, 5, 16, 11, 7)  # the same for the strides of a bounded layout

    def __init__(self, title, height, span_line_to):
        super().__init__(title, 640, height)
        self.comment(
            "The end of the span, 2^46 draws after the seed, for all the bars."
        )
        self.add(
            f'<line x1="{self.right}" y1="42" x2="{self.right}" y2="{span_line_to}" '
            'stroke="#bbb" stroke-width="1.5" stroke-dasharray="6 5"/>'
        )

    @property
    def span(self):
        return self.right - self.left

    def bar(self, y, ink):
        self.add(
            f'<rect x="{self.left}" y="{y}" width="{self.span}" height="24" '
            f'fill="none" stroke="{ink}" stroke-width="1.5"/>'
        )

    def seed(self, y, grey=False):
        self.add(
            f'<circle cx="{self.left}" cy="{y + 12}" r="5" '
            f'fill="{PALE_RED if grey else RED}"/>'
        )
        self.text(
            self.left - 10, y + 16.5, "seed", anchor="end", fill=GREY if grey else None
        )

    def tick(self, x, y, ink):
        self.add(
            f'<line x1="{x:.1f}" y1="{y}" x2="{x:.1f}" y2="{y + 24}" '
            f'stroke="{ink}" stroke-width="1.5"/>'
        )

    def brace(self, start, end, y, label):
        self.add(
            f'<path d="M {start:.1f} {y + 28} L {start:.1f} {y + 32} L {end:.1f} '
            f'{y + 32} L {end:.1f} {y + 28}" fill="none" stroke="{INK}" '
            'stroke-width="1.5"/>'
        )
        self.text((start + end) / 2, y + 48, label, anchor="middle")

    def strides(self, y, stride, count, ink, drawn, number_at, added=0):
        """Units along the bar: boundaries, numbers, and what batch 0 draws"""
        for i in range(count):
            x = self.left + i * stride
            if i:
                self.tick(x, y, ink)
            if i < 6:
                self.add(
                    f'<rect x="{x:.1f}" y="{y}" width="{drawn[i]}" height="24" '
                    f'fill="{ink}"/>'
                )
            if added:
                self.add(
                    f'<rect x="{x + stride - added:.1f}" y="{y + 1}" '
                    f'width="{added}" height="22" fill="#ccc"/>'
                )
            self.text(
                x + number_at * stride,
                y + 16.5,
                i % 6,
                anchor="middle",
                fill=ink if ink != INK else None,
            )
        self.tick(self.left + count * stride, y, ink)

    def red_mark(self, x, y):
        self.add(
            f'<line x1="{x:.1f}" y1="{y - 3}" x2="{x:.1f}" y2="{y + 27}" '
            f'stroke="{RED}" stroke-width="1.5" stroke-dasharray="4 3"/>'
        )

    def whole_span(self, y, note):
        self.comment(
            "The whole span divided into an odd number of equal parts, seven for",
            "six units, and a stride of a part rounded down to odd; the six units",
            "take six parts, the last part stays unused, and there is one batch",
            "only.",
        )
        part = self.span / 7
        self.text(self.left, y - 26, f"whole-span layout: {WHOLE_FN}")
        self.text(self.left, y - 10, note)
        self.bar(y, INK)
        self.strides(y, part, 6, INK, self.drawn, 0.7)
        self.text(
            self.left + 6.5 * part, y + 16.5, "unused", anchor="middle", fill="#888"
        )
        self.seed(y)

    def bounded(self, y):
        self.comment(
            "Bounded: a stride of the bound rounded up to odd, with the added draw",
            "in light grey, much wider than one draw is.",
        )
        self.text(self.left, y - 26, f"bounded layout: {BOUNDED_FN}")
        self.text(
            self.left,
            y - 10,
            "stride: the bound rounded up to odd; light grey: the draw added to "
            "an even bound",
        )
        self.bar(y, INK)
        self.strides(y, 40, 12, INK, self.drawn_short, 0.65, added=4)
        self.seed(y)
        self.brace(self.left, self.left + 240, y, f"{BATCH} 0")
        self.brace(self.left + 240, self.left + 480, y, f"{BATCH} 1")


def layouts_figure():
    """A single sequence and the three layouts of six units"""
    svg = Bars(
        "A single sequence and the whole-span, exact and bounded layouts of six "
        "units along the span",
        420,
        366,
    )
    y = 48
    svg.comment(
        "A single sequence: one unit which owns the whole span and draws the",
        "seed's sequence from its start, the values which the units of batch 0",
        "of the exact layout below draw together.",
    )
    svg.text(svg.left, y - 26, f"a single sequence: {SEED_FN}")
    svg.text(
        svg.left,
        y - 10,
        "one unit, which owns the whole span and draws the seed's sequence",
    )
    svg.bar(y, INK)
    svg.add(f'<rect x="{svg.left}" y="{y}" width="216" height="24" fill="{INK}"/>')
    svg.text(
        svg.left + 108,
        y + 16.5,
        "the values drawn so far",
        anchor="middle",
        fill="#fff",
    )
    svg.seed(y)

    y = 126
    svg.whole_span(
        y,
        f"stride: a part rounded down to odd, seven parts for six units; one {BATCH} only",
    )
    svg.brace(svg.left, svg.left + 6 * svg.span / 7, y, f"{BATCH} 0, the only {BATCH}")

    y = 224
    svg.comment(
        "Exact: a stride of exactly the values a unit draws; the units of batch",
        "0 draw the start of the seed's sequence and those of batch 1 what",
        "follows.",
    )
    svg.text(svg.left, y - 26, f"exact layout: {EXACT_FN}")
    svg.text(
        svg.left,
        y - 10,
        "stride: the values each unit draws; the units draw the seed's sequence in order",
    )
    svg.bar(y, INK)
    svg.add(f'<rect x="{svg.left}" y="{y}" width="216" height="24" fill="{INK}"/>')
    for i in range(12):
        x = svg.left + 36 * i
        if 0 < i < 6:
            svg.add(
                f'<line x1="{x}" y1="{y}" x2="{x}" y2="{y + 24}" stroke="#fff" '
                'stroke-width="1.5"/>'
            )
        elif i > 6:
            svg.tick(x, y, INK)
        svg.text(
            x + 18, y + 16.5, i % 6, anchor="middle", fill="#fff" if i < 6 else None
        )
    svg.tick(svg.left + 432, y, INK)
    svg.seed(y)
    svg.brace(svg.left, svg.left + 216, y, f"{BATCH} 0: the same values as above")
    svg.brace(svg.left + 216, svg.left + 432, y, f"{BATCH} 1: the values which follow")

    svg.bounded(322)
    svg.text(
        svg.right,
        390,
        "red dot: the seed; dashed line: the end of the span, 2^46 draws after the seed",
        anchor="end",
    )
    svg.text(svg.right, 408, f"dark: what the units of {BATCH} 0 draw", anchor="end")
    return svg


def parts_figure():
    """The whole-span layout and what an even number of parts would give"""
    svg = Bars(
        "The whole-span layout of six units with seven parts and what six parts "
        "would give",
        196,
        138,
    )
    y = 48
    svg.whole_span(
        y,
        "seven parts for six units: half the span lies in the middle of the "
        "stride of unit 3",
    )
    half = svg.left + svg.span / 2
    svg.red_mark(half, y)
    y = 108
    svg.comment(
        "Without the odd number of parts: the span divided into as many parts as",
        "there are units; unit 3 starts half the span after unit 0. In grey, since",
        "the library does not make this layout.",
    )
    svg.text(
        svg.left,
        y - 10,
        "with six parts, unit 3 would start at half the span",
        fill=GREY,
    )
    svg.bar(y, GREY)
    svg.strides(y, svg.span / 6, 6, GREY, svg.drawn, 0.7)
    svg.seed(y, grey=True)
    svg.red_mark(half, y)
    svg.comment("Half the span, 2^45 draws after the seed.")
    svg.text(
        half,
        y + 46,
        "half the span: the values of a unit starting here would be related to "
        "those of unit 0",
        anchor="middle",
        fill=RED,
    )
    svg.text(
        svg.right,
        y + 72,
        "red dot: the seed; dashed line: the end of the span; grey: not a layout "
        "the library makes",
        anchor="end",
    )
    return svg


def strides_figure():
    """The bounded layout and what an even bound as the stride would give"""
    svg = Bars(
        "The bounded layout with an odd stride and what an even bound taken as "
        "the stride would give",
        222,
        164,
    )
    svg.bounded(48)
    y = 134
    svg.comment(
        "Without the rounding: an even bound as the stride; the units 1, 2 and 4",
        "strides after unit 0 start at distances which carry the stride's power",
        "of two as well. In grey, since the library does not make this layout.",
    )
    svg.text(svg.left, y - 10, "with an even bound taken as the stride", fill=GREY)
    svg.bar(y, GREY)
    svg.strides(y, 36, 12, GREY, svg.drawn_short, 0.72)
    svg.seed(y, grey=True)
    for unit in (1, 2, 4):
        svg.red_mark(svg.left + unit * 36, y)
    svg.text(
        svg.left + svg.span / 2,
        y + 46,
        "an even bound would tie more low bits of the values of units 1, 2, and "
        "4 to those of unit 0",
        anchor="middle",
        fill=RED,
    )
    svg.text(
        svg.right,
        y + 72,
        "red dot: the seed; dashed line: the end of the span; grey: not a layout "
        "the library makes",
        anchor="end",
    )
    return svg


# The seeds figure: 64 computations, either seeded 42 to 105 or drawing from
# batches 0 to 63 of an exact layout of seed 42 with a million units of a
# million draws each.
COMPUTATIONS = 64
BATCH_DRAWS = 1000000 * 1000000


def seeds_figure():
    """The first values of consecutive seeds and of batches of one layout"""
    by_seed = [values(seed_state(42 + k), 2) for k in range(COMPUTATIONS)]
    by_batch = [
        values(jump(seed_state(42), b * BATCH_DRAWS), 2) for b in range(COMPUTATIONS)
    ]
    width, height = 230, 110
    columns, rows = (80, 370), (52, 196)
    svg = Svg(
        "The first two values of 64 consecutive seeds, which lie on lines, and of "
        "64 batches of one layout, which scatter",
        640,
        340,
    )

    def plot(x0, y0, series, draw, color):
        svg.add(
            f'<rect x="{x0}" y="{y0}" width="{width}" height="{height}" fill="none" '
            'stroke="#bbb" stroke-width="1.5"/>'
        )
        for k in range(COMPUTATIONS):
            x = x0 + 6 + k * (width - 12) / (COMPUTATIONS - 1)
            y = y0 + height - 4 - series[k][draw] * (height - 8)
            svg.add(f'<circle cx="{x:.1f}" cy="{y:.1f}" r="2.3" fill="{color}"/>')
        svg.text(x0 - 6, y0 + height - 1, "0", anchor="end", size=11, fill="#888")
        svg.text(x0 - 6, y0 + 9, "1", anchor="end", size=11, fill="#888")

    svg.comment(
        "Consecutive seeds: the value of seed 42 + k is that of seed 42 plus k",
        "times one amount, modulo 1, so the values lie on straight lines.",
    )
    svg.text(columns[0], 22, "seeds 42, 43, ... 105:")
    svg.text(columns[0], 38, "the values lie on lines")
    svg.comment(
        "Batches of one layout: unit 0 of batches 0 to 63 of an exact layout of",
        "seed 42 with a million units of a million draws each.",
    )
    svg.text(columns[1], 22, "batches 0, 1, ... 63 of one layout:")
    svg.text(columns[1], 38, "the values scatter")
    for draw, y0 in enumerate(rows):
        svg.text(columns[0] - 26, y0 + height / 2 - 4, "draw", anchor="end")
        svg.text(columns[0] - 26, y0 + height / 2 + 12, f"{draw + 1}", anchor="end")
        plot(columns[0], y0, by_seed, draw, RED)
        plot(columns[1], y0, by_batch, draw, INK)
    y = rows[1] + height + 18
    svg.text(columns[0] + width / 2, y, "seed, from 42 to 105", anchor="middle")
    svg.text(columns[1] + width / 2, y, "batch, from 0 to 63", anchor="middle")
    return svg


FIGURES = {
    "random_streams.svg": ring_figure,
    "random_streams_twins.svg": twins_figure,
    "random_streams_layouts.svg": layouts_figure,
    "random_streams_parts.svg": parts_figure,
    "random_streams_strides.svg": strides_figure,
    "random_streams_seeds.svg": seeds_figure,
}


def print_numbers():
    """Print the numbers which the page and the figures quote"""
    start = seed_state(42)
    first = values(start, 8)
    print("seed 42 draws:", ", ".join(f"{v:.4f}" for v in first))
    print("\nValues a distance after seed 42 minus those of seed 42, modulo 1:")
    for label, distance in [(f"2^{e}", 1 << e) for e in (47, 46, 45, 44, 40, 36)] + [
        ("12345 (odd)", 12345)
    ]:
        later = values(jump(start, distance), 4096)
        differences = [
            (b - a) % 1 for a, b in zip(values(start, 4096), later, strict=True)
        ]
        print(
            f"  {label}: {', '.join(f'{d:.4f}' for d in differences[:4])}, ...; "
            f"{len(set(differences))} different in 4096 draws"
        )
    total = 10**11
    print(f"\nExpected occurrence for T = {total:g} values, 2^k * T / 2^46:")
    for k in (1, 2, 6, 10):
        print(f"  distance 2^{46 - k} (k = {k}): {2**k * total / SPAN:.4f}")
    print("\nSeeds a quarter and a half of the ring apart:")
    for label, seed in (("42 + 2^30", 42 + (1 << 30)), ("42 + 2^31", 42 + (1 << 31))):
        print(
            f"  seed {label}: {', '.join(f'{v:.4f}' for v in values(seed_state(seed), 4))}"
        )
    step = [(b - a) % 1 for a, b in zip(first, values(seed_state(43), 8), strict=True)]
    print("\nStep from one seed to the next, at draws 1 to 4:")
    print("  " + ", ".join(f"{s:.4f}" for s in step[:4]))
    print(
        f"\nBatches of {BATCH_DRAWS:g} draws which fit into the span: {SPAN // BATCH_DRAWS}"
    )
    print(f"Units of a million draws which fit into one batch: {SPAN // 1000000}")


def main():
    if "--numbers" in sys.argv[1:]:
        print_numbers()
        return
    directory = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(__file__).parent
    for name, figure in FIGURES.items():
        figure().write(directory / name)
        print(directory / name)


if __name__ == "__main__":
    main()
