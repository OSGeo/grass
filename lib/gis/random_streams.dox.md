# Random Number Streams {#gislib_random_streams}

[TOC]

## Overview {#gislib_random_streams_overview}

The `G_random_*()` functions give a program random number generators of its
own and many streams of values from one seed which do not overlap. They
serve parallel computations, the runs of an ensemble, the components of a
model which must each be reproducible on their own, and library code which
must not change the random numbers of its caller.

A program builds one layout from its seed and places a state for every unit
of work, such as a row of a raster or a particle. The values of a unit are
then fixed by the seed, the layout and the unit's number, which allows
parallel code to give the same result with any number of threads.

The values come from drand48, the 48-bit linear congruential generator
behind the C function `drand48()` and behind `G_drand48()` in GRASS.

## Terms {#gislib_random_streams_terms}

- **Generator**: drand48. A draw replaces the 48-bit state x by x' = (a × x +
  c) mod 2^48, with the multiplier a = 0x5DEECE66D and the increment c = 0xB,
  and returns x' / 2^48. It passes through all 2^48 states before repeating,
  so the states form one ring and every draw moves one step along it.
- **Seed**: the integer which picks where drawing starts. Seed s starts at the
  state whose bits 16 to 47 are the low 32 bits of s and whose low 16 bits
  are 0x330E, as `srand48()` sets it. The values drawn one after another
  from there are the seed's sequence.
- **State**: a `struct G_random_state`, a position on the ring and nothing
  else, used by one thread at a time; copying it copies the position.
  `G_random_double()` moves it one step and returns the value there, and
  `G_random_advance()` moves it any number of draws in one jump.
- **Span**: the first 2^46 draws after the seed, a quarter of the ring. The
  library places everything within it (see \ref gislib_random_streams_span
  for why).
- **Layout**: a `struct G_random_layout`, holding where the seed's sequence
  starts, the number of units and the stride. A program builds it once,
  outside any parallel region, and only reads it afterwards.
- **Unit**: a piece of work with values of its own, such as a row of a
  raster, a particle, a point or an object, numbered from 0. Its **stream**
  is the values of the seed's sequence it draws: at most stride values from
  where the layout places it, one stream per unit and run.
- **Stride**: the number of draws from the start of one unit to the start of
  the next, and so the number of values a unit may draw.
  `G_random_layout_length()` returns it.
- **Run**: one pass of the whole computation, one stream per unit. Unit u of
  run r starts (r × units + u) × stride draws after the seed, so the runs
  follow one another along the span.

\image html random_streams.svg

The figure shows the ring of 2^48 states, the seed (the red dot) and the
span, the shaded quarter after the seed, with the arrow in the direction of
drawing. The ticks are one stride apart from the seed, and the last one marks
the end of the span. The dark arcs are the streams of units 0, 1 and 2, each
starting at a tick, and the black dot is a state in the stream of unit 1.
Every seed starts at a different place on the ring, and the span is always
the quarter after it.

## Layouts {#gislib_random_streams_layouts}

The three ways to initialize a layout differ in where the stride comes from:

- An exact layout,
  `G_random_init_layout_exact(&layout, seed, units, draws_per_unit)`, takes
  the stride as given, the number of values every unit draws. Unit u of run 0
  then draws values u × stride to (u + 1) × stride - 1 of the seed's
  sequence, counted from 0, so the units together reproduce that sequence,
  whichever order they are computed in.
- A bounded layout,
  `G_random_init_layout_bounded(&layout, seed, units, max_draws)`, takes a
  bound on what a unit draws and rounds it up to odd (see
  \ref gislib_random_streams_odd), for units whose draws vary.
- A spread layout, `G_random_init_layout_spread(&layout, seed, units)`,
  divides the span into an odd number of equal parts, as many as there are
  units or one more when that number is even, and gives every unit the
  longest stride the span allows, for one run only. The stride is the length
  of a part rounded down to odd (see \ref gislib_random_streams_distance and
  \ref gislib_random_streams_odd). A single unit keeps the whole span. A
  spread layout holds at most 2^20 units; a bounded layout serves more.

\image html random_streams_layouts.svg

The figure shows the three layouts of six units along the span, with what
each unit of run 0 draws in dark. In the exact layout, run 0 is the seed's
sequence and run 1 follows; in the bounded layout, each unit draws less than
its stride; in the spread layout, the six units take six of seven parts, one
run covers the span, and the last part stays unused.

### Runs {#gislib_random_streams_runs}

Runs are appended, never reserved: where a run lies depends on nothing but
the seed, the stride, the number of units and the run number, so a run
computed later lands where it would have landed now.
`G_random_state_for_run(&rng, &layout, run, unit)` places a state at the
start of the unit's stream in that run, and
`G_random_state_for_unit(&rng, &layout, unit)` is the same call with run 0.
Placing a state is an advance from the seed, which costs about as much as a
few dozen draws, however far from the seed the stream starts.

A layout answers two queries: `G_random_layout_runs(&layout)` returns the
number of runs that fit into the span, which is 0 when not even one run
fits, and `G_random_layout_length(&layout)` returns the stride. A program
compares the stride with the most values any of its units can draw, not the
average, and checks that its run fits; whether to refuse or to warn and
continue is its decision. Run 0 can always be placed, even when no run fits,
so a tool whose earlier versions drew the same values without complaint may
warn and continue, as the fragments below do; what their warning says is
explained in \ref gislib_random_streams_span.

## Usage {#gislib_random_streams_usage}

A unit is what the computation is naturally divided into and numbered by;
the number of units is their count or an upper bound on it, and unused units
cost nothing but their share of the span. Which pattern applies depends on
whether a unit draws its values in one go or over many steps.

In every pattern, the values a unit draws do not depend on the thread which
draws them. The result of the whole computation is then the same for any
number of threads when nothing else in it depends on the schedule, which
holds when every unit writes only its own output and not when threads add
into shared sums.

### A single sequence {#gislib_random_streams_single}

One sequence has one state, seeded with `G_random_seed(&rng, seed)` and
drawn from in a loop. It needs no layout, so there are no runs to check:

```c
// seed: the seed, a long long; n: how many values to draw; values: where
// they go.
struct G_random_state rng;

G_random_seed(&rng, seed);
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
if (G_random_layout_runs(&layout) < 1)
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
if (G_random_layout_runs(&layout) < 1)
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
allows from a spread layout,
`G_random_init_layout_spread(&layout, seed, items)`, placed in the same way,
and the program warns when the stride, `G_random_layout_length(&layout)`, is
below an estimate of what an item draws. With more items than a spread
layout accepts, the program chooses a bound itself, from what it expects an
item to draw, uses a bounded layout with that bound, and checks that the run
fits as in the fragment above.

### Ensembles {#gislib_random_streams_ensembles}

An ensemble is many runs of the same model under one seed, meant to be
independent replicates, and its members are the runs of the layout the
program uses for a single run. Whether they are computed in one process or
one per invocation, on one machine or many, does not change where a run lies.

A tool which supports ensembles needs one option, `run`, the run this
invocation computes, numbered from 1 and 1 by default, so that nothing
changes for a user who runs the tool once. No option for the number of runs
is needed: with an exact or bounded layout the stride is set by what a unit
draws, so a member does not need to know the size of the ensemble. A spread
layout holds a single run and so cannot serve an ensemble. The tool places
unit u of its run with
`G_random_state_for_run(&rng, &layout, run - 1, u)`, and before any work it
refuses a run beyond `G_random_layout_runs(&layout)`, with that number in the
message; run 1 of a layout into which no run fits is placed with the warning
of the fragments above, as earlier versions ran. It writes the seed and the
run into the history of its output, for example `random seed = 42, run 3`,
so that a member can be found and repeated.

The members must share one seed, so each member must be given it explicitly.
When `run` is given, the tool takes the seed from the seed option or, when
that is not set, from `GRASS_RANDOM_SEED`, which it reads itself with
`getenv()`, and refuses to run with neither. It does not call
`G_random_generate_seed()` in that case, since that function falls back to
sources the members do not share.

What must agree between the members is everything which decides the layout:
the seed and whatever sets the number of units and the stride, such as the
region and the tool's options. A member with a different layout can have
streams which share values with another member's, so the members should
differ only in the run number and the output names. The number of members is
limited by the runs that fit (see \ref gislib_random_streams_capacity).

## Capacity {#gislib_random_streams_capacity}

The number of runs that fit is the span, 2^46 draws, divided by the draws of
one run, units × stride, rounded down; a spread layout holds one run. A
million units drawing a million values each in an exact layout draw 10^12
values per run, so 70 runs fit on one seed, and a single run could hold
70,368,744 such units, about 70 million.

The library does not limit how many values a unit draws, and cannot, since it
does not know how many a unit will draw. A unit which draws past its stride
continues into the next unit's stream and draws the next unit's values. Where
the units stay within their strides, only run 0 of a layout into which no run
fits reaches past the span; \ref gislib_random_streams_span explains what it
draws there.

No layout and no way of seeding makes the span larger (see
\ref gislib_random_streams_span); more than it holds needs a generator with a
longer period behind the same calls.

## Quality of the values {#gislib_random_streams_quality}

### The generator and the span {#gislib_random_streams_span}

All the values come from drand48, the 48-bit linear congruential generator:
multiples of 2^-48, uniform in [0, 1). A unit's stream is taken from the one
sequence the generator has, so its values are as good as that sequence: fine
for simulations, sampling and Monte Carlo estimates, and not for
cryptography.

The multiplier a is 5 modulo 8, so a^(2^46) = 1 modulo 2^48 while a^(2^45) is
not, and, with this increment, 2^46 draws take every state x to x + 2^46
modulo 2^48. Positions 2^46, 2^47 or 3 × 2^46 draws apart therefore give
values which differ by exactly 0.25, 0.5 or 0.75 at every draw: the four
quarters of the ring hold the same values shifted by 0, 0.25, 0.5 and 0.75.
With seed 42 the generator draws 0.7445, 0.3427, 0.1111 at the seed and
0.9945, 0.5927, 0.3611 a quarter of the ring later, each value the seed's
plus 0.25, wrapping past 1. The other three quarters add no values of their
own, so the library places everything within the span, where no two
positions are 2^46 draws apart.

Run 0 of a layout into which no run fits reaches past the span, where the
generator gives the values of the positions 2^46 draws earlier shifted by a
constant. A tool which warns and continues in that case should say that
values beyond 2^46 draws repeat earlier values shifted by a constant, not
only that a limit was exceeded, as the fragments in
\ref gislib_random_streams_usage do.

Seeding computations separately does not give more room: more than 2^46
values drawn in total, however they are seeded, include two positions a
multiple of 2^46 draws apart, which give the same values or the same values
shifted by a constant, and computations seeded at random can overlap before
that.

\image html random_streams_twins.svg

The figure shows the seed and the positions a quarter, a half and three
quarters of the ring after it, with the first three values of seed 42 at
each: the seed's values plus 1/4, 1/2 and 3/4, wrapping past 1. The streams
of a layout lie within the span, the shaded quarter, and do not reach the
position a quarter of the ring after the seed.

### Relations by distance {#gislib_random_streams_distance}

A layout does not change the generator; it places the units' streams along
its one sequence. Quality can therefore be lost not within a stream but
between two positions in use, and what relates two positions is their
distance in draws. Two positions whose values differ by one constant at every
draw are a constant-shift twin, as positions 2^46 apart are. At 2^45,
a^(2^45) = 1 + 2^47 modulo 2^48, and the difference takes two values as the
lowest bit of the state alternates: positions 2^45 apart are an
alternating-shift twin. The general rule, for k from 0 to 46: a distance
divisible by 2^(46 - k) fixes the low 48 - k bits of the difference of the
two states and leaves the top k bits free, so the values differ by one of 2^k
constants, cycling with period 2^k. An odd distance fixes only the low two
bits, which every distance does, and the differences do not repeat within
the span.

A distance occurs in a layout when two positions in use, positions at which
some unit draws, lie exactly that far apart. In a layout drawing T values in
total, the distances which occur cover at most about 2T of the 2^46
distances within the span, so counting each multiple of 2^(46 - k) below
2^46 as occurring with a chance of 2T / 2^46 estimates how many of those
occur, the distances with 2^k or fewer constants, at
(2^k - 1) × 2T / 2^46. The estimate treats the distances which occur as
spread at random, which fits bounded and spread layouts, not run 0 of an
exact layout, whose positions in use are the first T draws of the seed's
sequence and so include every distance below T.

A spread layout is placed by dividing the span, so its units would land
exactly on these distances if the span were divided into an even number of
parts: the unit halfway along would start 2^45 draws after unit 0. With the
odd number of parts, the unit which starts nearest to 2^45 draws after unit 0
starts about half a stride from that position, and the unit nearest to 2^44
about a quarter of a stride, for every number of units a spread layout
accepts, so a spread unit meets those relations only after drawing about a
quarter of its stride. Rounding the stride down to odd leaves up to two draws
of every part unused, and that loss accumulates along the span; the limit on
the number of units keeps it within a few percent of a stride at 2^45 and
2^44.

Seed 42 draws 0.7445, 0.3427, 0.1111, 0.4223, 0.0811, 0.8564, 0.4988,
0.4788, and the table gives, for each distance, the part of the ring it
spans, the relation, the values drawn that distance after the seed minus
these, modulo 1, and the estimate above for T = 10^11. The row with k is the
general rule: the rows from 2^46 down are its cases k = 0, 1, 2, 6 and 10,
and an odd distance is its last case, k = 46.

| distance | part of the ring | relation | values drawn this distance after seed 42 minus seed 42's first eight, modulo 1 | estimated number of multiples of this distance below 2^46 which occur, (2^k - 1) × 2T / 2^46 for T = 10^11 |
| --- | --- | --- | --- | --- |
| 2^47 | 1/2 | constant-shift twin | 0.5000 at every draw | none: no two positions in the span are this far apart |
| 2^46 | 1/4 | constant-shift twin | 0.2500 at every draw | none: no two positions in the span are this far apart |
| 2^45 | 1/8 | alternating-shift twin | 0.6250, 0.1250 alternating | 0.003 |
| 2^44 | 1/16 | one of 4 constants, cycling | 0.8125, 0.5625, 0.3125, 0.0625 | 0.009 |
| 2^40 | 1/256 | one of 64 constants, cycling | 0.3633, 0.5977, 0.1445, 0.7539, ... | about 0.18 |
| 2^36 | 1/4,096 | one of 1,024 constants, cycling | 0.7727, 0.0999, 0.2590, 0.1096, ... | about 3 |
| 2^(46 - k), for k from 0 to 46 | 1/2^(k + 2) | one of 2^k constants, cycling | 2^k values, then the same again | (2^k - 1) × 2T / 2^46 |
| any odd distance | an odd multiple of 1/2^48 | the low two bits fixed, 2^46 constants which do not repeat within the span | no pattern | about 2T, that is all the distances which occur |

The coarsest relations are rare and exact, the finest common and invisible:
positions 2^36 apart have the low 38 bits of their difference fixed and the
top ten free, so their values look unrelated at the precision computations
use, and only a comparison of the low bits of the difference finds them.

### Odd strides {#gislib_random_streams_odd}

By the rule above, a distance divisible by 2^j and by no higher power of two
fixes the low j + 2 bits of the difference of two states. Units 2^i apart in
number start 2^i × stride apart; with an odd stride that distance is
divisible by 2^i and by no higher power of two, so units 1, 2 and 4 apart
have the low 2, 3 and 4 bits of their difference fixed. With an even stride,
say a bound of 1,024 taken as it is, every such distance carries the stride's
power of two on top, and the same units have the low 12, 13 and 14 bits
fixed. That is why the library keeps the stride odd: the spread layout rounds
down to odd and the bounded layout rounds the bound up to odd, so no caller
has to know the rule. An exact layout cannot round, since its units must draw
what the seed's sequence draws; it keeps that sequence's structure and is
never worse than the seed's sequence it reproduces, in which the same values
were the stride apart already.

### Relations between seeds {#gislib_random_streams_between_seeds}

The seed occupies bits 16 to 47 of the state, so two seeds 2^30 apart start
2^46 apart in state, and 2^46 draws add 2^46 to a state: seed 42 + 2^30
starts a quarter of the ring after seed 42 and draws 0.9945, 0.5927, 0.3611,
0.6723, seed 42's values plus one quarter, and seed 42 + 2^31 draws 0.2445,
0.8427, 0.6111, 0.9223, plus one half.

Consecutive seeds start 2^16 apart in state. Since the step is linear, states
x and x + d become a × x + c and a × x + a × d + c after one draw, so states
d apart are a^t × d apart after t draws, whatever x was. Computations seeded
seed, seed + 1, seed + 2 and so on are therefore not independent: at every
draw, the one seeded seed + k has the value of the one seeded seed plus k
times the same amount, modulo 1, so their values step by the same amount at
each draw instead of scattering. Seed 43 draws 0.6153, 0.0473, 0.8495, 0.8879
and seed 44 draws 0.4861, 0.7519, 0.5880, 0.3534, nothing in common with seed
42 at first sight, yet 43 minus 42 and 44 minus 43, modulo 1, are the same at
every draw: 0.8708, 0.7046, 0.7384, 0.4655 for the first four. Computations
meant to be independent are the runs of one layout, whose streams do not
overlap.

### Checking two sequences {#gislib_random_streams_checking}

All the relations above are of one kind, two sequences which are one sequence
shifted in value and possibly in time. Draw a hundred or so values from each
of two sequences, and then:

1. Compare at the same draw: subtract the values draw by draw, modulo 1. A
   constant difference is a constant-shift twin, two constants alternating an
   alternating-shift twin, and a difference which changes from draw to draw
   but is the same for another pair of seeds equally far apart marks seeds
   taken at a constant step.
2. Compare at lags: shift one sequence by d draws against the other and
   subtract again. A constant difference at lag d means the same values when
   the constant is 0, and otherwise a constant-shift twin, which no layout
   gives within the span.
3. Compare against the stride: two units of one layout have no values in
   common, at any lag, as long as each stays within its stride, and a unit
   which draws past its stride draws exactly the next unit's values.

The differences are exact: every value is a multiple of 2^-48, and a double
holds the difference of two such values without rounding, so the comparison
needs no tolerance.

## Seeds {#gislib_random_streams_seeds}

Any seed from -2^31 to 2^32 - 1 can be used. The generator uses the low 32
bits of the seed, so -1 and 4294967295 are the same seed. A process which
uses one seed, as every ordinary execution of a tool does, needs to know
nothing more: any seed in the range is as good as any other, and the layout
keeps the streams apart.

The relations between seeds matter only when several seeds are used
together: an ensemble with one seed per invocation, computations compared
after choosing their seeds by hand, or a script deriving seeds from one
another. Keeping all the seeds from 0 to 2^30 - 1 rules out pairs a multiple
of 2^30 apart; no range rules out seeds taken at a constant step. Seeds drawn
at random, or hashed from names, avoid both relations as far as this
generator allows, but not the overlap of computations seeded at random. The
sound arrangement is one seed for all of them, each using its own run of the
layout.

The library does not parse a seed option. Parse it into a `long long` with
`strtoll()`, since the parser checks an integer option only loosely and a
`long` does not hold the range on every platform. Refuse a string which
`strtoll()` does not consume whole, or for which it sets `errno` to `ERANGE`,
with an error naming the option and the value given. When the user gives no
seed, generate one with `G_random_generate_seed()`. It takes the value of
`GRASS_RANDOM_SEED`, or of `SOURCE_DATE_EPOCH` when that one is not set, an
empty value counting as not set, and otherwise hashes the time and the
process ID. A value which is not an integer or does not fit a `long long` is
a fatal error, a value from -2^31 to 2^32 - 1 is used as it is, and a value
outside is reduced to its low 32 bits with a warning. The result is always in
the accepted range, so it needs no validation. Record the seed the tool
used, for example in the history of the output map, so that the computation
can be repeated.

## Migrating from the shared generator {#gislib_random_streams_migrating}

The shared generator, `G_srand48()` and `G_drand48()`, is unchanged. Code
which seeds it and draws from it gets exactly the same values, the seed's
sequence, from a state seeded with `G_random_seed()` and drawn from with
`G_random_double()`, without moving the shared generator or being moved by
it:

```c
// seed: the seed, a long long; value: a double;
// rng: a struct G_random_state.
// Before: the shared generator.
G_srand48(seed);
value = G_drand48();

// After: a state of the program's own, giving the same values.
G_random_seed(&rng, seed);
value = G_random_double(&rng);
```

Parallel code which drew from the shared generator inside the parallel loop
depended on the schedule for which unit got which values. With an exact
layout, as in \ref gislib_random_streams_rows, each unit draws the values of
the seed's sequence it drew in a single-threaded run, so those results are
kept and no longer depend on the number of threads. Where the draws of a unit
vary, a bounded or spread layout replaces the exact one, and the values of a
seed change once.

`G_srand48_auto()` seeds the shared generator with the seed
`G_random_generate_seed()` returns, so code which generated a seed with it
calls `G_random_generate_seed()` instead and gets the same seed without
seeding the shared generator, though not necessarily the same value where
`long` has 32 bits, since `G_srand48_auto()` then returns a seed of 2^31 or
more as a negative number. `G_srand48()` silently reduced any seed to its low
32 bits, while the `G_random_*()` functions refuse a seed outside their
range, so a tool which passes a user's seed through now refuses seeds it once
accepted, such as 5000000000. Validate the seed right after parsing, before
any work is done, by initializing the layout there, or, when the layout's
counts are known only later, with `G_random_seed()` on a local state.

The `G_random_*()` functions include no integer-returning one. To get an
integer in [0, n), use `(long)(G_random_double(&rng) * n)`. With n = 2^31
this is exactly what `G_lrand48()` gives at the same draw, since the state
has 48 bits and a double holds the product without rounding. The value of
`G_mrand48()` is `(long long)(G_random_double(&rng) * 4294967296.0)` read
as a two's complement 32-bit integer, that is, with 4294967296 subtracted when
it is 2147483648 or more; code which cast `G_mrand48()` to `unsigned int`
gets the same value from
`(unsigned int)(G_random_double(&rng) * 4294967296.0)`.
