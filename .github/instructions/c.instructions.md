---
description: "GRASS Programming Style Guide for C and C++ Instructions"
applyTo: "**/*.c, **/*.h"
---
# GRASS Programming Style Guide for C and C++

- C and C++ code is formatted with ClangFormat
- Contributions are expected to be formatted
with `clang-format` (currently with version 18+).
- The most convenient method to install clang-format and format files is using pre-commit.

Alternatively, using separately installed clang-format on modified files:

```bash
clang-format -i <new_or_modified_file.c>
```

The ClangFormat settings for the repo are defined in
[.clang-format](../../.clang-format).

If using pre-commit is not an option, for whatever reason, there is a helper
script [grass_clang_format.sh](./utils/grass_clang_format.sh), which simplifies
bulk reformatting.

## Order of include headers

In general, headers should be included in the order:

1. Core system headers (stdio.h, ctype.h, ...)
2. Headers for non-core system components (X11, libraries).
3. GRASS headers (grass/gis.h, grass/glocale.h, ...)
4. Headers for the specific library/program (geodesic.h, ...)

Each class of headers has an obligation to be compatible with those above it in
the list, but not those below it. The header groups should be alphabetically
sorted and separated by a newline.

```c
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/raster.h>

#include "local_proto.h"
#include "mask.h"
```

## Naming conventions

Use function names which fulfill the official [GNU naming
convention](https://www.gnu.org/prep/standards/html_node/Names.html). Instead of
naming a function like: `MyNewFunction()` use snake case: `my_new_function()`.

## C API documentation

We
[use doxygen and document the functions](https://grass.osgeo.org/programming8/)
directly in the source code. See `lib/gis/open.c` and `lib/gis/gislib.dox` for
examples.

### Developing C tools

Refer to the [online GRASS Programmer's
Manual](https://grass.osgeo.org/programming8/) or generate it with `make
htmldocs` or `make pdfdocs`.

#### Use GRASS library functions

Use the GRASS library functions, when available, instead of the standard C
functions. The reason for this is that the following functions ensure good
programming practice (e.g. always checking if memory was allocated) and/or
improves portability.

- Memory management: `G_malloc()`, `G_calloc()`, `G_realloc()`, `G_free()`
- Environmental variables: `G_getenv()`, `G_setenv()`, `G_unsetenv()`
- File seek: `G_fseek()`, `G_ftell()`
- Printing: `G_asprintf()`, `G_vsaprintf()`, `G_vfaprintf()`, ...

Please refer to the [programmers manual](https://grass.osgeo.org/programming8/)
for the proper use (e.g., determining if any casts are needed for arguments or
return values) of these library functions. They may perform a task slightly
different from their corresponding C library function, and thus, their use may
not be the same.

#### Returning value of main function

Tool exit status is defined as `EXIT_SUCCESS` or `EXIT_FAILURE` (declared in
`stdlib.h`), e.g.

```c
    {
      ...
      if (G_parser (argc, argv))
          exit (EXIT_FAILURE);

      ...
      exit (EXIT_SUCCESS);
    }
```

#### Messages and data output

See rules for [messages in Python scripts](python.instructions.md#messages)
for proper usage of `G_fatal_error()`, `G_warning()`, etc. Message
output is not expected to be sent to pipe or file.

For data output redirected to pipe or file, please use `fprintf()` and specify
the stdout stream as follows:

```c
      fprintf(stdout, ...);
      fflush(stdout);

      fflush(stdout) /* always required when using fprintf(stdout, ...). */
```

#### Header section

- Add a header section to file main.c of your tool and make sure you include the
copyright.
- If you are modifying an existing file you may under no circumstances
remove prior copyright or licensing text that is not your own, even for a major
rewrite.
- If any original code or code that is in part derived from another's
original work remains, it must be properly cited.

```c
/****************************************************************************
 *
 * MODULE:       g.foo
 * AUTHOR(S):    John Doe <jdoe at somewhere org>
 * PURPOSE:      Provide short description of module here...
 * COPYRIGHT:    (C) 2010 by John Doe, and the GRASS Development Team
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 *****************************************************************************/
```
