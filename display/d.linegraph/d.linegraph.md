## DESCRIPTION

*d.linegraph* is a module to draw simple x,y line graphs (plots) based
on numerical data contained in separate files.

### Data format

The X and Y data files for the graph are essentially a column of numbers
in each file, with one input number per line. The program expects that
each X value will have a corresponding Y value, therefore the number of
lines in each data input file should be the same. Essentially, the X
data becomes the X axis reference to which the Y data is plotted as a
line. Therefore, the X data should be a monotonically increasing
progression of numbers (i.e. "1,2,3,..."; "0, 10, 100, 1000,...";
"...-5,-1,0,1,5..."). If multiple Y data files are used, the Y axis
scale will be based on the range of minimum and maximum values from all
Y files, then all Y data given will be graphed according to that Y
scale. Therefore, if multiple Y data inputs are used with dissimilar
units, the graph produced comparing the two will be deceptive.

### File inputs

If the **directory** option is provided, the paths to files can (and
should) be only relative paths to these files. While this is not
recommended for scripting, it can be advantageous when typing the paths
manually. For example when all files are stored in the directory
`/home/john/data`, the user can provide the following in the command
line:

```sh
d.linegraph directory=/home/john/data x_file=x.txt y_file=y1.txt,y2.txt
```

### Managing colors

The user can specify the **y_color** option, the **color_table** option
or just leave the defaults to influence the color of the plotted lines.

Colors specified by **y_color** option are used for drawing the lines in
the graph. If multiple Y data files are used, an equal number of colors
may be used to control the colors of the lines. Colors will be assigned
to Y data in respect to the sequence of instantiation on the command
line. It can be one of GRASS GIS named colors or the RGB values from
0-255 separated by colons (RRR:GGG:BBB).

Alternatively, the user can use the **color_table** option to specify
one of the GRASS GIS predefined color tables.

By default, a series of colors will be chosen by the module if none are
provided upon invocation. The order of default colors is red, green,
violet, blue, orange, gray, brown, magenta, white, and indigo. The user
is advised not to rely on the order of default colors but to either use
the **y_color** or the **color_table** option to obtain predictable and
reproducible results.

The color to be used for titles, axis lines, tics, and scale numbers is
determined by the **title_color** option. The user can provide one of
the GRASS GIS named colors (such as gray, white, or black) or use the
GRASS GIS colon-separated format for RGB (RRR:GGG:BBB).

### Titles, labels, and tics

The **title** option specifies the text for the title of the graph. It
will be centered over the top of graph. The **x_title** option is a text
to describe data for X axis. It will be centered beneath the graph.
Default is no text unless there is a need for a unit descriptor
determined by the *d.linegraph* module, then string such as "in
hundreds" is generated. The **y_title** option is a text to describe
data for Y axis. It will be centered beneath the X data description.
Similarly, to the **x_title** option, default is no text unless there is
a need for an auto-generated description. In the case of graphs with
multiple lines (multiple inputs for Y axis), user may wish to use more
specific text placement using the *[d.text](d.text.md)* or
*[v.label](v.label.md)* programs.

## NOTES

For historical reasons, the *d.linegraph* module accepts titles of more
than one word where the underscore character ("\_") is used to represent
spaces (" "). For example "Census_data_1990" would be printed over the
graph as "Census data 1990". The use of underscores is not necessary to
use as long as the parameter is quoted in the command line. In general,
use of underscores is not recommended and there is no need to use it at
all in the GUI or when using *d.linegraph* in Python scripts.

The way the program locates and labels tic marks is less than perfect:  

1) although distances between Y tics are proportional to the value, they
are not proportional on the X axis;  
2) decimal values between -1 and 1 can be printed on the X axis, but not
on Y. (With respect to the later, the input for Y values can all be
multiplied by a factor of 10 before graphing).

Depending on the user's needs, it might be easier or more appropriate to
use a 3rd party tool such as xgraph, gnuplot, Matplotlib in Python, or R
instead of *d.linegraph*. For a more general solution for plotting in
GRASS GIS, the user is advised to use the *[d.graph](d.graph.md)*
module.

## EXAMPLE

The following can be executed in Bash to create the input data for this
example. The user can just create these files in a text editor, save
them and specify path to them.

```sh
cat > x.txt <<EOF
1
3
4
6
9
EOF
cat > y1.txt <<EOF
50
58
65
34
27
EOF
cat > y2.txt <<EOF
10
20
35
50
45
EOF
```

The next command sequence creates a file `plot.png` in the current
directory which is the drawing made by *d.linegraph*.

```sh
d.mon start=cairo output=plot.png width=400 height=400
d.linegraph x_file=x.txt y_file=y1.txt,y2.txt
d.mon stop=cairo
```

## SEE ALSO

*[d.frame](d.frame.md), [d.text](d.text.md), [v.label](v.label.md),
[d.graph](d.graph.md), [d.histogram](d.histogram.md)*

## AUTHOR

Chris Rewerts, Agricultural Engineering, Purdue University
