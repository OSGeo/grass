# Contributing

## Introduction

GRASS GIS is written in more than one programming language, but you need
to know only the language relevant to your contribution. While much
of the source code is written in C, a significat portion is written in Python.
A compiler is needed to convert the C and C++ source code into executable
files ("binaries"). In contrast, Python is an interpreted language that
can only be executed with Python software. There is also documentation
in HTML files and other files in the GRASS GIS source code.

## Contributions other than code

There is more than one way of contributing, see full list at
<https://grass.osgeo.org/get-involved/>.
In the rest of this document, we will focus on contributions centered
around the GRASS GIS source code.

## Reporting issues and suggesting features

To report an issue or to suggest features or a change,
[open an issue](https://github.com/OSGeo/grass/issues/new/choose)
on GitHub.

## Changing code and documentation

This guide covers contributing to the main version of GRASS GIS source
code which is in the branch called _main_.
It assumes that you have some very basic knowledge of Git and GitHub,
but if you don't just go through some tutorial online or ask on the
GRASS GIS developer mailing list.

### First time setup

* Create an account on GitHub.
* Install Git on your computer.
* Set up Git with your name and email.
* Fork the repository (by clicking the `Fork` button in the upper right corner
  of the GitHub interface).
* Clone your fork (use SSH or HTTPS URL):

```
git clone git@github.com:your_GH_account/grass.git
```

* Enter the directory

```
cd grass/
```

* Add main GRASS GIS repository as "upstream" (use HTTPS URL):

```
git remote add upstream https://github.com/OSGeo/grass
```

* Your remotes now should be "origin" which is your fork and "upstream" which
  is this main GRASS GIS repository. You can confirm that using:

```
git remote -v
```

* You should see something like:

```
origin	git@github.com:your_GH_account/grass.git (fetch)
origin	git@github.com:your_GH_account/grass.git (push)
upstream	https://github.com/OSGeo/grass.git (fetch)
upstream	https://github.com/OSGeo/grass.git (push)
```

For the following workflow, it is important that "origin" points to your fork
(although generally, the naming is up to you).

### Update before creating a feature branch

* Make sure your are using the _main_ branch to create the new branch:

```
git switch main
```

* Download updates from all branches from the _upstream_ remote:

```
git fetch upstream
```

* Update your local _main_ branch to match the _main_ branch
  in the _upstream_ repository:

```
git rebase upstream/main
```

Notably, you should not make commits to your main branch,
so the above is than just a simple update (and no actual
rebase or merge happens).

### Update if you have local branches

If `rebase` fails with "error: cannot rebase: You have unstaged changes...",
then move your uncommitted local changes to "stash" using:

```
git stash
```

* Now you can rebase:

```
git rebase upstream/main
```

* Get the changes back from stash:

```
git stash pop
```

### Creating a new feature branch

Now you have updated your local _main_ branch, you can create a feature branch
based on it.

* Create a new feature branch and switch to it:

```
git checkout -b new-feature
```

### Making changes

You can use your favorite tools to change source code or other files
in the local copy of the code. When make changes, please follow
Submitting Guidelines at
<http://trac.osgeo.org/grass/wiki/Submitting>.

### Testing changes

Testing helps to ensure that the changes work well with the rest
of the project. While there is many different ways to test,
usually you will want to compile the source code (see below),
add test code (using _grass.gunittest_ or pytest), and run code
linters (automated code quality checks).

There is a series of automated checks which will run you pull request
after you create that later on. You don't need to run every one of these
checks locally and some of these may fail for your code. This is a part of
the standard iterative process of interating changes into the main code,
so if that happens, just see the error messages and go back to your code
and try again. If you are not sure what to do, let other know in a pull
request comment.

There are some steps you can do locally, to improve your code.
For Python, run `black .` to apply standardized formatting. You can
also run linter tools such as Pylint which will suggest you how
you may improve your code.

### Committing

* Add files to the commit (changed ones or new ones):

```
git add file1
git add file2
```

* Commit the change (first word is the module name):

```
git commit -m "module: added a new feature"
```

### Pushing changes to GitHub

* Push your local feature branch to your fork:

```
git push origin new-feature
```

### Pull request

When you push, GitHub will respond back in the command line to tell
you what URL to use to create a pull request. You can follow that URL
or you can go any time later to your fork on GitHub, display the
branch `new-feature`, and GitHub will show you button to create
a pull request.

Alternatively, you can explore GitHub CLI tool (_gh_) which allows you
to do `git push` and creating a pull request in one step with `gh pr create -fw`.

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
for most operating systems in our wiki:

[Compile and install instructions](https://grasswiki.osgeo.org/wiki/Compile_and_Install)
