# Submitting C code

When submitting C code to GRASS GIS GitHub repository, please take care of
following rules (see also
[RFC/7_LanguageStandardsSupport](https://trac.osgeo.org/grass/wiki/RFC/7_LanguageStandardsSupport#CLanguage)):

## API Manual

Get and read the GRASS Programmer's Manual here:

<https://grass.osgeo.org/programming8/>

Or generate it from this source code (the programmer's manual is integrated in
the source code in doxygen style):

```bash
make htmldocs
make pdfdocs
```

## Directory Conventions

Use the directory structure to place your module appropriately into the source tree

- libraries go into `lib/`
- raster modules go into `raster/`
- vector modules go into `vector/`
- ...

Consider to take a look at "GNU Coding Standards":
<https://www.gnu.org/prep/standards/>

## Headers

Add a header section to file main.c of your module and make sure you include the
copyright. The purpose section is meant to contain a general overview of the code
in the file to assist other programmers that will need to make changes to your
code. If you are modifying an existing file you may under no circumstances remove
prior copyright or licensing text that is not your own, even for a major rewrite.
If any original code or code that is in part derived from another's original work
remains, it must be properly cited.

Example:

```c
/****************************************************************************
 *
 * MODULE:       g.foo
 * AUTHOR(S):    John Doe <jdoe at somewhere org>
 * PURPOSE:      Provide short description of module here...
 * COPYRIGHT:    (C) 2010 by John Doe, and the GRASS Development Team
 *
 *               This program is free software under the GNU General Public
 *               License (>=v2). Read the COPYING file that comes with GRASS
 *               for details.
 *
 *****************************************************************************/
```

The copyright protects your rights according to GNU General Public License
<https://www.gnu.org>.

### GRASS Config Header

To ensure that the software system continues to work, please include

`#include <grass/config.h>`

in your files and make use of the various system dependencies contained therein.
As one example of this, see [lib/gmath/fft.c​](../../../lib/gmath/fft.c). Please
refrain from declaring system functions within the software; include the proper
header files (conditionally dependent on config.h macros if necessary) instead.

### Other Headers

Order of include headers

In general, headers should be included in the order:

1. Core system headers (stdio.h, ctype.h, ...)
2. Headers for non-core system components (X11, libraries).
3. Headers for core systems of the package being compiled
   (grass/gis.h, grass/glocale.h, ...)
4. Headers for the specific library/program being compiled (geodesic.h, ...)

Each class of header has an obligation to be compatible with those above it in
the list, but not those below it.

## Functions

### Void

Always specify the return type for ALL functions including those that return type
"void", and insert return statements for any function which returns a value.

Also, use ANSI C prototypes to declare your functions. For module return values,
see "Exit status" below.

Examples:

```c
void G_something(void);
int G_something_else(int, int);

void G_something(void)
{
    /* Snipped out code */

    return;
}

int G_something_else(int x, int y)
{
    /* Snipped out code */

    return 0;
}
```

### G_asprintf()

Use the GRASS library function G_asprintf() instead of the standard C functions
asprintf(), vsnprintf() and snprintf(). These functions are not portable or have
other issues. Example:

```c
char *msg;

G_asprintf(&msg, "%s", parameters);
do_something_with_msg();
G_free(msg);
```

Note that you should free memory when G_asprintf() is used.

### Memory Allocations

Use the following GRASS library functions instead of the standard C functions.
The reason for this is that the following functions ensure good programming
practice (e.g. always checking if memory was allocated) and/or improves
portability. PLEASE refer to the programmers manual for the proper use (e.g.
determining if any casts are needed for arguments or return values) of these
library functions. They may perform a task slightly different from their
corresponding C library function, and thus, their use may not be the same.

```c
G_malloc();   // instead of malloc()
G_calloc();   // instead of calloc()
G_realloc();  // instead of realloc()
G_free();     // instead of free()
G_getenv();   // instead of getenv()
G_setenv();   // instead of setenv()
G_unsetenv(); // instead of unsetenv()
G_sleep();    // instead of sleep()
```

Could somebody please add others (please verify that they are useful and safe
first)

### Naming Conventions

Use function names which fulfill the official GNU naming convention:
<https://www.gnu.org/prep/standards/html_node/Names.html#Names>

Instead of naming a function like: MyNewFunction() use underscores for separation
and lower case letters: my_new_function()`.

### Comments

If you want to comment code portions, use

```c
#ifdef notdef
     portion_to_be_commented;
#endif
```

This is safe comparing to nested `/* comments */`

Functions in the library must be documented in doxygen style to get them into the
programmer's manual (generate with make pdfdocs or make htmldocs). See
[lib/gis/](../../../lib/gis/) for examples.

### Documentation in Doxygen

Use doxygen style for source code documentation. It is required for GRASS libraries,
but also recommended for GRASS modules.

Do not use structural command inside documentation block since it leads to some
duplication of information (e.g. do not use \fn command in comment blocks). The
exception is \file command for documenting a file, in this case structural command
is required.

For files

```c
/*!
   \file snap.c

   \brief Vector library - Clean vector map (snap lines)

   (C) 2001-2008 by the GRASS Development Team

   This program is free software under the GNU General Public
   License (>=v2).  Read the file COPYING that comes with GRASS
   for details.

   \author Radim Blazek
*/
```

For functions

<!-- markdownlint-disable line-length -->
```c
/*!
   \brief Snap lines in vector map to existing vertex in threshold

   For details see Vect_snap_lines_list()

   \param Map pointer to input vector map
   \param type filter features of given type to be snap
   \param thresh threshold value for snapping
   \param[out] Err pointer to vector map where lines representing snap are written or NULL
   \param[out] msgout file pointer where messages will be written or NULL

   \return 1
*/
```
<!-- markdownlint-enable line-length -->

## Modules

### Returning Value Of Main function

Module exit status is defined as EXIT_SUCCESS or EXIT_FAILURE (declared in
stdlib.h), e.g.

```c
{
  ...
  if (G_parser (argc, argv))
      exit (EXIT_FAILURE);

  ...
  exit (EXIT_SUCCESS);
}
```

### Messages

Use fprintf() instead of printf(). For errors and warnings please use the
G_fatal_error() and G_warning() functions. General messages for the user should
use G_message() while debug messages should use G_debug() whenever possible.

There are two variants to G_message(): G_verbose_message() which will only display
the message if in --verbose mode, and G_important_message() which will always
show the message unless the module is running in --quiet mode. G_fatal_error() and
G_warning() will always be displayed regardless of verbosity setting. Messages
sent to any of these functions will be printed to stderr.

G_message() output is not expected to be sent to pipe or file.

Messages aiming at the user should be marked for translation. Output meant for
automatic parsing by other software should not be marked for translation.
Generally all modules producing output should include localisation header:

```c
#include "glocale.h"
```

Afterwards mark all user visible strings with the gettext macro \_("message"):

```c
G_fatal_error(_("Vector map <%s> not found"), name);
```

It is suggested to add a comment line before translatable user message to give a
hint to translators about meaning or use of cumbersome or obscure message. First
word in the comment must be GTC: GRASS translation comment,

Example:

```c
/* GTC: Name of a projection */
G_message(_("State Plane"));
```

Any message with a noun in plural form has to pass `n_()` macro, even if for the
English language is not required! The syntax is
`n_("English singular", "English plural", count)`

```c
G_message( n_("%d map from mapset <%s> removed",
              "%d maps from mapset <%s> removed", count), count, mapset);
/* Notice double use of "count" - as an argument for both functions
   - n_() and G_message() */

G_message( n_("%d map selected", "%d maps selected", count), count);
G_message( n_("One file removed", "%d files removed", count) count);
/* Both of forms of singular case "%d file" or "One file" are correct.
   The choice between them is purely stylistic one. */

/* Although in English it is not necessary to provide a separate
   text if "n" always is >1, other languages do have a difference if "n"
   is i.e. 2-4, or n==10 etc. */
G_message( n_("Remove map", "Remove maps", count));
/* Number it self doesn't have to be used in the output text */
```

Pipe/file data output: For data output redirected to pipe or file, please use
fprintf() and specify the stdout stream as follows:

```c
fprintf(stdout, ...);
fflush(stdout);

fflush(stdout); /* always required when using fprintf(stdout, ...). */
```

### Map History

Have a function included in your module which writes to the history file of the
map (e.g. command line, parameters etc.). See e.g.
[raster/r.patch/main.c​](../../../raster/r.patch/main.c) (the same applies to
vector and raster3d modules!)

### Standardized Options and Flags

Standard parser options: use G_define_standard_option() whenever possible to
define standard module command line options. This will save you time, create fewer
bugs, and make things easier on the translators. See
[lib/gis/parser_standard_options.c]​(../../../lib/gis/parser_standard_options.c)
for details of the function definition.

### Adding description and keywords

Each module needs to have a description and at least 3 keywords. Here an example
from general/g.list/main.c:

```c
G_gisinit(argv[0]);

module = G_define_module();
G_add_keyword(_("general"));
G_add_keyword(_("map management"));
G_add_keyword(_("list"));
G_add_keyword(_("search"));
module->description =
    _("Lists available GRASS data base files of "
      "the user-specified data type optionally using the search pattern.");
```

Notes:

- the **first** keyword is the module family (**g**.list belongs to "general")
  which go to the
  [module family index](https://grass.osgeo.org/grass74/manuals/general.html)
  in the manual
- the **second** keyword is the overall topic which go to the
  [topic index](https://grass.osgeo.org/grass74/manuals/topics.html) in the manual
- the **third\* (and more) keyword is describing further keywords which go to the
  [keyword index](https://grass.osgeo.org/grass74/manuals/keywords.html)
  in the manual**

These index manual pages are autogenerated during the build process of GRASS GIS.

## Source Code Formatting

C and C++ code is formatted with [ClangFormat](https://clang.llvm.org/docs/ClangFormat.html).
Contributions to main branch (and grass8 branch for grass-addons) are expected
to be formatted with `clang-format` (currently with version 15+). The most
convenient method to install clang-format and format files is
[using pre-commit](./submitting.md#use-pre-commit).

Alternatively, using separately installed clang-format on modified files:

```bash
clang-format -i <new_or_modified_file.c>
```

The ClangFormat settings for the repo are defined in
[.clang-format](../../../.clang-format).

If using pre-commit is not an option, for whatever reason, there is a helper
script [grass_clang_format.sh](./utils/grass_clang_format.sh), which simplifies
bulk reformatting. Before using this script you need to install `clang-format`
and make sure it is available on PATH.

```bash
# Simple way to install clang-format (optional)
python -m pip install 'clang-format==15.0.6'

# Run script to format all files in source repo
./utils/grass_clang_format.sh

# It is also possible to format the content in a (one) given directory (faster)
./utils/grass_clang_format.sh ./lib/raster

# Setting GRASS_CLANG_FORMAT enables use of clang-format by other name/path
GRASS_CLANG_FORMAT="clang-format-15" ./utils/grass_clang_format.sh
```

## Compilation

Platform dependent code:

Do not remove `#ifdef __CYGWIN__` and/or `#ifndef __CYGWIN__` lines and their
encapsulated lines from source code (one example was that someone removed
drand48 definition.)

### Compiler Flags

Suggested compiler flags: We suggest to use very strict compiler flags to capture
errors at the very beginning. Here our list of flags, please use them to configure
you development version of GRASS GIS.

See also <https://grasswiki.osgeo.org/wiki/Compile_and_Install>

#### GNU/Linux

```bash
MYCFLAGS="-g -Wall -Werror-implicit-function-declaration -Wreturn-type \
          -fno-common -fexceptions"
MYCXXFLAGS="-g -Wall"
MYLDFLAGS="-Wl,--no-undefined -Wl,-z,relro"

CFLAGS="$MYCFLAGS" CXXFLAGS="$MYCXXFLAGS" LDFLAGS="$MYLDFLAGS" ./configure ...
```

#### MacOSX

(to be suggested, see ​macosx/ReadMe.rtf)

#### MS-Windows

(to be suggested, see CompileOnWindows)

#### FreeBSD / NetBSD

(to be suggested)

See also <https://grasswiki.osgeo.org/wiki/Compile_and_Install#FreeBSD_.2F_NetBSD>

#### IBM/AIX

(to be suggested)

See also <https://grasswiki.osgeo.org/wiki/Compile_and_Install#AIX>

#### Solaris

(to be suggested)

See also <https://grasswiki.osgeo.org/wiki/Compile_and_Install#AIX>

...
