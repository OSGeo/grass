<h2>DESCRIPTION</h2>

The <em>v.dissolve</em> module is used to merge adjacent or overlapping
features in a vector map that share the same category value. The
resulting merged feature(s) retain this category value.

<center>
<img src="v_dissolve_zipcodes.png">
<img src="v_dissolve_towns.png">
<p><em>
Figure: Areas with the same attribute value (first image) are merged
into one (second image).
</em></p>
</center>

<p>

Instead of dissolving features based on the category values, the user
can define an integer or string column using the <b>column</b>
parameter. In that case, features that share the same value in that
column are dissolved. Note, the newly created layer does not retain the
category (cat) values from the input layer.

<p>
Note that multiple areas with the same category or the same attribute
value that are not adjacent are merged into one entity, which consists
of multiple features, i.e., a multipart feature.

<h3>Attribute aggregation</h3>

The attributes of merged areas can be aggregated using various
aggregation methods. The specific methods available depend on the
backend used for aggregation. Two aggregate backends (specified with
the <b>aggregate_backend</b> parameter) are available, <em>univar</em>
and <em>sql</em>. The backend is determined automatically based on the
requested methods. When the function is one of the <em>SQL</em>
build-in aggregate functions, the <em>sql</em> backend is used.
Otherwise, the <em>univar</em> backend is used.

<p>
The default behavior is intended for interactive use and
testing. For scripting and other automated usage, explicitly specifying
the backend with the <b>aggregate_backend</b> parameter is strongly
recommended. When choosing, note that the <em>sql</em> aggregate
backend, regardless of the underlying database, will typically perform
significantly better than the <em>univar</em> backend.

<h4>Aggregation using univar backend</h4>

When <em>univar</em> is used, the methods available are the ones which
<em>v.db.univar</em> uses by default, i.e., <em>n</em>, <em>min</em>,
<em>max</em>, <em>range</em>, <em>mean</em>, <em>mean_abs</em>,
<em>variance</em>, <em>stddev</em>, <em>coef_var</em>, and
<em>sum</em>.

<h4>Aggregation using sql backend</h4>

When the <em>sql</em> backend is used, the methods depend on the SQL
database backend used for the attribute table of the input vector. For
SQLite, there are at least the following
<a href="https://www.sqlite.org/lang_aggfunc.html">built-in aggregate
functions</a>: <em>count</em>, <em>min</em>, <em>max</em>,
<em>avg</em>, <em>sum</em>, and <em>total</em>.

For PostgreSQL, the list of
<a href="https://www.postgresql.org/docs/current/functions-aggregate.html">aggregate
functions</a> is much longer and includes, e.g., <em>count</em>,
<em>min</em>, <em>max</em>, <em>avg</em>, <em>sum</em>,
<em>stddev</em>, and <em>variance</em>.

<h4>Defining the aggregation method</h4>

If only the parameter <b>aggregate_columns</b> is provided, all the
following aggregation statistics are calculated: <em>n</em>,
<em>min</em>, <em>max</em>, <em>mean</em>, and <em>sum</em>. If the
<em>univar</em> backend is specified, all the available methods for the
<em>univar</em> backend are used.

<p>
The <b>aggregate_methods</b> parameter can be used to specify which
aggregation statistics should be computed. Alternatively, the parameter
<b>aggregate_columns</b> can be used to specify the method using SQL
syntax. This provides the highest flexibility, and it is suitable for
scripting. The SQL statement should specify both the column and the
functions applied, e.g.,

<div class="code"><pre>
<code>aggregate_columns="sum(cows) / sum(animals)"</code>.
</pre></div>

<p>
Note that when the <b>aggregate_columns</b> parameter is used, the
<i>sql</i> backend should be used. In addition, the
<b>aggregate_columns</b> and <b>aggregate_methods</b> cannot be used
together.

<p>
For convenience, certain methods, namely <em>n</em>, <em>count</em>,
<em>mean</em>, and <em>avg</em>, are automatically converted to the
appropriate name for the selected backend. However, for scripting, it
is recommended to specify the appropriate method (function) name for
the backend, as the conversion is a heuristic that may change in the
future.

<p>
If the <b>result_columns</b> is not provided, each method is applied to
each column specified by <b>aggregate_columns</b>. This results in a
column for each of the combinations. These result columns have
auto-generated names based on the aggregate column and method. For
example, setting the following parameters:

<div class="code"><pre>
aggregate_columns=A,B
aggregate_methods=sum,n
</pre></div>

<p>
results in the following columns: A_sum, A_n, B_sum, B_n. See
the Examples section.

<p>
If the <b>result_column</b> is provided, each method is applied only
once to the matching column in the aggregate column list, and the
result will be available under the name of the matching result column.
For example, setting the following parameter:

<div class="code"><pre>
aggregate_columns=A,B
aggregate_methods=sum,max
result_column=sum_a, n_b
</pre></div>

<p>
results in the column <i>sum_a</i> with the sum of the values of
<i>A</i> and the column <i>n_b</i> with the max of <i>B</i>. Note that
the number of items in <b>aggregate_columns</b>,
<b>aggregate_methods</b> (unless omitted), and <b>result_column</b>
needs to match, and no combinations are created on the fly. See
the Examples section.

<p>
For scripting, it is recommended to specify all resulting column names,
while for interactive use, automatically created combinations are
expected to be beneficial, especially for exploratory analysis.

<p>
The type of the result column is determined based on the method
selected. For <em>n</em> and <em>count</em>, the type is INTEGER and
for all other methods, it is DOUBLE. Aggregate methods that produce
other types require the type to be specified as part of the
<b>result_columns</b>. A type can be provided in <b>result_columns</b>
using the SQL syntax <code>name type</code>, e.g., <code>sum_of_values
double precision</code>. Type specification is mandatory when SQL
syntax is used in <b>aggregate_columns</b> (and
<b>aggregate_methods</b> is omitted).

<h2>NOTES</h2>

GRASS defines a vector area as a composite entity consisting of a set
of closed boundaries and a centroid. The centroids must contain a
category number (see <em>v.centroids</em>), this number is linked to
area attributes and database links.

<p>
Multiple attributes may be linked to a single vector entity through
numbered fields referred to as layers. Refer to <em>v.category</em> for
more details.

<p>
Merging of areas can also be accomplished using <code>v.extract -d</code>
which provides some additional options. In fact, <em>v.dissolve</em> is
simply a front-end to that module. The use of the <em>column</em>
parameter adds a call to <em>v.reclass</em> before.


<h2>EXAMPLES</h2>

<h3>Basic use</h3>
<div class="code"><pre>
v.dissolve input=undissolved output=dissolved
</pre></div>

<h3>Dissolving based on column attributes</h3>

North Carolina data set:

<div class="code"><pre>
g.copy vect=soils_general,mysoils_general
v.dissolve mysoils_general output=mysoils_general_families column=GSL_NAME
</pre></div>

<h3>Dissolving adjacent SHAPE files to remove tile boundaries</h3>

If tile boundaries of adjacent maps (e.g. CORINE Landcover SHAPE files)
have to be removed, an extra step is required to remove duplicated
boundaries:

<div class="code"><pre>
# patch tiles after import:
v.patch -e `g.list type=vector pat="clc2000_*" separator=","` out=clc2000_patched

# remove duplicated tile boundaries:
v.clean clc2000_patched out=clc2000_clean tool=snap,break,rmdupl thresh=.01

# dissolve based on column attributes:
v.dissolve input=clc2000_clean output=clc2000_final col=CODE_00
</pre></div>

<h3>Attribute aggregation</h3>

While dissolving, we can aggregate attribute values of the original features.
Let's aggregate area in acres (ACRES) of all municipal boundaries
(boundary_municp) in the full NC dataset while dissolving common boundaries
based on the name in the DOTURBAN_N column
(long lines are split with backslash marking continued line as in Bash):

<div class="code"><pre>
v.dissolve input=boundary_municp column=DOTURBAN_N output=municipalities \
    aggregate_columns=ACRES
</pre></div>

<p>
To inspect the result, we will use <em>v.db.select</em> retrieving only one row
for <code>DOTURBAN_N == 'Wadesboro'</code>:

<div class="code"><pre>
v.db.select municipalities where="DOTURBAN_N == 'Wadesboro'" separator=tab
</pre></div>

<p>
The resulting table may look like this:

<div class="code"><pre>
cat  DOTURBAN_N    ACRES_n    ACRES_min    ACRES_max    ACRES_mean    ACRES_sum
66   Wadesboro     2          634.987      3935.325     2285.156      4570.312
</pre></div>

<p>
The above created multiple columns for each of the statistics computed
by default. We can limit the number of statistics computed by specifying
the method which should be used:

<div class="code"><pre>
v.dissolve input=boundary_municp column=DOTURBAN_N output=municipalities_2 \
    aggregate_columns=ACRES aggregate_methods=sum
</pre></div>

<p>
The above gives a single column with the sum for all values in the ACRES column
for each group of original features which had the same value in the DOTURBAN_N
column and are now dissolved (merged) into one.

<h3>Aggregating multiple attributes</h3>

Expanding on the previous example, we can compute values for multiple columns
at once by adding more columns to the <b>aggregate_columns</b> option.
We will compute average of values in the NEW_PERC_G column:

<div class="code"><pre>
v.dissolve input=boundary_municp column=DOTURBAN_N output=municipalities_3 \
    aggregate_columns=ACRES,NEW_PERC_G aggregate_methods=sum,avg
</pre></div>

<p>
By default, all methods specified in the <b>aggregate_methods</b> are
applied to all columns, so result of the above is four columns. While
this is convenient for getting multiple statistics for similar columns
(e.g. averages and standard deviations of multiple population
statistics columns), in our case, each column is different and each
aggregate method should be applied only to its corresponding column.

<p>
The <em>v.dissolve</em> module will apply each aggregate method only to
the corresponding column when column names for the results are
specified manually with the <b>result_columns</b> option:

<div class="code"><pre>
v.dissolve input=boundary_municp column=DOTURBAN_N output=municipalities_4 \
    aggregate_columns=ACRES,NEW_PERC_G aggregate_methods=sum,avg \
    result_columns=acres,new_perc_g
</pre></div>

<p>
Now we have full control over what columns are created, but we also
need to specify an aggregate method for each column even when the
aggregate methods are the same:

<div class="code"><pre>
v.dissolve input=boundary_municp column=DOTURBAN_N output=municipalities_5 \
    aggregate_columns=ACRES,DOTURBAN_N,TEXT_NAME aggregate_methods=sum,count,count \
    result_columns=acres,number_of_parts,named_parts
</pre></div>

<p>
While it is often not necessary to specify aggregate methods or names
for interactive exploratory analysis, specifying both
<b>aggregate_methods</b> and <b>result_columns</b> manually is a best
practice for scripting (unless SQL syntax is used for
<b>aggregate_columns</b>, see below).

<h3>Aggregating using SQL syntax</h3>

The aggregation can be done also using the full SQL syntax and set of
aggregate functions available for a given attribute database backend.
Here, we will assume the default SQLite database backend for attribute.

<p>
Modifying the previous example, we will now specify the SQL aggregate
function calls explicitly instead of letting <em>v.dissolve</em>
generate them for us. We will compute sum of the ACRES column using
<code>sum(ACRES)</code> (alternatively, we could use SQLite specific
<code>total(ACRES)</code> which returns zero even when all values are
NULL). Further, we will count number of aggregated (i.e., dissolved)
parts using <code>count(*)</code> which counts all rows regardless of
NULL values. Then, we will count all unique names of parts as
distinguished by the MB_NAME column using <code>count(distinct
MB_NAME)</code>. Finally, we will collect all these names into a
comma-separated list using <code>group_concat(MB_NAME)</code>:

<div class="code"><pre>
v.dissolve input=boundary_municp column=DOTURBAN_N output=municipalities_6 \
    aggregate_columns="total(ACRES),count(*),count(distinct MB_NAME),group_concat(MB_NAME)" \
    result_columns="acres REAL,named_parts INTEGER,unique_names INTEGER,names TEXT"
</pre></div>

<p>
Here, <em>v.dissolve</em> doesn't make any assumptions about the
resulting column types, so we specified both named and the type of each
column.

<p>
When working with general SQL syntax, <em>v.dissolve</em> turns off its
checks for number of aggregate and result columns to allow for all SQL
syntax to be used for aggregate columns. This allows us to use also
functions with multiple parameters, for example specify separator to be
used with <em>group_concat</em>:

<div class="code"><pre>
v.dissolve input=boundary_municp column=DOTURBAN_N output=municipalities_7 \
    aggregate_columns="group_concat(MB_NAME, ';')" \
    result_columns="names TEXT"
</pre></div>

<p>
To inspect the result, we will use <em>v.db.select</em> retrieving only
one row for <code>DOTURBAN_N == 'Wadesboro'</code>:

<div class="code"><pre>
v.db.select municipalities_7 where="DOTURBAN_N == 'Wadesboro'" separator=tab
</pre></div>

<p>
The resulting table may look like this:

<div class="code"><pre>
cat	DOTURBAN_N	names
66	Wadesboro	Wadesboro;Lilesville
</pre></div>


<h2>SEE ALSO</h2>

<em>
<a href="v.category.html">v.category</a>,
<a href="v.centroids.html">v.centroids</a>,
<a href="v.extract.html">v.extract</a>,
<a href="v.reclass.html">v.reclass</a>,
<a href="v.db.univar.html">v.db.univar</a>,
<a href="v.db.select.html">v.db.select</a>
</em>

<h2>AUTHORS</h2>

M. Hamish Bowman, Department of Marine Science, Otago University, New Zealand (module)<br>
Markus Neteler (column support)<br>
Trevor Wiens (help page)<br>
Vaclav Petras, NC State University, Center for Geospatial Analytics, GeoForAll Lab (aggregate statistics)
