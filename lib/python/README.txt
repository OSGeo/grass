GRASS scripting tasks for Python provided by "grass.py".

Usage: "import grass"




def make_command(prog, flags = "", overwrite = False, quiet = False, verbose = False, **options):

Return a list of strings suitable for use as the args parameter to
Popen() or call(). Example:

>>> grass.make_command("g.message", flags = 'w', message = 'this is a warning')
['g.message', '-w', 'message=this is a warning']




def start_command(prog, flags = "", overwrite = False, quiet = False, verbose = False, **kwargs):

Returns a Popen object with the command created by make_command.
Accepts any of the arguments which Popen() accepts apart from "args"
and "shell". Example:

>>> p = grass.start_command("g.gisenv", stdout = subprocess.PIPE)
>>> print p
<subprocess.Popen object at 0xb7c12f6c>
>>> print p.communicate()[0]
GISDBASE='/opt/grass-data';
LOCATION_NAME='spearfish57';
MAPSET='glynn';
GRASS_DB_ENCODING='ascii';
GRASS_GUI='text';
MONITOR='x0';




def pipe_command(*args, **kwargs):

Passes all arguments to start_command, but also adds
"stdout = subprocess.PIPE". Returns the Popen object. Example:

>>> p = grass.pipe_command("g.gisenv")
>>> print p
<subprocess.Popen object at 0xb7c12f6c>
>>> print p.communicate()[0]
GISDBASE='/opt/grass-data';
LOCATION_NAME='spearfish57';
MAPSET='glynn';
GRASS_DB_ENCODING='ascii';
GRASS_GUI='text';
MONITOR='x0';




def run_command(*args, **kwargs):

Passes all arguments to start_command, then waits for the process to
complete, returning its exit code. Similar to subprocess.call(), but
with the make_command() interface.




def read_command(*args, **kwargs):

Passes all arguments to start_command, then waits for the process to
complete, returning its stdout (i.e. similar to shell "backticks").




def message(msg, flag = None):
def debug(msg):
def verbose(msg):
def info(msg):
def warning(msg):
def error(msg):

These all run g.message, differing only in which flag (if any) is used.




def fatal(msg):

Like error(), but also calls sys.exit(1).




def parser():

Interface to g.parser, intended to be run from the top-level, e.g.:

	if __name__ == "__main__":
	    options, flags = grass.parser()
	    main()

Thereafter, the global variables "options" and "flags" will be
dictionaries containing option/flag values, keyed by lower-case
option/flag names. The values in "options" are strings, those in
"flags" are Python booleans.




def tempfile():

Returns the name of a temporary file, created with g.tempfile.




def gisenv():

Returns the output from running g.gisenv (with no arguments), as a
dictionary. Example:

>>> env = grass.gisenv()
>>> print env['GISDBASE']
/opt/grass-data




def region():

Returns the output from running "g.region -g", as a dictionary. 
Example:

>>> region = grass.region()
>>> [region[key] for key in "nsew"]
['4928000', '4914020', '609000', '590010']
>>> (region['nsres'], region['ewres'])
('30', '30')




def use_temp_region():

Copies the current region to a temporary region with "g.region save=",
then sets WIND_OVERRIDE to refer to that region. Installs an atexit
handler to delete the temporary region upon termination.




def del_temp_region():

Unsets WIND_OVERRIDE and removes any region named by it.




def find_file(name, element = 'cell'):

Returns the output from running g.findfile as a dictionary. Example:

>>> result = grass.find_file('fields', element = 'vector')
>>> print result['fullname']
fields@PERMANENT
>>> print result['file']
/opt/grass-data/spearfish57/PERMANENT/vector/fields




def list_grouped(type):

Returns the output from running g.list, as a dictionary where the keys
are mapset names and the values are lists of maps in that mapset. 
Example:

>>> grass.list_grouped('rast')['PERMANENT']
['aspect', 'erosion1', 'quads', 'soils', 'strm.dist', ...




def list_pairs(type):

Returns the output from running g.list, as a list of (map, mapset)
pairs. Example:

>>> grass.list_pairs('rast')
[('aspect', 'PERMANENT'), ('erosion1', 'PERMANENT'), ('quads', 'PERMANENT'), ...




def list_strings(type):

Returns the output from running g.list, as a list of qualified names. 
Example:

>>> grass.list_strings('rast')
['aspect@PERMANENT', 'erosion1@PERMANENT', 'quads@PERMANENT', 'soils@PERMANENT', ...




def parse_color(val, dflt = None):

Parses the string "val" as a GRASS colour, which can be either one of
the named colours or an R:G:B tuple e.g. 255:255:255. Returns an
(r,g,b) triple whose components are floating point values between 0
and 1. Example:

>>> grass.parse_color("red")
(1.0, 0.0, 0.0)
>>> grass.parse_color("255:0:0")
(1.0, 0.0, 0.0)

