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

The generator is the one of the C function `drand48()` and of `G_drand48()` in
GRASS, a 48-bit linear congruential generator.

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
  starts, the number of units and the stride. A tool builds it once,
  outside any parallel region, and only reads it afterwards.
- **Unit**: a piece of work with values of its own, such as a row of a raster,
  a particle, a point or an object, numbered from 0. Its **stream** is the
  values of the seed's sequence it draws: at most stride values from where the
  layout places it, one stream per unit and batch.
- **Stride**: the number of draws from the start of one unit to the start of
  the next, and so the number of values a unit may draw.
  `G_random_layout_length()` returns it.
- **Batch**: one stream for every unit of a layout. Unit u of batch b starts
  (b × units + u) × stride draws after the seed, so the batches follow one
  another along the span. A computation which needs one stream per unit uses
  batch 0 only.

\image html random_streams.svg

The figure shows the ring of 2^48 states, the seed (the red dot) and the span,
the shaded quarter after the seed, with the arrow in the direction of drawing.
The ticks are one stride apart from the seed, and the last one marks the end
of the span. Units 0, 1 and 2 each start at a tick, the dark arcs are their
draws so far, and the black dot is the state of unit 1, the position at which
it draws next. Every seed starts at a different place on the ring, and the
span is always the quarter after it.

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

### Batches {#gislib_random_streams_batches}

A batch is one stream for every unit. What it stands for is up to the tool: a
member of an ensemble, another pass over the same units, or one of the
processes of a model. `G_random_state_for_batch(&rng, &layout, batch, unit)`
places a state at the start of the unit's stream in that batch, and
`G_random_state_for_unit(&rng, &layout, unit)` does so in batch 0. A batch
needs no reservation, since where it lies depends only on the layout and its
number, and placing a state costs about as much as a few dozen draws wherever
the stream starts.

`G_random_layout_batches(&layout)` returns the number of batches that fit into
the span and `G_random_layout_length(&layout)` the stride. A tool checks that
no unit can draw more than the stride and that the batches it uses fit. Batch
0 can be placed even when no batch fits, so that a tool may warn and continue,
as the fragments below do.

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

The library does not limit how many values a unit draws, and cannot, since it
does not know how many a unit will draw. A unit which draws past its stride
continues into the next unit's stream and draws the next unit's values. Where
the units stay within their strides, only batch 0 of a layout into which no
batch fits reaches past the span; \ref gislib_random_streams_span explains
what it draws there.

No layout and no way of seeding makes the span larger (see
\ref gislib_random_streams_span); more than it holds needs a generator with a
longer period behind the same calls.

## Quality of the values {#gislib_random_streams_quality}

### The generator and the span {#gislib_random_streams_span}

The generator is the 48-bit linear congruential generator of `drand48()`. Its
values are multiples of 2^-48, uniform in [0, 1): fine for simulations,
sampling, and Monte Carlo estimates, but not for cryptography.

The four quarters of the ring hold the same values shifted by 0, 0.25, 0.5 and
0.75: two positions a quarter of the ring apart give values which differ by
exactly 0.25 at every draw. Three of the quarters therefore add no values of
their own, and the library places everything within the span, where no two
positions are that far apart.

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

A layout does not change the generator; it places the units' streams along its
one sequence. Quality can therefore be lost not within a stream but between
two positions in use, and what relates two positions is their distance in
draws. A distance divisible by 2^(46 - k) leaves only the top k bits of the
difference of the two states free, so the values drawn at the two positions
differ by one of 2^k constants in turn. With one constant the two positions
are a constant-shift twin, and with two an alternating-shift twin. The table
lists the relations from the largest distance down.

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

The differences are those between the values drawn that distance apart from
seed 42, modulo 1. The expected occurrence is the number of distances with the
row's relation expected between the positions a computation uses when it draws
T = 10^11 values in total, for example a million units drawing 100,000 values
each; below 1, it reads as a chance. Such a computation has at most about 2T
of the 2^46 distances within the span between its positions, and 2^(k - 1) of
all the distances have the relation of the row with k. That row is the general
rule, of which the rows from 2^45 down are cases and an odd distance the last
one.

What a tool should be aware of:

- A relation at a large distance is exact and easy to see in the values, but a
  layout rarely has two positions that far apart.
- A relation at a small distance occurs in most layouts and cannot be seen in
  the values at the precision computations use; only a comparison of the low
  bits of the differences finds it.
- The expected occurrence grows in proportion to the number of values drawn in
  total, so the largest computations are the ones to look at.
- The estimate holds for bounded and whole-span layouts. Batch 0 of an exact
  layout draws the seed's sequence without gaps, so it has every distance up
  to its total number of draws, exactly as the single sequence it reproduces
  has.

What the library does about it:

- Every layout lies within the span, where no two positions are a quarter of
  the ring or more apart, so constant-shift twins do not occur.
- A whole-span layout divides the span into an odd number of parts, so that no
  unit starts at half or at a quarter of the span. The units nearest to those
  positions start well inside a stride away from them, and the limit on the
  number of units keeps the rounding of the stride from moving a unit there.
- Bounded and whole-span layouts have odd strides, which leave the units
  related in the fewest low bits (see \ref gislib_random_streams_odd).

\image html random_streams_parts.svg

The figure shows a whole-span layout of six units, with seven parts, and in
grey what six parts would give: unit 3 would start at half the span, and its
values would be related to those of unit 0 from its first draw. With seven
parts, half the span lies in the middle of the stride of unit 3, which reaches
that position only after drawing half of its stride.

### Odd strides {#gislib_random_streams_odd}

By the rule above, a distance divisible by 2^j and by no higher power of two
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

The seed occupies bits 16 to 47 of the state, so two seeds 2^30 apart start
2^46 apart in state, and 2^46 draws add 2^46 to a state: seed 42 + 2^30
starts a quarter of the ring after seed 42 and draws 0.9945, 0.5927, 0.3611,
0.6723, seed 42's values plus one quarter, and seed 42 + 2^31 draws 0.2445,
0.8427, 0.6111, 0.9223, plus one half.

Consecutive seeds start 2^16 apart in state. Since the step is linear, states
x and x + d become a × x + c and a × x + a × d + c after one draw, so states d
apart are a^t × d apart after t draws, whatever x was. Computations seeded
seed, seed + 1, seed + 2 and so on are therefore not independent: at every
draw, the one seeded seed + k has the value of the one seeded seed plus k
times the same amount, modulo 1, so their values step by the same amount at
each draw instead of scattering. Seed 43 draws 0.6153, 0.0473, 0.8495, 0.8879
and seed 44 draws 0.4861, 0.7519, 0.5880, 0.3534, nothing in common with seed
42 at first sight, yet 43 minus 42 and 44 minus 43, modulo 1, are the same at
every draw: 0.8708, 0.7046, 0.7384, 0.4655 for the first four. Computations
meant to be independent use the batches of one layout, whose streams do not
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

The relations between seeds matter only when several seeds are used together:
an ensemble with one seed per invocation, computations compared after choosing
their seeds by hand, or a script deriving seeds from one another. Keeping all
the seeds from 0 to 2^30 - 1 rules out pairs a multiple of 2^30 apart; no
range rules out seeds taken at a constant step. Seeds drawn at random, or
hashed from names, avoid both relations as far as this generator allows, but
not the overlap of computations seeded at random. The sound arrangement is one
seed for all of them, each using its own batch of the layout.

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
sequence, from a state placed with `G_random_state_from_seed()` and drawn from
with `G_random_double()`, without moving the shared generator or being moved
by it:

```c
// seed: the seed, a long long; value: a double;
// rng: a struct G_random_state.
// Before: the shared generator.
G_srand48(seed);
value = G_drand48();

// After: a state of the tool's own, giving the same values.
G_random_state_from_seed(&rng, seed);
value = G_random_double(&rng);
```

Parallel code which drew from the shared generator inside the parallel loop
depended on the schedule for which unit got which values. With an exact
layout, as in \ref gislib_random_streams_rows, each unit draws the values of
the seed's sequence it drew in a single-threaded run, so those results are
kept and no longer depend on the number of threads. Where the draws of a unit
vary, a bounded or whole-span layout replaces the exact one, and the values of
a seed change once.

`G_srand48_auto()` seeds the shared generator with the seed
`G_random_generate_seed()` returns, so code which generated a seed with it
calls `G_random_generate_seed()` instead and gets the same seed without
seeding the shared generator, though not necessarily the same value where
`long` has 32 bits, since `G_srand48_auto()` then returns a seed of 2^31 or
more as a negative number. `G_srand48()` silently reduced any seed to its low
32 bits, while the `G_random_*()` functions refuse a seed outside their range,
so a tool which passes a user's seed through now refuses seeds it once
accepted, such as 5000000000. Validate the seed right after parsing, before
any work is done, by initializing the layout there, or, when the layout's
counts are known only later, with `G_random_state_from_seed()` on a local
state.

The `G_random_*()` functions include no integer-returning one. To get an
integer in [0, n), use `(long)(G_random_double(&rng) * n)`. With n = 2^31
this is exactly what `G_lrand48()` gives at the same draw, since the state
has 48 bits and a double holds the product without rounding. The value of
`G_mrand48()` is `(long long)(G_random_double(&rng) * 4294967296.0)` read
as a two's complement 32-bit integer, that is, with 4294967296 subtracted when
it is 2147483648 or more; code which cast `G_mrand48()` to `unsigned int`
gets the same value from
`(unsigned int)(G_random_double(&rng) * 4294967296.0)`.
