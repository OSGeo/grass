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
- **Unit**: a piece of work with its own stream of numbers, such as a row of a
  raster, a particle, a point or an object, numbered from 0.
- **Stride**: the number of draws from the start of one unit to the start of
  the next, and so the most values a unit may draw. `G_random_layout_length()`
  returns it.
- **Batch**: one use of all the units of a layout. A further batch gives every
  unit another stream of numbers, further along the span; a computation which
  needs each unit once uses batch 0 only.

\image html random_streams.svg

The figure shows the ring of states, the seed (the red dot) and the span, the
shaded quarter after the seed, with the arrow in the direction of drawing. The
ticks are one stride apart from the seed, and the last one marks the end of
the span. Units 0, 1 and 2 each start at a tick, the dark arcs are their draws
so far, and the black dot is the state of unit 1, the position at which it
draws next. Every seed starts at a different place on the ring, and the span
is always the quarter after it.

## Capacity and layouts {#gislib_random_streams_layouts}

The span holds 2^46 draws, about 70 trillion. A layout gives units × stride of
them to one batch, so the number of batches that fit is about the span divided
by units × stride. A million units drawing a million values each draw 10^12
values per batch, so 70 batches fit on one seed. The library does not check
how many values a unit draws: a unit which draws past its stride draws the
next unit's values. No layout and no way of seeding makes the span larger (see
\ref gislib_random_streams_span).

A layout gives every unit its own stream of numbers. A single sequence is the
simplest case, and the three ways to initialize a layout for more units differ
in where the stride comes from:

- A single sequence, `G_random_state_from_seed(&rng, seed)`, is for values
  drawn one after another in one place. Its one unit owns the whole span and
  draws the seed's sequence. It needs no `struct G_random_layout`: the
  function places the state at the seed.
- A whole-span layout, `G_random_init_layout(&layout, seed, units)`, is for
  units whose number of draws is not known in advance. It divides the span
  into an odd number of equal parts, as many as there are units or one more
  when that number is even, and gives every unit the longest stride the span
  allows, in a single batch. The stride is the length of a part rounded down
  to odd (see \ref gislib_random_streams_distance and
  \ref gislib_random_streams_odd). The layout takes at most 2^20 units, about
  a million, so that this rounding cannot shift a unit to the positions which
  the odd number of parts avoids; a bounded layout serves more.
- An exact layout,
  `G_random_init_layout_exact(&layout, seed, units, draws_per_unit)`, is for
  units which all draw the same, known number of values. That number is the
  stride, and the rest of the span is left to further batches. Unit u of batch
  0 draws values u × stride to (u + 1) × stride - 1 of the seed's sequence,
  counted from 0, so the units together reproduce that sequence, whichever
  order they are computed in.
- A bounded layout,
  `G_random_init_layout_bounded(&layout, seed, units, max_draws)`, is for
  units whose number of draws varies below a known bound. Its stride is that
  bound rounded up to odd (see \ref gislib_random_streams_odd); apart from the
  rounding, the units are placed as in an exact layout whose stride is the
  bound.

\image html random_streams_layouts.svg

The figure shows the single sequence, whose one unit owns the whole span, and
below it the layouts of six units, with what each unit of batch 0 draws in
dark. The whole-span layout divides the span into seven parts; the six units
take six of them, one batch covers the span, and the last part stays unused.
In the exact layout, the units of batch 0 draw the start of the seed's
sequence, the same values as the single sequence above them, and batch 1 draws
the values which follow. The bounded layout has the strides of an exact layout
whose stride is the bound, made odd, and each unit is expected to draw less
than its stride; the draw which rounding adds to a stride is shown much wider
than one draw is. Why the number of parts and the strides are odd is explained
in \ref gislib_random_streams_distance and \ref gislib_random_streams_odd.

The units of a layout can be placed more than once: a further batch gives
every unit another stream of numbers, and the batches follow one another along
the span. Batches start units × stride draws apart, or one more when that
number is even (see \ref gislib_random_streams_odd), and unit u starts u ×
stride draws after the start of its batch.
`G_random_state_for_unit(&rng, &layout, unit)` places a state at the start of
the unit in batch 0, `G_random_state_for_batch(&rng, &layout, batch, unit)`
does so in any batch, and `G_random_layout_batches(&layout)` returns the
number of batches that fit into the span. What a batch stands for is up to the
tool: a member of an ensemble (see \ref gislib_random_streams_ensembles),
another pass over the same units, or one of the processes of a model.
Computations which are to be compared or combined, such as the members of an
ensemble, share one seed and use different batches, not different seeds (see
\ref gislib_random_streams_between_seeds).

## Usage {#gislib_random_streams_usage}

A unit is what the computation is naturally divided into and numbered by;
the number of units is their count or an upper bound on it, and unused units
cost nothing but their share of the span. Which pattern applies depends on
whether a unit draws its values in one go or over many steps.

The seed comes from the user or is generated. A tool reads its seed option
with `G_random_seed_from_option()`, which refuses what is not an integer
from -2^31 to 2^32 - 1, naming the option. When the user gives no seed, `G_random_generate_seed()`
gives one, from `GRASS_RANDOM_SEED`, or else from `SOURCE_DATE_EPOCH`, or
else from the time and the process ID. The tool records the seed it used, for
example in the history of the output map, so that the computation can be
repeated. Seeds and counts are `int64_t`. Code which holds one includes
`<stdint.h>` itself rather than relying on `<grass/gis.h>` to bring it in,
and printing one in a message takes `PRId64` from `<inttypes.h>`.

Whatever the pattern, the tool checks that no unit can draw more than the
stride, which `G_random_layout_length(&layout)` returns, and that the batches
it uses fit. Batch 0 can be placed even when no batch fits, so that a tool may
warn and continue, as the fragments below do. Placing a state costs about as
much as a few dozen draws, wherever on the span it is placed.

The values a unit draws do not depend on the thread which draws them. The
result of the whole computation is then the same for any number of threads
when nothing else in it depends on the schedule, which holds when every unit
writes only its own output and not when threads add into shared sums.

### A single sequence {#gislib_random_streams_single}

One sequence has one state, placed with `G_random_state_from_seed(&rng, seed)`
and drawn from in a loop. Its one unit owns the whole span, so there is no
`struct G_random_layout` to initialize and there are no batches to check:

```c
// seed: the seed, an int64_t; n: how many values to draw; values: where
// they go.
struct G_random_state rng;

G_random_state_from_seed(&rng, seed);
for (int i = 0; i < n; i++)
    values[i] = G_random_double(&rng);
```

### Rows of a raster {#gislib_random_streams_rows}

When a computation goes by rows of a raster and every row draws a known number
of values, for example a fixed number for each cell, a row is a unit of an
exact layout, and each thread owns one state. The thread places its state at
the start of every row it takes, whatever the state held before, and draws the
row's values. Since the layout is exact, the rows together draw the seed's
sequence, so a computation which drew from one sequence row after row keeps
its values:

```c
#include <stdint.h>

#include <grass/gis.h>
#include <grass/glocale.h>

// seed: the seed, an int64_t; rows, cols: the region; draws_per_value: the
// number of random values drawn for one cell. The product is formed in
// int64_t, since it may not fit an int.
struct G_random_layout layout;

G_random_init_layout_exact(&layout, seed, rows,
                           (int64_t)cols * draws_per_value);
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
such as points or objects: each item is a unit, the thread places its state at
the start of the item when it takes it, and a bounded layout replaces the
exact one when the number of values varies under a known bound.

### Items drawn over time {#gislib_random_streams_items}

When a computation goes over items, such as particles, which draw a few values
at every one of many steps, each item is a unit which owns one state for the
whole computation. The state is placed once and drawn from at every step by
whichever thread handles the item in that step. It is never placed again,
since that would return it to the start of the item and draw the same values
in every step. The states therefore live in an array for the whole
computation, not in the thread's loop. When the values an item draws in a step
vary under a known bound, the item is a unit of a bounded layout:

```c
#include <inttypes.h>

#include <grass/gis.h>
#include <grass/glocale.h>

// seed: the seed, an int64_t; items, steps: the number of items and of time
// steps, as int64_t; max_draws_per_step: the most values an item draws in
// one step.
struct G_random_layout layout;
struct G_random_state *states = G_malloc(items * sizeof(*states));

G_random_init_layout_bounded(&layout, seed, items,
                             steps * max_draws_per_step);
if (G_random_layout_batches(&layout) < 1)
    G_warning(_("%" PRId64 " items over %" PRId64 " steps may draw more than "
                "2^46 random values; values beyond that repeat earlier values "
                "shifted by a constant"),
              items, steps);
for (int64_t i = 0; i < items; i++)
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

An ensemble is many runs of the same model under one seed. Each member draws
from a batch of its own of the layout the tool uses for a single run. A tool
which supports ensembles takes the number of the member, for example as an
option `run` numbered from 1, places unit u with
`G_random_state_for_batch(&rng, &layout, run - 1, u)`, and refuses a run
beyond `G_random_layout_batches(&layout)`. With an exact or bounded layout, a
member does not need to know the size of the ensemble; a whole-span layout
holds a single batch and so cannot serve one.

The members must share the seed and whatever else decides the layout, the
number of units and the stride, so they should differ only in the run number
and the output names. The number of members is limited by the batches that fit
(see \ref gislib_random_streams_layouts).

## Quality of the values {#gislib_random_streams_quality}

The values are those of the 48-bit linear congruential generator of
`drand48()`: multiples of 2^-48, uniform in [0, 1), fine for simulations,
sampling, and Monte Carlo estimates, but not for cryptography. A layout keeps
the streams of numbers of its units from overlapping. It does not make them
independent, since positions on the ring are related by their distance. The
table lists the issues, how the library avoids each of them, and what remains
for a tool to keep in mind; the sections below give the details.

| issue | how the library avoids it | what remains |
| --- | --- | --- |
| Positions a quarter of the ring, 2^46 draws, apart give the same values shifted by a constant. | Every layout lies within the span, the first 2^46 draws after the seed. | Batch 0 of a layout which does not fit into the span reaches past it; the tool warns. |
| A unit which started half or a quarter of the span, 2^45 or 2^44 draws, after another would draw related values. | A whole-span layout divides the span into an odd number of parts. It takes at most 2^20 units, so that rounding its stride cannot move a unit to those positions. | A unit reaches those positions after drawing half or a quarter of its stride. |
| Units an even stride apart draw related values at the same draw; with a stride divisible by 2^23, the values lie on lines. | A bounded layout rounds its bound up to odd, and a whole-span layout its stride down to odd. | The relation holds between draws whose numbers differ by one from unit to unit. An exact layout cannot round and keeps the structure of the seed's sequence. |
| Batches an even number of draws apart do the same. | Batches start an odd number of draws apart. | The relation holds between draws whose numbers differ by one from batch to batch. |
| Consecutive seeds give values on lines, and seeds 2^30 apart give values shifted by a constant. | One seed serves many computations through the batches of a layout. | Seeds which a user picks for separate computations. |

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

A relation at a large distance is easy to see in the values but rarely occurs,
while one at a small distance occurs in most layouts but cannot be seen in the
values at the precision computations use. The strongest relations within the
span are at half and at a quarter of it, and a whole-span layout keeps its
units from starting there by dividing the span into an odd number of parts.
Its stride is rounded down to odd, which loses up to two draws per unit; the
limit of 2^20 units keeps those losses from adding up to a shift which would
bring a unit to one of those positions.

\image html random_streams_parts.svg

The figure shows a whole-span layout of six units, with seven parts, and in
grey what six parts would give: unit 3 would start at half the span, and its
values would be related to those of unit 0 from its first draw. With seven
parts, unit 3 reaches that position only after drawing half of its stride.

### Odd strides {#gislib_random_streams_odd}

When units start an even number of draws apart, the values which they draw at
the same draw are related across the units: the more often 2 divides the
distance, the simpler the pattern, and with a stride divisible by 2^23 the
values of the units lie on straight lines, as those of consecutive seeds do.
The library therefore keeps the distances odd: a bounded layout rounds its
bound up to odd, a whole-span layout rounds its stride down to odd, and
batches start an odd number of draws apart. The same draw of different units,
or of different batches, is then as unrelated as the generator allows.

\image html random_streams_strides.svg

The figure shows the first value of each of 64 units, and below them of each
of 64 batches, against its number. With an even stride of 2^24, which the
library does not make, the values of the units lie on lines (grey). With the
odd stride which the library makes of a bound of 2^24, they scatter (blue).
The relation is not gone, however: it holds between draws whose numbers fall
by one from unit to unit, here draw 64 of unit 0, draw 63 of unit 1 and so on
(red). The same holds from batch to batch, shown for a batch of 2^25 draws. A
tool meets the relation only where it compares or combines such draws.

An exact layout cannot round its stride, since its units must draw what the
seed's sequence draws. It keeps that sequence's structure, no better and no
worse.

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

The figure shows the first value of each of 64 computations against the number
of the computation. Seeded 0 to 63, the values lie on straight lines (red),
since every seed adds the same step to the value of the seed before it; the
same happens at every draw. As batches 0 to 63 of one layout of seed 0, with a
million units of a million draws each, the values scatter (blue). Computations
which are to be compared or combined therefore share one seed and use the
batches of one layout.

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
