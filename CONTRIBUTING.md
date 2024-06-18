# Contributing

There is more than one way of contributing to GRASS GIS.
Here we will focus on contributions centered
around the main GRASS GIS source code.
You can also report issues, plan new features,
or explore <https://grass.osgeo.org/get-involved/>.

## Changing code and documentation

This guide covers contributing to the main version of GRASS GIS source
code which is in the branch called _main_.
It assumes that you have some very basic knowledge of Git and GitHub,
but if you don't just go through some tutorial online or ask on the
GRASS GIS developer mailing list.

To contribute effectively, please familiarize yourself with our
[Programming Style Guide](./doc/development/style_guide.md).

### First time setup

* Create an account on GitHub.
* Install Git on your computer.
* Set up Git with your name and email.
* Fork the repository (by clicking the `Fork` button in the upper right corner
  of the GitHub interface).
* Clone your fork (use HTTPS or SSH URL, here we will use HTTPS):

```bash
git clone git@github.com:your_GH_account/grass.git
```

* Enter the directory

```bash
cd grass/
```

* Add main GRASS GIS repository as "upstream" (use HTTPS URL):

```bash
git remote add upstream https://github.com/OSGeo/grass
```

* Your remotes now should be "origin" which is your fork and "upstream" which
  is this main GRASS GIS repository. You can confirm that using:

```bash
git remote -v
```

* You should see something like:

```bash
origin  git@github.com:your_GH_account/grass.git (fetch)
origin  git@github.com:your_GH_account/grass.git (push)
```

For the following workflow, it is important that
"upstream" points to the OSGeo/grass repository
and "origin" to your fork
(although generally, the naming is up to you).

### Update before creating a feature branch

* Make sure your are using the _main_ branch to create the new branch:

```bash
git checkout main
```

* Download updates from all branches from the _upstream_ remote:

```bash
git fetch upstream
```

* Update your local _main_ branch to match the _main_ branch
  in the _upstream_ repository:

```bash
git rebase upstream/main
```

Notably, you should not make commits to your local main branch,
so the above is then just a simple update (and no actual
rebase or merge happens).

### Update if you have local changes

If `rebase` fails with "error: cannot rebase: You have unstaged changes...",
then move your uncommitted local changes to "stash" using:

```bash
git stash
```

* Now you can rebase:

```bash
git rebase upstream/main
```

* Get the changes back from stash:

```bash
git stash pop
```

### Creating a new feature branch

Now you have updated your local _main_ branch, you can create a feature branch
based on it.

* Create a new feature branch and switch to it:

```bash
git checkout -b new-feature
```

### Making changes

You can use your favorite tools to change source code or other files
in the local copy of the code. When making changes, please follow the
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
For Python, run `black .` to apply standardized formatting. You can
also run linter tools such as Pylint which will suggest different improvements
to your code.

### Committing

* Add files to the commit (changed ones or new ones):

```bash
git add file1
git add file2
```

* Commit the change (first word is the module name):

```bash
git commit -m "module: added a new feature"
```

### Pushing changes to GitHub

* Push your local feature branch to your fork:

```bash
git push origin new-feature
```

### Pull request

When you push, GitHub will respond back in the command line to tell
you what URL to use to create a pull request. You can follow that URL
or you can go any time later to your fork on GitHub, display the
branch `new-feature`, and GitHub will show you a button to create
a pull request.

Alternatively, you can explore GitHub CLI tool (_gh_) which allows you
to do `git push` and create a pull request in one step with `gh pr create -fw`.

### After creating a pull request

GRASS GIS maintainers will now review your pull request.
If needed, the maintainers will work with you to improve your changes.

Once the changes in the pull request are ready to be accepted,
the maintainers will usually squash all your commits into one commit and merge it
to the _main_ branch.

Once the pull request is merged, it is a good time to update your
local _main_ branch in order to get the change you just contributed.

### Further notes

GRASS GIS maintainers use additional workflows besides the one described
above. These are detailed at <https://trac.osgeo.org/grass/wiki/HowToGit>

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
