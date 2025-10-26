## DESCRIPTION

*t.rast.colors* computes a color table based on all registered maps of a
space time raster dataset and to assign this color table to each map.
Hence the created color table reflects the data range of the space time
raster dataset. This module is a simple wrapper around
[r.colors](r.colors.md). All options of *r.colors* are supported.
Internally a file with map names is created and passed to the *file*
option of *r.colors*.

Please have a look at the [r.colors](r.colors.md) manual page for
further information.

## EXAMPLE

Set Celsius color table to monthly dataset

```sh
t.rast.colors input=tempmean_monthly color=celsius

r.colors.out map=2009_01_tempmean

-80 0:0:40
-40 91:10:168
-30 220:220:220
-25 91:50:128
-20 50:0:150
-15 4:25:130
-10 8:54:106
-8 4:20:150
-5 0:50:255
-2 8:10:118
-0.061449 8:113:155
0.013855 8:118:157
0.090594 8:123:158
0.168767 8:128:159
0.248375 8:133:159
0.329417 8:136:160
...
40.6976 183:163:163
41.0483 185:167:167
41.4004 188:170:171
41.754 190:174:174
42.109 191:178:178
42.4655 193:182:182
42.8233 195:185:185
43.1827 197:189:189
43.5434 199:192:193
43.9056 200:197:197
44.2692 202:200:201
44.6342 204:204:204
45.0007 206:206:206
80 155:10:155
nv 255:255:255
default 255:255:255
```

## SEE ALSO

*[r.colors](r.colors.md)*

## AUTHOR

Sören Gebbert, Thünen Institute of Climate-Smart Agriculture
