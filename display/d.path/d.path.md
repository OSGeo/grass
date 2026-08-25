## DESCRIPTION

*d.path* enables shortest path vector networking. Costs may be either
line lengths, or attributes saved in a database table. Supported are
cost assignments for both arcs and nodes, and also different in both
directions of a vector line. For areas cost will be calculated along
boundary lines.

## NOTE

The user needs to display a vector map before using d.path. If no
graphics monitor is open, a file `map.png` is generated in the current
directory.

The 'from' and 'to' points are entered by mouse into the map displayed
in the GRASS monitor, or if the **coordinates** option is used they can
be specified non-interactively. The actions bound to the mouse buttons
are described in the terminal window when running the command.

To calculate shortest path non-interactively and save the path to a new
vector map, use the *v.net.path* module.

## EXAMPLES

Interactive shortest path routing on road network (North Carolina sample
dataset):

```sh
g.region vector=roadsmajor -p
d.vect roadsmajor
d.path roadsmajor coordinates=668646.15,224447.16,668348.83,235894.02
```

Non-interactive shortest path routing on road network (North Carolina
sample dataset):

```sh
d.path -b roadsmajor coordinates=668646.15,224447.16,668348.83,235894.02
```

## SEE ALSO

*[v.net.path](v.net.path.md)*

## AUTHOR

Radim Blazek, ITC-Irst, Trento, Italy
