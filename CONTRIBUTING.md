# Contributing

There is more than one way of contributing to GRASS.
Here we will focus on contributions centered
around the main GRASS source code.
You can also report issues, plan new features,
or explore <https://grass.osgeo.org/get-involved/>.

## AI use policy

AI tools are part of modern development workflows and
contributors may use them. However, all contributions must meet
GRASS quality standards regardless of how they were created.

### Guidelines

AI-assisted development is acceptable when used responsibly.
Contributors must:

- **Test all code thoroughly.** Submit only code you have verified works correctly.
- **Understand your contributions.** You need to be able to explain the code changes
 you submit.
- **Write clear, concise PR descriptions** in your own words.
- **Use your own voice** in GitHub issues and PR discussions.
- **Take responsibility** for code quality, correctness, and maintainability.

### Disclosure

Disclose AI assistance when substantial algorithms or logic were AI-generated,
or when uncertain about licensing or copyright implications.
Be honest if a reviewer asks about code origins.

### Unacceptable submissions

Pull requests may be closed without review if they contain:

- Untested code
- Verbose AI-generated descriptions
- Evidence the contributor doesn't understand the submission

Using AI to assist learning and development is encouraged.
Using it to bypass understanding or submit work you cannot explain is not.

## Translations

Help make GRASS accessible in more languages!
No programming knowledge is needed — all translation work happens through
[Weblate](https://weblate.osgeo.org/projects/grass-gis/), a web-based platform.

### Getting started with translations

1. Create an [OSGeo UserID](https://www.osgeo.org/community/getting-started-osgeo/osgeo_userid/)
2. Sign in to [OSGeo Weblate](https://weblate.osgeo.org/) with your OSGeo UserID
3. Browse the [GRASS GIS translation project](https://weblate.osgeo.org/projects/grass-gis/)
4. Select your language (or request a new one if it's not listed)
5. Start translating! The web interface guides you through each message

Translations are automatically submitted to the GRASS repository as pull requests.
For help or questions, join the
[GRASS user community on Discourse](https://discourse.osgeo.org/c/grass/grass-user/70).

See [locale/README.md](locale/README.md) for more technical details about the
translation process.

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
checks locally and, indeed, some of them may fail for your code. This is a part
of the standard iterative process of integrating changes into the main code,
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

GRASS is written in more than one programming language, but you need
to know only the language relevant to your contribution. While much
of the source code is written in C, a significant portion is written in Python.
A compiler is needed to convert the C and C++ source code into executable
files ("binaries"). In contrast, Python is an interpreted language that
can only be executed with Python software. There is also documentation
in HTML files and other files in the GRASS source code.
