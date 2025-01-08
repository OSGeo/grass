<h2>DESCRIPTION</h2>

<em>g.message</em> prints a message, warning, progress info, or fatal error
in the GRASS GIS way.

This program is to be used in Shell/Perl/Python scripts, so the author does not
need to use the <code>echo</code> program. The advantage of <em>g.message</em> is
that it formats messages just like other GRASS modules do and that its
functionality is influenced by the <code>GRASS_VERBOSE</code> and
<code>GRASS_MESSAGE_FORMAT</code> environment variables.

<p>
The program can be used for standard informative messages as well as warnings
(<b>-w</b> flag) and fatal errors (<b>-e</b> flag). For debugging
purposes, the <b>-d</b> flag will cause <em>g.message</em> to print a debugging
message at the given level.

<h2>NOTES</h2>

Messages containing "<code>=</code>" must use the full <b>message=</b> syntax so
the parser doesn't get confused.
<p>
If you want a long message (multi-line) to be dealt with as a single
paragraph, use a single call to <em>g.message</em> with text split in the
script using the backslash as the last character. (In shell scripts don't
close the "quote")
<p>
A blank line may be obtained with
<div class="code"><pre>
g.message message=""
</pre></div>
<p>
Redundant whitespace will be stripped away.
<p>
It's advisable to single quote the messages that are to be printed literally.
It prevents a number of characters (most notably, space and the dollar sign
'<code>$</code>') from being treated specifically by the shell.
<p>
When it is necessary to include, for example, a variable's value as part of
the message, the double quotes may be used, which do not deprive the
dollar sign of its special variable-expansion powers.
<p>
While it is known that the interactive Bash instances may treat the
exclamation mark '<code>!</code>' character specifically (making single quoting
of it necessary), it shouldn't be the case for the non-interactive
instances of Bash. Nonetheless, to avoid context-based confusion later on
you are encouraged to single-quote messages that do not require
<code>$VARIABLE</code> expansion.

<h3>Usage in Python scripts</h3>

<a href="https://grass.osgeo.org/grass-devel/manuals/libpython/">GRASS
Python Scripting Library</a> defines special wrappers
for <em>g.message</em>.

<ul>
  <li><code>debug()</code> for <code>g.message -d</code></li>
  <li><code>error()</code> for <code>g.message -e</code></li>
  <li><code>fatal()</code> for <code>g.message -e</code> + <code>exit()</code></li>
  <li><code>info()</code> for <code>g.message -i</code></li>
  <li><code>message()</code> for <code>g.message</code></li>
  <li><code>verbose()</code> for <code>g.message -v</code></li>
  <li><code>warning()</code> for <code>g.message -w</code></li>
</ul>

<p>
Note: The Python tab in the <em>wxGUI</em> can be used for entering the
following sample code:
<p>

<div class="code"><pre>
import grass.script as gcore

gcore.warning("This is a warning")
</pre></div>

is identical with

<div class="code"><pre>
g.message -w message="This is a warning"
</pre></div>

<h3>VERBOSITY LEVELS</h3>
Controlled by the "<code>GRASS_VERBOSE</code>" environment variable. Typically this
is set using the <b>--quiet</b> or <b>--verbose</b> command line options.
<ul>
<li>0 - only errors and warnings are printed</li>
<li>1 - progress messages are printed</li>
<li>2 - all module messages are printed</li>
<li>3 - additional verbose messages are printed</li>
</ul>

<h3>DEBUG LEVELS</h3>
Controlled by the "<code>DEBUG</code>" GRASS <i>gisenv</i> variable (set with
<em><a href="g.gisenv.html">g.gisenv</a></em>).
<br>
Recommended levels:
<ul>
<li>1 - message is printed once or few times per module</li>
<li>3 - each row (raster) or line (vector)</li>
<li>5 - each cell (raster) or point (vector)</li>
</ul>

<h2>EXAMPLES</h2>

This basic example prints the message "hello" in the console:

<div class="code"><pre>
g.message message="hello"
</pre></div>

<p>
To print a message as an error message use the <b>-e</b> flag:

<div class="code"><pre>
g.message -e message="my error"
</pre></div>

<p>
To print a message highlighted as a debug message ("D0/0: debug") in the
console, use the <b>-d</b> flag. Optionally the debug level can be defined (see
also <a href="g.gisenv.html">g.gisenv</a> for details):

<div class="code"><pre>
# Levels: (recommended levels)
#   0 - silence
#   1 - message is printed once or few times per module
#   3 - each row (raster) or line (vector)
#   5 - each cell (raster) or point (vector)
g.message -d message="debug" debug=0
</pre></div>

<p>
To print a message highlighted as a warning message ("WARNING: my warning")
in the console, use the <b>-w</b> flag:

<div class="code"><pre>
g.message -w message="my warning"
</pre></div>

<h2>SEE ALSO</h2>

<em>
<a href="variables.html">GRASS variables and environment variables</a>
</em>
<br>
<em>
<a href="g.gisenv.html">g.gisenv</a>,
<a href="g.parser.html">g.parser</a>
</em>

<h2>AUTHOR</h2>

Jachym Cepicky
