# Contributing

There is more than one way of contributing to GRASS GIS.
Here we will focus on contributions centered
around the main GRASS GIS source code.
You can also report issues, plan new features,
or explore <https://grass.osgeo.org/get-involved/>.

## Changing code and documentation

To contribute changes to GRASS GitHub repository, use a
"fork and pull request" workflow. This [guide](./doc/development/github_guide.md)
leads you through a first time setup and shows how to create a pull request.

To contribute effectively, please familiarize yourself with our
[Programming Style Guide](./doc/development/style_guide.md).

### Testing changes

Testing helps to ensure that the changes work well with the rest
of the project. While there are many different ways to test,
usually you will want to compile the source code (see below),
add test code (using _grass.gunittest_ or pytest), and run code
linters (automated code quality checks).

There is a series of automated checks which will run on your pull request
after you create one. You don't need to run all these
checks locally and, indeed, some of them may fail for your code. This is a part of
the standard iterative process of integrating changes into the main code,
so if that happens, just see the error messages, go back to your code
and try again. If you are not sure what to do, let others know in a pull
request comment.

Note that there are some steps you can do locally to improve your code.
For Python, run `ruff format` to apply standardized formatting. You can
also run linter tools such as `ruff check` or Pylint which will suggest
different improvements to your code.

## Compilation

More often than not, in order to test the changes, you need to create
a runnable binary program from the source code,
using the so-called "compilation step". While the
source code consists of thousands of C and Python files (plus HTML
documentation and other files), the included "makefiles" tell the build system to
generate binaries from the source code in the correct order, render the
manual pages, etc.

The way to install the compiler tools and Python depends on the operating
system. To make this easier, we have collected copy-paste instructions
to install dependencies and compile GRASS source code for most operating systems.
Please see our dedicated wiki:

[Compile and install instructions](https://grasswiki.osgeo.org/wiki/Compile_and_Install)

## About source code

GRASS GIS is written in more than one programming language, but you need
to know only the language relevant to your contribution. While much
of the source code is written in C, a significant portion is written in Python.
A compiler is needed to convert the C and C++ source code into executable
files ("binaries"). In contrast, Python is an interpreted language that
can only be executed with Python software. There is also documentation
in HTML files and other files in the GRASS GIS source code.
