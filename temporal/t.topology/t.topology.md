## DESCRIPTION

The module *t.topology* lists temporal relations of the maps in a space
time dataset.

## EXAMPLE

In order to obtain information about space time dataset topology, run:

```sh
t.topology tempmean_monthly@climate_2009_2012
 +-------------------- Basic information -------------------------------------+
 | Id: ........................ tempmean_monthly@climate_2009_2012
 | Name: ...................... tempmean_monthly
 | Mapset: .................... climate_2009_2012
 | Creator: ................... lucadelu
 | Temporal type: ............. absolute
 | Creation time: ............. 2014-11-28 10:40:55.060096
 | Modification time:.......... 2014-11-28 16:08:42.166628
 | Semantic type:.............. mean
 +-------------------- Temporal topology -------------------------------------+
 | Is subset of dataset: ...... False
 | Temporal topology is: ...... valid
 | Number of intervals: ....... 48
 | Invalid time stamps: ....... 0
 | Number of points: .......... 0
 | Number of gaps: ............ 0
 | Granularity: ............... 1 month
 +-------------------- Topological relations ---------------------------------+
 | Overlaps: .................. 0
 | Overlapped: ................ 0
 | Finishes: .................. 0
 | Started: ................... 0
 | Follows: ................... 47
 | Contains: .................. 0
 | Equal:...................... 0
 | Finished: .................. 0
 | Precedes: .................. 47
 | Starts: .................... 0
 | During: .................... 0
 +----------------------------------------------------------------------------+
```

## SEE ALSO

*[t.create](t.create.md), [t.info](t.info.md)*

## AUTHOR

Sören Gebbert, Thünen Institute of Climate-Smart Agriculture
