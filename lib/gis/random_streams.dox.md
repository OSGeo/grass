# Random Number Streams {#gislib_random_streams}

[TOC]

## Overview {#gislib_random_streams_overview}

The `G_random_*()` functions give a tool random number generators of its own
and many streams of values from one seed which do not overlap. They serve
parallel computations, the members of an ensemble, the components of a model
which must each be reproducible on their own, and library code which must not
change the random numbers of the tool which calls it.

A tool builds one layout from its seed and places a state for every unit of
work, such as a row of a raster or a particle. The values of a unit are then
fixed by the seed, the layout and the unit's number, which allows parallel
code to give the same result with any number of threads.

The generator is a 48-bit linear congruential generator, the one behind the C
function `drand48()` and behind `G_drand48()` in GRASS.

## Terms {#gislib_random_streams_terms}

- **Generator**: the rule which gives the next state from the current one, and
  with it the next value, a number in [0, 1). Its states form one ring, and
  every draw moves one step along it.
- **Seed**: the integer which picks where on the ring drawing starts. The
  values drawn one after another from there are the seed's sequence.
- **State**: a `struct G_random_state`, a position on the ring and nothing
  else, used by one thread at a time; copying it copies the position.
  `G_random_double()` moves it one step and returns the value there.
- **Span**: the quarter of the ring after the seed. The library places
  everything within it (see \ref gislib_random_streams_span for why).
- **Layout**: a `struct G_random_layout`, holding where the seed's sequence
  starts, the number of units and the stride. A tool builds it once, outside
  any parallel region, and only reads it afterwards.
- **Unit**: a piece of work with values of its own, such as a row of a raster,
  a particle, a point or an object, numbered from 0. Its **stream** is the
  values of the seed's sequence it draws, one stream per unit and batch.
- **Stride**: the number of draws from the start of one unit to the start of
  the next, and so the most values a unit may draw. `G_random_layout_length()`
  returns it.
- **Batch**: one stream for every unit of a layout. The batches follow one
  another along the span, and a computation which needs one stream per unit
  uses batch 0 only.

\image html random_streams.svg

The figure shows the ring of states, the seed (the red dot) and the span, the
shaded quarter after the seed, with the arrow in the direction of drawing. The
ticks are one stride apart from the seed, and the last one marks the end of
the span. Units 0, 1 and 2 each start at a tick, the dark arcs are their draws
so far, and the black dot is the state of unit 1, the position at which it
draws next. Every seed starts at a different place on the ring, and the span
is always the quarter after it.

## Layouts {#gislib_random_streams_layouts}

A layout gives every unit a stream of its own. A single sequence is the
simplest case, and the three ways to initialize a layout for more units differ
in where the stride comes from:

- A single sequence, `G_random_state_from_seed(&rng, seed)`, is one unit which
  owns the whole span and draws the seed's sequence. It needs no
  `struct G_random_layout`: the function places the state at the seed.
- A whole-span layout, `G_random_init_layout(&layout, seed, units)`, divides
  the span into an odd number of equal parts, as many as there are units or
  one more when that number is even, and gives every unit the longest stride
  the span allows, in a single batch. The stride is the length of a part
  rounded down to odd (see \ref gislib_random_streams_distance and
  \ref gislib_random_streams_odd). A single unit keeps the whole span. A
  whole-span layout holds at most 2^20 units; a bounded layout serves more.
- An exact layout,
  `G_random_init_layout_exact(&layout, seed, units, draws_per_unit)`, takes
  the stride as given, the number of values every unit draws, and leaves the
  rest of the span to further batches. Unit u of batch 0 then draws values u ×
  stride to (u + 1) × stride - 1 of the seed's sequence, counted from 0, so
  the units together reproduce that sequence, whichever order they are
  computed in.
- A bounded layout,
  `G_random_init_layout_bounded(&layout, seed, units, max_draws)`, takes a
  bound on what a unit draws and rounds it up to odd (see
  \ref gislib_random_streams_odd), for units whose draws vary. But for that
  rounding, it places the units as an exact layout with the bound as its
  stride does.

\image html random_streams_layouts.svg

The figure shows the single sequence, whose one unit owns the whole span, and
below it the layouts of six units, with what each unit of batch 0 draws in
dark. The whole-span layout divides the span into seven parts; the six units
take six of them, one batch covers the span, and the last part stays unused.
In the exact layout, batch 0 is the seed's sequence and batch 1 follows. The
bounded layout has the strides of an exact layout whose stride is the bound,
made odd, and each unit draws less than its stride; the draw which rounding
adds to a stride is drawn much wider than one draw is. Why the number of parts
and the strides are odd is explained in \ref gislib_random_streams_distance
and \ref gislib_random_streams_odd.

The units of a layout can be placed more than once: a batch is one stream for
every unit, and the batches follow one another along the span, unit u of batch
b starting (b × units + u) × stride draws after the seed.
`G_random_state_for_unit(&rng, &layout, unit)` places a state at the start of
the unit's stream in batch 0,
`G_random_state_for_batch(&rng, &layout, batch, unit)` does so in any batch,
and `G_random_layout_batches(&layout)` returns the number of batches that fit
into the span. What a batch stands for is up to the tool: a member of an
ensemble (see \ref gislib_random_streams_ensembles), another pass over the
same units, or one of the processes of a model. Computations which must be
independent of one another share one seed and use different batches, not
different seeds (see \ref gislib_random_streams_between_seeds).

## Usage {#gislib_random_streams_usage}

A unit is what the computation is naturally divided into and numbered by;
the number of units is their count or an upper bound on it, and unused units
cost nothing but their share of the span. Which pattern applies depends on
whether a unit draws its values in one go or over many steps.

The seed comes from the user or is generated. A tool parses its seed option
into a `long long` with `strtoll()` and refuses what is not an integer from
-2^31 to 2^32 - 1, naming the option. When the user gives no seed,
`G_random_generate_seed()` gives one, from `GRASS_RANDOM_SEED`, or else from
`SOURCE_DATE_EPOCH`, or else from the time and the process ID. The tool
records the seed it used, for example in the history of the output map, so
that the computation can be repeated.

Whatever the pattern, the tool checks that no unit can draw more than the
stride, which `G_random_layout_length(&layout)` returns, and that the batches
it uses fit. Batch 0 can be placed even when no batch fits, so that a tool may
warn and continue, as the fragments below do. Placing a state costs about as
much as a few dozen draws, wherever its stream starts.

The values a unit draws do not depend on the thread which draws them. The
result of the whole computation is then the same for any number of threads
when nothing else in it depends on the schedule, which holds when every unit
writes only its own output and not when threads add into shared sums.

### A single sequence {#gislib_random_streams_single}

One sequence has one state, placed with `G_random_state_from_seed(&rng, seed)`
and drawn from in a loop. Its one unit owns the whole span, so there is no
`struct G_random_layout` to initialize and there are no batches to check:

```c
// seed: the seed, a long long; n: how many values to draw; values: where
// they go.
struct G_random_state rng;

G_random_state_from_seed(&rng, seed);
for (int i = 0; i < n; i++)
    values[i] = G_random_double(&rng);
```

### Rows of a raster {#gislib_random_streams_rows}

When a computation goes by rows of a raster and every row draws a known
number of values, for example a fixed number for each cell, a row is a unit
of an exact layout, and each thread owns one state. The thread places its
state at the stream of every row it takes, whatever the state held before,
and draws the row's values. Since the layout is exact, the rows together draw
the seed's sequence, so a computation which drew from one sequence row after
row keeps its values:

```c
#include <grass/gis.h>
#include <grass/glocale.h>

// seed: the seed, a long long; rows, cols: the region; draws_per_value: the
// number of random values drawn for one cell. The product is formed in
// long long, since it may not fit an int.
struct G_random_layout layout;

G_random_init_layout_exact(&layout, seed, rows,
                           (long long)cols * draws_per_value);
if (G_random_layout_batches(&layout) < 1)
    G_warning(_("The computation draws more than 2^46 random values; "
                "values beyond that repeat earlier values shifted by a "
                "constant"));

#pragma omp parallel
{
    struct G_random_state rng; // One per thread.

#pragma omp for
    for (int row = 0; row < rows; row++) {
        G_random_state_for_unit(&rng, &layout, row);
        // Compute the row, drawing with G_random_double(&rng).
    }
}
```

When the value is drawn in a function which does not know the row, for
example a callback, keep each thread's state where that function can reach
it, such as in an array indexed by the thread number, and place it when the
row starts. Placing a thread's state once and drawing all of its rows from it
would save the advances, but a row's values would then depend on which thread
took it and on the rows that thread computed before, so the result would
change with the number of threads and with the schedule.

The same pattern serves other items which draw all their values in one go,
such as points or objects: each item is a unit, the thread places its state
at the item's stream when it takes the item, and a bounded layout replaces
the exact one when the number of values varies under a known bound.

### Items drawn over time {#gislib_random_streams_items}

When a computation goes over items, such as particles, which draw a few
values at every one of many steps, each item is a unit which owns one state
for the whole computation. The state is placed once and drawn from at every
step by whichever thread handles the item in that step. It is never placed
again, since that would return it to the start of its stream and draw the
same values in every step. The states therefore live in an array for the
whole computation, not in the thread's loop. When the values an item draws in
a step vary under a known bound, the item is a unit of a bounded layout:

```c
#include <grass/gis.h>
#include <grass/glocale.h>

// seed: the seed, a long long; items, steps: the number of items and of time
// steps, as long long; max_draws_per_step: the most values an item draws in
// one step.
struct G_random_layout layout;
struct G_random_state *states = G_malloc(items * sizeof(*states));

G_random_init_layout_bounded(&layout, seed, items,
                             steps * max_draws_per_step);
if (G_random_layout_batches(&layout) < 1)
    G_warning(_("%lld items over %lld steps may draw more than 2^46 random "
                "values; values beyond that repeat earlier values shifted "
                "by a constant"),
              items, steps);
for (long long i = 0; i < items; i++)
    G_random_state_for_unit(&states[i], &layout, i);
// At every step, whichever thread moves item i draws from states[i].
G_free(states);
```

An item whose draws have no known bound gets the longest stride the span
allows from a whole-span layout, `G_random_init_layout(&layout, seed, items)`,
placed in the same way, and the tool warns when the stride,
`G_random_layout_length(&layout)`, is below an estimate of what an item draws.
With more items than a whole-span layout accepts, the tool chooses a bound
itself, from what it expects an item to draw, uses a bounded layout with that
bound, and checks that the batch fits as in the fragment above.

### Ensembles {#gislib_random_streams_ensembles}

An ensemble is many runs of the same model under one seed, meant to be
independent replicates. Each member draws from a batch of its own of the
layout the tool uses for a single run. A tool which supports ensembles takes
the number of the member, for example as an option `run` numbered from 1,
places unit u with `G_random_state_for_batch(&rng, &layout, run - 1, u)`, and
refuses a run beyond `G_random_layout_batches(&layout)`. With an exact or
bounded layout, a member does not need to know the size of the ensemble; a
whole-span layout holds a single batch and so cannot serve one.

The members must share the seed and whatever else decides the layout, the
number of units and the stride, so they should differ only in the run number
and the output names. The number of members is limited by the batches that fit
(see \ref gislib_random_streams_capacity).

## Capacity {#gislib_random_streams_capacity}

The number of batches that fit is the span, 2^46 draws, divided by the draws
of one batch, units × stride, rounded down; a whole-span layout holds one
batch. A million units drawing a million values each in an exact layout draw
10^12 values per batch, so 70 batches fit on one seed, and a single batch
could hold 70,368,744 such units, about 70 million.

The library does not check how many values a unit draws. A unit which draws
past its stride continues into the next unit's stream and draws the next
unit's values. Where the units stay within their strides, only batch 0 of a
layout into which no batch fits reaches past the span;
\ref gislib_random_streams_span explains what it draws there.

No layout and no way of seeding makes the span larger (see
\ref gislib_random_streams_span); more than it holds needs a generator with a
longer period behind the same calls.

## Quality of the values {#gislib_random_streams_quality}

The values are those of the 48-bit linear congruential generator of
`drand48()`: multiples of 2^-48, uniform in [0, 1), fine for simulations,
sampling, and Monte Carlo estimates, but not for cryptography. What needs care
is not the values of one unit but how the positions in use on the ring relate
to one another. The sections below say what a tool should know about that and
what the library does about it.

### The generator and the span {#gislib_random_streams_span}

A draw replaces the 48-bit state x by (a × x + c) mod 2^48, with the
multiplier a = 0x5DEECE66D and the increment c = 0xB, and returns the new
state divided by 2^48. The generator passes through all 2^48 states before
repeating; they are the ring.

The four quarters of the ring hold the same values shifted by 0, 0.25, 0.5 and
0.75: two positions a quarter of the ring, 2^46 draws, apart give values which
differ by exactly 0.25 at every draw. Three of the quarters therefore add no
values of their own, and the library places everything within the span, the
first 2^46 draws after the seed, where no two positions are that far apart.

\image html random_streams_twins.svg

The figure shows the seed and the positions a quarter, a half and three
quarters of the ring after it, with the first three values of seed 42 at each.

Only batch 0 of a layout into which no batch fits reaches past the span, where
it draws earlier values shifted by a constant; the warning of such a tool
should say so, as the fragments in \ref gislib_random_streams_usage do.
Seeding computations separately does not give more room, since more than 2^46
values drawn in total always include two positions a quarter of the ring
apart.

### Relations by distance {#gislib_random_streams_distance}

What relates two positions on the ring is their distance in draws: the more
often 2 divides it, the fewer constants the differences of their values cycle
through. Positions whose values differ by one constant are a constant-shift
twin, and by two constants in turn an alternating-shift twin. The table lists
the relations from the largest distance down, with the differences for seed 42
as an example. The expected occurrence is the number of distances with the
relation which a computation drawing T = 10^11 values in total is expected to
have between the positions it uses; below 1, it reads as a chance.

| distance in draws | part of the ring | relation | differences for seed 42 | expected occurrence |
| --- | --- | --- | --- | --- |
| 2^47 | 1/2 | constant-shift twin | 0.5000 at every draw | none: no two positions in the span are this far apart |
| 2^46 | 1/4 | constant-shift twin | 0.2500 at every draw | none: no two positions in the span are this far apart |
| 2^45 | 1/8 | alternating-shift twin | 0.6250, 0.1250 alternating | 0.003, a chance of 0.3% |
| 2^44 | 1/16 | one of 4 constants, cycling | 0.8125, 0.5625, 0.3125, 0.0625 | 0.006 |
| 2^40 | 1/256 | one of 64 constants, cycling | 0.3633, 0.5977, 0.1445, 0.7539, ... | about 0.09 |
| 2^36 | 1/4,096 | one of 1,024 constants, cycling | 0.7727, 0.0999, 0.2590, 0.1096, ... | about 1.5 |
| 2^(46 - k), for k from 1 to 46 | 1/2^(k + 2) | one of 2^k constants, cycling | 2^k values, then the same again | 2^k × T / 2^46 |
| any odd distance | an odd multiple of 1/2^48 | the low two bits fixed, 2^46 constants which do not repeat within the span | no pattern | about T, that is half of the distances which occur |

What a tool should be aware of:

- A relation at a large distance is easy to see in the values, but rarely
  occurs.
- A relation at a small distance occurs in most layouts, but cannot be seen in
  the values at the precision computations use.
- Batch 0 of an exact layout is the seed's sequence and has its structure, no
  better and no worse.

What the library does about it:

- Every layout lies within the span, so constant-shift twins do not occur.
- A whole-span layout divides the span into an odd number of parts, so that no
  unit starts at half or at a quarter of the span.
- Bounded and whole-span layouts have odd strides (see
  \ref gislib_random_streams_odd).

\image html random_streams_parts.svg

The figure shows a whole-span layout of six units, with seven parts, and in
grey what six parts would give: unit 3 would start at half the span, and its
values would be related to those of unit 0 from its first draw. With seven
parts, unit 3 reaches that position only after drawing half of its stride.

### Odd strides {#gislib_random_streams_odd}

A distance divisible by 2^j and by no higher power of two
fixes the low j + 2 bits of the difference of two states. Units 2^i apart in
number start 2^i × stride apart; with an odd stride that distance is divisible
by 2^i and by no higher power of two, so units 1, 2 and 4 apart have the low
2, 3 and 4 bits of their difference fixed. With an even stride, say a bound of
1,024 taken as it is, every such distance carries the stride's power of two on
top, and the same units have the low 12, 13 and 14 bits fixed. That is why the
library keeps the stride odd: the whole-span layout rounds down to odd and the
bounded layout rounds the bound up to odd, so no tool has to know the rule. An
exact layout cannot round, since its units must draw what the seed's sequence
draws; it keeps that sequence's structure and is never worse than the seed's
sequence it reproduces, in which the same values were the stride apart
already.

\image html random_streams_strides.svg

The figure shows a bounded layout, whose stride is the bound rounded up to
odd, and in grey an even bound taken as the stride: units 1, 2 and 4, which
start at the red marks, would have more of the low bits of their values tied
to those of unit 0, a risk for a computation which depends on the low bits of
its values.

### Relations between seeds {#gislib_random_streams_between_seeds}

On its own, any seed is as good as any other. Seeds are places on the same
ring, however, so two seeds are related by their distance. Seed s starts at
the state whose bits 16 to 47 are the low 32 bits of s and whose low 16 bits
are 0x330E, as `srand48()` sets it. So -1 and 4294967295 are the same seed,
and seeds 2^30 apart start a quarter of the ring apart: seed 42 + 2^30 draws
the values of seed 42 plus one quarter. Consecutive seeds start close
together, and because the generator is linear, the value of seed s + k is that
of seed s plus k times one amount at every draw, modulo 1.

\image html random_streams_seeds.svg

The figure shows the first two values of 64 computations, each as a dot.
Seeded with the consecutive seeds 42 to 105, their values lie on straight
lines, since every seed adds the same step to the value of the seed before it.
As batches 0 to 63 of one layout of seed 42, with a million units of a million
draws each, their values scatter. Computations meant to be independent
therefore share one seed and use the batches of one layout, whose strides do
not overlap.

## Migrating from the shared generator {#gislib_random_streams_migrating}

The shared generator, `G_srand48()` and `G_drand48()`, is unchanged. Code
which moves from it to a generator of its own changes as follows:

| before | after | values |
| --- | --- | --- |
| `G_srand48(seed)`, then `G_drand48()` | `G_random_state_from_seed(&rng, seed)`, then `G_random_double(&rng)` | the same |
| `G_srand48_auto()` | `G_random_generate_seed()`, then as above | the same seed |
| `G_drand48()` in a parallel loop, with a known number of draws per unit | an exact layout and `G_random_state_for_unit()` for every unit | those of one thread, for any number of threads |
| `G_drand48()` in a parallel loop, with a varying number of draws per unit | a bounded or whole-span layout | change once |
| `G_lrand48()` | `(long)(G_random_double(&rng) * 2147483648.0)` | the same |
| `G_mrand48()` | `(unsigned int)(G_random_double(&rng) * 4294967296.0)` | the same bits; for the signed value, subtract 2^32 from 2^31 and above |

The functions refuse a seed outside their range, which `G_srand48()` reduced
to its low 32 bits without a word, so a tool which passes a user's seed
through now refuses seeds it once accepted, such as 5000000000. Validate the
seed right after parsing, before any work is done, by initializing the layout
or a state there.
