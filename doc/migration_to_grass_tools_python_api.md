# Migration guide: from *grass.script* command functions to *grass.tools*

## Overview

The *grass.tools* module provides a modern, more Pythonic interface
to GRASS tools compared to the traditional *grass.script* command functions
(*run_command*, *read_command*, *write_command*, and *parse_command*).
This guide helps you migrate existing code that uses these functions
to the new *grass.tools.Tools* object.

Note that this migration guide focuses specifically
on replacing the *grass.script.run_command* family of functions and their wrappers,
such as *list_strings*.
Other *grass.script* functionality, such as project and session management
(*gs.create_project*, *gs.setup.init*), was enhanced and no changes
in user code are required.

## Why migrate?

The *grass.script.run_command* family of functions stems from GRASS's
transition from Bash scripting to Python.
While functional, this approach has significant drawbacks:
the tool name is in a string parameter while generic function names
like *run_command* and *parse_command* dominate your code.
Different functions are required depending on whether you need
to capture output (*read_command*), parse results (*parse_command*),
or pipe input (*write_command*).
This forces you to think about the mechanics of calling tools
rather than focusing on the tools themselves.

The new *grass.tools* API centers everything around the tools.
Tool names become Python function names of a *Tools* object,
all parameters become keyword arguments, and you get consistent,
smart return values regardless of what the tool outputs.
The result is code that reads more naturally and requires less mental overhead
when reading as the tool names are more prominent
and special configuration is concentrated in one place.

Key advantages of *grass.tools*:

* *More Pythonic*: Function-style syntax with snake-case tool names (e.g., `tools.r_random_surface()`)
* *Powerful return values*: All tools return result objects with standardized attributes
* *Cleaner syntax*: No need to repeatedly pass the *env* parameter
* *JSON support*: Better support for JSON output format
* *Flexible input*: Direct support for Python objects beyond numbers and lists
  (e.g., `io.StringIO` or `np.array`)

## Basic syntax transformation

### Tool names as function names

The fundamental shift is from calling generic functions
with tool names as string parameters to calling tools directly as Python functions.

The *run_command* approach requires *run_command* with the tool name as a string:

```python
import grass.script as gs

gs.run_command("r.slope.aspect", elevation="elevation", slope="slope")
```

The new approach makes the tool name the function itself.
Create a *Tools* object once, then call tools using snake_case names
(dots become underscores):

```python
from grass.tools import Tools

tools = Tools()
tools.r_slope_aspect(elevation="elevation", slope="slope")
```

Notice how *r.slope.aspect* becomes *r_slope_aspect*,
and the tool name is now prominent in your code rather than hidden in a string.

If you need the string-based syntax
(for example, when the tool name is in a variable), you can use *tools.run()*:

```python
tools.run("r.slope.aspect", elevation="elevation", slope="slope")
tools.run(tool_name, **parameters)
```

However, the function syntax is recommended as your primary approach
because it provides better code completion support,
works better with generative AI, catches typos sooner, and reads more naturally.

### One object for all tool types

The *run_command* API requires different functions depending on what you needed
from the tool.
Running a tool that creates data outputs as files used *run_command*,
getting text output requires *read_command*,
piping input needed *write_command*, and parsing structured output demands *parse_command*.
You have to remember which function to use and switch
between them as your needs change.

The new API uses a single, unified approach. Every tool is called the same way,
and the result object provides access to everything you might need.
This means you don't need to change function names when you realize
you need to access the tool's output or when using a different tool.

### Special parameters in one place

In *grass.tools*, handling of special parameters is centralized.
For example, instead of passing an *env* parameter to every tool invocation:

```python
gs.run_command("g.region", raster="elevation", env=session.env)
gs.run_command("r.slope.aspect", elevation="elevation", slope="slope", env=session.env)
```

You configure the environment once when creating a `Tools` instance:

```python
tools = Tools(env=session.env)
tools.g_region(raster="elevation")
tools.r_slope_aspect(elevation="elevation", slope="slope")
```

This eliminates repetitive parameters in tool calls and makes workflows easier
to read and maintain.
The following examples will not use the explicit session setting,
assuming an existing global session,
but the *Tools* API makes it easy to add an explicit session in one place.

## Replacing different command functions

### From *run_command* to direct tool calls

When you just need to run a tool and create outputs, the migration is straightforward.
The *run_command* syntax:

```python
gs.run_command("r.random.surface", output="surface", seed=42)
```

Becomes:

```python
tools = Tools()
tools.r_random_surface(output="surface", seed=42)
```

Or with the string syntax:

```python
tools.run("r.random.surface", output="surface", seed=42)
```

### From *read_command* to result attributes

The *read_command* function captures a tool's standard output as a string.
With *grass.tools*, every tool call returns a result object,
and you access the output through its attributes.

The *run_command* family way to get text output:

```python
result = gs.read_command("g.region", flags="c")
# Returns: "center easting:  0.500000\ncenter northing: 0.500000\n"
```

The new approach accesses the *text* attribute, which strips trailing whitespace:

```python
result = tools.g_region(flags="c").text
# Returns: "center easting:  0.500000\ncenter northing: 0.500000"
```

If you need the output exactly as it was written,
including newlines or original encoding, use *stdout* instead:

```python
result = tools.g_region(flags="c").stdout
# Returns: "center easting:  0.500000\ncenter northing: 0.500000\n"
```

This works identically with the string syntax:

```python
result = tools.run("g.region", flags="c").stdout
```

### From *parse_command* to smart result objects

The *parse_command* function parses key-value output from tools,
and newly also JSON, based on the function call parameters.
Shell format returns strings, while JSON format returns proper numbers.
With *grass.tools*, you get automatic type conversion
and can access parsed data directly.

The old approach with shell format:

```python
result = gs.parse_command("g.region", flags="c", format="shell")
# Returns: {"center_easting": "0.500000", "center_northing": "0.500000"}
```

The new *keyval* attribute automatically converts to numbers
if the string values are convertible,
bringing the output processing closer to JSON:

```python
result = tools.run("g.region", flags="c", format="shell").keyval
# Returns: {"center_easting": 0.5, "center_northing": 0.5}
```

For JSON format, the old approach was:

```python
result = gs.parse_command("g.region", flags="c", format="json")
# Returns: {"center_easting": 0.5, "center_northing": 0.5}
```

For JSON format, you can use the *json* attribute explicitly:

```python
result = tools.g_region(flags="c", format="json").json
```

However, for simpler syntax, result objects also support dictionary-like access,
avoiding a need for an additional variable or attribute access in the code:

```python
data = tools.g_region(flags="c", format="json")
center_e = data["center_easting"]
center_n = data["center_northing"]
```

### From *write_command* to passing objects

The *write_command* function pipes text to a tool's standard input
using a special *stdin* parameter along with *input="-"*
which is required by the tools (but standardized across tools).
This syntax requires two different parameters to set the input in addition
to the usage of dedicated function name.

The *run_command* family approach:

```python
gs.write_command(
    "v.in.ascii",
    input="-",
    output="point1",
    separator=",",
    stdin="13.45,29.96,200\n",
)
```

With *grass.tools*, you pass *io.StringIO* objects directly to the *input* parameter,
treating stdin like any other input source:

```python
import io

tools.v_in_ascii(
    input=io.StringIO("13.45,29.96,200\n"),
    output="point3",
    separator=",",
)
```

This also works with the string syntax:

```python
tools.run(
    "v.in.ascii",
    input=io.StringIO("13.45,29.96,200\n"),
    output="point2",
    separator=",",
)
```

## Replacing convenience wrapper functions

Beyond the core **_command* functions, *grass.script* provides various convenience
wrappers to mitigate different shortcomings of the *run_command* family approach.
These can be replaced with direct tool calls using JSON format.

Type conversion for values is done automatically
in the result object and/or with JSON,
and the basic tool call syntax is more lightweight, so the direct tool call
is usually not that different from a wrapper.
Direct tool calling also benefits from other features, not just the new Python API,
namely from better tool defaults (e.g., printing more in JSON)
and from more consistent tool behavior (e.g., tools accepting `format="json"`).

Not using the wrappers also makes it clear that any special parameters,
such as a session, need to be passes to each relevant call.
While this is obvious for the tools, it is less obvious when the wrappers are used.

### From *mapcalc* to *r.mapcalc*

The *mapcalc* function is a shortcut for *r.mapcalc*:

```python
gs.mapcalc("a = 1")
```

With *grass.tools*, call *r.mapcalc* directly with the *expression* parameter
for short expressions:

```python
tools.r_mapcalc(expression="a = 1")
```

For longer expressions (which would hit the operating system limit on the length
of subprocess parameter list), you can use a *StringIO* object:

```python
tools.r_mapcalc(file=io.StringIO("a = 1"))
```

While this requires you to make a decision about the parameter,
depending on the length of the expression as well as specifying the parameter explicitly,
the same approach works for any tools with a potentially long parameter value
without relying on an a dedicated wrapper (e.g., *r.series*).
In practice, the decision should be easy: all generated expressions should
use the *file* parameter,
while expressions written as Python string literal over couple lines
can use the simpler *expression* parameter.

### Replacing *mapcalc* usage of string templates

The *mapcalc* wrapper function internally uses Python string templates
which allows shell-like variable replacement where the values are passed as parameters:

```python
gs.mapcalc("$c = $a + $b", a=input_1, b=input_2, c=sum_raster)
```

This usage pattern can be replaced by direct use of Python formatting capabilities,
for example an f-strings:

```python
tools.r_mapcalc(f"{sum_raster} = {input_1} + {input_2}")
```

For simple expressions and limited number of input rasters,
the *r.mapcalc.simple* tool provides an alternative syntax:

```python
tools.r_mapcalc_simple("A + B", a=input_1, b=input_2, output=sum_raster)
```

For *r.mapcalc.simple*, the parametrized inputs need to be rasters and
they are always marked by letters in the expression.
However, *r.mapcalc.simple* provides access to convenience features
of the *grass.tools* API, such as the direct input of NumPy arrays.

### From *list_grouped*, *list_strings*, *list_pairs* to *g.list*

The old API provides several functions for listing maps,
each returning data in a different format.
These are replaced by calling *g.list* with JSON format
and using list comprehensions to extract the data you need.

Assume we have rasters named `["a", "b", "c", "surface", "surface2", "surface3"]`
in the PERMANENT mapset.

The *list_grouped* wrapper organizes maps by mapset:

```python
result = gs.list_grouped("raster")
names = result["PERMANENT"]
```

Use *g.list* with JSON format and filter by mapset:

```python
names = [
    item["name"]
    for item in tools.g_list(type="raster", format="json")
    if item["mapset"] == "PERMANENT"
]
```

The *list_strings* wrapper returns full names:

```python
result = gs.list_strings("raster")
```

Extract the *fullname* field from the JSON output:

```python
result = [
    item["fullname"] 
    for item in tools.g_list(type="raster", format="json")
]
```

The *list_pairs* returns tuples:

```python
result = gs.list_pairs("raster")
```

Extract name and mapset as tuples:

```python
result = [
    (item["name"], item["mapset"])
    for item in tools.g_list(type="raster", format="json")
]
```

While these require more code than the convenience functions,
they're more flexible and use the standard Python functionality to deal
with the data retrieved through the JSON output,
allowing for greater customization most fitting to the specific use case.

### From *region* to *g.region*

The *region* wrapper parses *g.region* output and returns a dictionary
with type-converted values:

```python
info = gs.region()
num_rows = info["rows"]
# Returns: 1 (as integer because of internal conversion)
```

Call *g.region* directly with JSON format:

```python
num_rows = tools.g_region(flags="p", format="json")["rows"]
# Returns: 1 (as an integer thanks to JSON)
```

## Handling special cases

The general tool behavior and interaction is configured
when creating a new *Tools* instance:

```python
tools = Tools(consistent_return_value=True, overwrite=True)
```

This configuration applies to all calls made with this *Tools* instance,
avoiding repetition of the parameters.
On the other hand, if you need to set the special parameter for only one tool call,
you will typically need to create a separate *Tools* object for that call.
However, the objects are cheap to create,
so the overhead is negligible in most cases.

### Checking return codes without exceptions

By default, *grass.tools* raises exceptions when tools fail,
which is appropriate for most workflows.
If you need to check return codes without raising exceptions,
configure the *Tools* object accordingly.

The *run_command* approach uses *errors="status"*:

```python
returncode = gs.run_command(
    "r.mask.status", 
    flags="t", 
    errors="status"
)
# Returns: 1 (if no mask exists)
```

Configure the *Tools* object to ignore errors and always return result objects:

```python
tools_with_returncode = Tools(
    errors="ignore", 
    consistent_return_value=True, 
)
result = tools_with_returncode.r_mask_status(flags="t")
returncode = result.returncode
# Returns: 1
```

While the new tool call above involves more objects,
namely a separate *Tools* object and also tool result object,
an actual tool call in the code may be quite straightforward:

```python
returncode = tools.r_mask_status(flags="t").returncode
# Returns: 1
```

### Using the overwrite flag

The overwrite flag can be set per tool call or globally for all calls.

The *run_command* approach requires it on every call:

```python
gs.run_command(
    "r.random.surface",
    output="surface",
    seed=42,
    overwrite=True,
)
```

Or you can set that for the whole process using an environment variable,
called `GRASS_OVERWRITE`.
You also can pass a dedicated environment with the *env* parameter
with the variable set there influencing only those tools for
which you pass the *env* variable.

The environment variable approach is still available with *grass.tools*,
but the *Tools* class allows a more clear and friendlier approach.

You can still pass it per call:

```python
tools = Tools()
tools.r_random_surface(output="surface", seed=42, overwrite=True)
```

However, you can set overwrite globally when creating the *Tools* object:

```python
tools = Tools(overwrite=True)
tools.r_random_surface(output="surface", seed=42)
```

This approach is particularly useful in scripts where you frequently recreate outputs
during development and testing,
but are not ready to turn your script into a GRASS tool in Python
which provides the overwrite settings automatically.
For many scripts, this means that overwrite setting is limited to one line
rather than being present in every tool call.

### Environment and session management

One of the biggest improvements in *grass.tools* is how it handles environments.
Instead of passing *env=session.env* to every single function call,
you configure it once.

The *run_command* approach repeats the environment parameter for every call:

```python
gs.run_command("r.import", input="elevation.tif", output="elevation", env=session.env)
gs.run_command("g.region", raster="elevation", env=session.env)
gs.run_command("r.slope.aspect", elevation="elevation", slope="slope", env=session.env)
```

With *grass.tools*, set the environment once:

```python
tools = Tools(env=session.env)
tools.r_import(input="elevation.tif", output="elevation")
tools.g_region(raster="elevation")
tools.r_slope_aspect(elevation="elevation", slope="slope")
```

You can pass either a session object or an environment dictionary:

```python
# Using a session object
tools = Tools(session=session)

# Using an environment dictionary
tools = Tools(env=session.env)
```

For rare cases where individual tool calls need different environments
(such as in certain parallelization scenarios), you can still override per call:

```python
tools.g_region(rows=100, cols=100, env=custom_env)
tools.r_random_surface(output="surface", seed=42, env=another_custom_env)
```

However, the typical pattern is to create separate *Tools* objects
for different environments rather than overriding per call.

## Return values

For *grass.tools*, the return value of a tool call
depends on the parameters and the tool's output,
unlike the *run_command* family of functions where the return value
depends on the function used in addition to parameters.
By default, a tool call returns a result object if the tool produces output (stdout)
or *None* when a tool produces no output.
The result object provides multiple ways to access the tool's output
and performs any further processing based on what attributes or functions are used.

For JSON results, you can also use dictionary-style indexing
directly on the result object:

```python
result = tools.g_region(flags="p", format="json")
rows = result["rows"]  # Equivalent to result.json["rows"]
```

While the *grass.tools* return value is supposed to be predictable and useful,
it is not as consistent as with the *run_command* family of functions.
If you need a consistent return value, e.g., for automation,
a consistent return value behavior can be requested:

```python
tools = Tools(consistent_return_value=True)
result = tools.r_slope_aspect(elevation="elevation", slope="slope")
# result is now a result object, even though stdout was empty
```

This configurable behavior avoids cluttering interactive sessions when a tool
creates output files, but prints nothing to stdout (like *r.slope.aspect*),
while at the same time it accommodates more advanced cases
when consistent, not just predictable, return value is needed.
This further provides better control when NumPy arrays
are returned by the tool call.

## Complete migration example

Here's a complete example showing the transformation
from *run_command* to *grass.tools* syntax for a script
which explicitly sets up and passes a session to tool calls.
(A code for an analytical GRASS tool will typically leave out the session
handling completely relying on the parent session.
Scripts may only setup the a global session without passing it explicitly.
Consequently, your code may be much simpler.)

### Old approach using run_command functions

```python
import grass.script as gs

with gs.setup.init("path/to/project") as session:
    # Import raster
    gs.run_command(
        "r.import", 
        input="/path/to/elevation.tif", 
        output="elevation",
        env=session.env
    )
    
    # Set region
    gs.run_command("g.region", raster="elevation", env=session.env)
    
    # Calculate slope
    gs.run_command(
        "r.slope.aspect",
        elevation="elevation",
        slope="slope",
        env=session.env
    )
    
    # Get region info
    region_info = gs.region(env=session.env)
    print(f"Rows: {region_info['rows']}")
    
    # List rasters
    rasters = gs.list_strings("raster", env=session.env)
    print(f"Rasters: {rasters}")
```

### New approach using grass.tools

```python
import grass.script as gs
from grass.tools import Tools

with gs.setup.init("path/to/project") as session:
    tools = Tools(session=session)
    
    # Import raster
    tools.r_import(input="/path/to/elevation.tif", output="elevation")
    
    # Set region
    tools.g_region(raster="elevation")
    
    # Calculate slope
    tools.r_slope_aspect(elevation="elevation", slope="slope")
    
    # Get region info
    region_info = tools.g_region(flags="p", format="json")
    print(f"Rows: {region_info['rows']}")
    
    # List rasters
    rasters = [
        item["fullname"] 
        for item in tools.g_list(type="raster", format="json")
    ]
    print(f"Rasters: {rasters}")
```

Most of the calls are shorter because *run_command*-like functions are avoided
and passing of the environment is done once.
The code focuses on what you're doing rather than how you're calling tools.

## Migration status and stability

Migration to *grass.tools* is recommended for new code but not required.
The API is included in GRASS 8.5 but not yet considered stable.
However, no major API changes are expected—only implementation refinements,
additions, and internal refactoring.

If your project requires absolute API stability,
continue using *grass.script.run_command* and related functions.
There are no current plans to deprecate the command functions
or the *grass.pygrass.modules* submodule,
so you can migrate at your own pace.

For projects that can accommodate minor API adjustments, adopting *grass.tools*
now will provide immediate benefits in code clarity and maintainability.
The core syntax and patterns are solid and unlikely to change.

## Summary

Migrating from *grass.script* command functions to *grass.tools* involves
these key changes:

* Create and use a *Tools* object once instead of calling different generic functions.
* Replace `run_command("tool.name", ...)` with `tools.tool_name(...)`.
* Replace *read_command* with result attributes like *stdout* or *text*.
* Replace *parse_command* with *json* or *keyval* attributes.
* Replace *write_command* with direct *io.StringIO* input.
* Replace convenience functions like *mapcalc*, *list_pairs*, and *region*
  with direct tool calls using JSON format.
* Set session (environment) once on the *Tools* object
  rather than passing *env* to every call.
* Use the function syntax *tools.r_slope_aspect()* as your primary approach,
  with *tools.run("r.slope.aspect")* available when needed.

The rest of *grass.script* (project management, session handling, etc.) remains
unchanged and should continue to be used alongside *grass.tools*.

For more information, see the [introduction to the Python interface](python_intro.md).
