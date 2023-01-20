# Submitting General Notes

## Module behaviour: computational region settings

The current region or [​computational region](https://grasswiki.osgeo.org/wiki/Computational_region)
is the actual setting of the region boundaries and the actual raster resolution.
It can be considered as region of interest (ROI).

As a general rule in GRASS GIS (new module implementations should follow this!):

- **Raster maps** are always imported completely at their own resolution
  (exception: WMS imported layers).
- **Vector maps** are always imported completely.
- In computations,
  - raster input maps are automatically cropped/padded and rescaled (using
    nearest neighbour resampling) to match the current region in order to
    produce the output raster map or to query values.
  - Raster output maps have their bounds and resolution equal to those of the
    current computational region.
  - Vector maps are always considered completely.

## Submitting code

Be sure to develop on top of the _latest_ GRASS GIS code which is in a Git
repository on GitHub. See [​CONTRIBUTING](https://github.com/OSGeo/grass/blob/master/CONTRIBUTING.md)
file there.

### Commit messages

Generally, the commit message (log message) should give an information about
_what changed in the code_ and _how the change affects the functionality_.
Additionally, the change of dependencies and changes of functionality of depended
code should be discussed if applicable.

The general format of a message is:

```txt
module or library: description (possible Trac ticket, merge commit or related commit)
```

**Good examples** (of single line commit messages):

```txt
r.to.vect: corrected x in the crowded message
g.mremove: Changed the interface to that of g.mlist and added exclude= (ticket #2228)
libraster: Added raster name and row info to get/put\_row error messages
vlib/pg: column check must be case-insensitive
wxGUI/lmgr: add measuring of distances also to Layer Manager
wxGUI: workaround for not visible toolbars on Mac with wxPython 3
```

Include ticket using hash mark and ticket number, e.g. `#2228`, and another commit
(revision) using letter r and revision number, e.g. `r60975`. This will allow Trac
(and perhaps other systems) to create a link to the ticket or commit (or revision).
However, do not rely on this and always include the information about what and why
the commit is changing and how (consider browsing commit messages in command line).

It is possible and allowed to do multiline commit messages but you should consider
the following. First, multiline commit messages should be used to provide further
details about the commit. They should not be used to describe large changes of
code. Instead, these large changes should be split into the separate commits with
shorted commit messages. Note that this not only simplifies writing of good,
simple and readable commit messages but it also makes code review and regression
testing easier. Second, if you have a lot to say about the commit you should
perhaps include this in the comments or documentation (you can refer there to
tickets or other commits too in the same way as in commit messages, although they
will not be automatically linked).

Write the commit messages in the way that they can be used to create/update
change logs, [wiki:Release](https://trac.osgeo.org/grass/wiki/Release) pages and
news in general.

Don't include your name (or id) to commit message, this is stored separately and
automatically. However, if you are committing someone's code (e.g. path) or you
are writing the code together with someone, include his or her name.

Include the module, library or component name as a prefix separated by colon,
e.g. `libraster:`. You don't have to include file names in the commit message,
they are managed by SVN itself.

If you are not sure if your style is correct, ask on mailing list.

Some **bad examples** (of single line commit messages):

```txt
r.slope.aspect: fix compilation
(missing information what exactly was broken and reasoning behind the fix; it's
clear that we are not trying to break something by the commit)

wxGUI/render: attempt to fix #560
(missing information what is #560 and how we are trying to fix it)

Add tests for Table and Columns classes
(we don't know where the classes belongs to, prefix pygrass: or
pythonlib/pygrass would tell us in this case)

fix bug introduced in r60216
(missing information how the bug was fixed and which bug it was)

fix r60216 (i18n)
(it should probably say something like: wxGUI: fix insufficient handling of i18n
(introduced in r60216))

libraster:Added raster name and row info to get/put\_row error messages
(missing space after colon)

d.histogram launched from map display toolbar doesn't work
(this is description of what is wrong, not how it is fixed)

fixed loading from a file, should I backport it?
(commit messages are not for opening discussions or general communication)
```

### Creating (legacy) diffs

Be sure to create unified (`diff -u`) format. "Plain" diffs (the default format)
are risky, because they will apply without warning to code which has been
substantially changed; they are also harder to read than unified.

Such diffs should be made from the top-level directory, e.g.
`git diff display/d.vect/main.c`; that way, the diff will include the pathname
rather than just an ambiguous `main.c`.

### SVN Properties

When submitting new files to the repository set SVN properties,

usually for directory

```text
svn:ignore : *.tmp.html
             *OBJ*
```

or e.g. for C-file

```text
svn:mime-type : text/x-csrc
```

```text
svn:keywords : Author Date Id
svn:eol-style : native
```

See [​http://svnbook.red-bean.com/en/1.4/svn.advanced.props.html](http://svnbook.red-bean.com/en/1.4/svn.advanced.props.html)

To set a property:

```bash
svn propset svn:keywords 'Author Date Id' <file>
svn propset svn:mime-type text/x-sh grass\_shellscript.sh
```

To edit the `svn:ignore` property using your default text editor:

```bash
svn propedit svn:ignore <directory>
```

To set the `svn:ignore` property non-interactively, first create a file
containing the value:

```bash
echo "\*.tmp.html" > ignore.txt
echo "\*OBJ\*" >> ignore.txt
```

then use:

```bash
svn propset -F ignore.txt svn:ignore <directory>
```

List of mime-type:

```text
C++ files (.cpp): text/x-c++src
C files (.c): text/x-csrc
DTD files (.dtd): text/xml-dtd
GIF files (.gif): image/gif
Header files (.h): text/x-chdr
HTML files (.html): text/html
JPEG files (.jpg): image/jpeg
Makefiles: text/x-makefile
PNG files (.png): image/png
Python files (.py): text/x-python
Shell scripts (.sh): text/x-sh
Text files (.txt): text/plain
XML files (.xml): text/xml
```

(please update the list...)

For your convenience use the
[source:grass-addons/tools/module\_svn\_propset.sh](https://trac.osgeo.org/grass/browser/grass-addons/tools/module_svn_propset.sh)
script.

### SVN Property Id

We don't want the `$ID$` in source code any more as it causes problems for the
SVN branches.

### Comments

PLEASE take the time to add comments throughout your code explaining what the
code is doing. It will save a HUGE amount of time and frustration for other
programmers that may have to change your code in the future.

### End Of Line

Make sure a new line is at the end of each file and UNIX style newlines are
used (`\n`).

### Branches and backports

_This section applies to the core developers with access to the main repository
(other contributors can safely ignore it)._

GRASS GIS Subversion repository has trunk and several branches. All the development
should happen in trunk (trunk is a "development branch"). All the other branches
are usually associated with releases and should contain stable code which is being
prepared towards the given release.

When a bug is fixed the fix should be committed to trunk, tested there, and than
backported to the release branch or branches. The testing before backport should
include user testing, running automated test (if available), and compilation of
the whole source tree (ideally after `make distclean`). Note that thinks like
testing should be done also before the original commit to trunk. Also note that
not all these steps has to be done manually, you can take an advantage of
[​Travis CI](https://travis-ci.org/GRASS-GIS/) or
[​automated runs of the GRASS GIS testing suite](http://fatra.cnr.ncsu.edu/grassgistests/).

Often there is more than one active stable branch, if you are backporting, make
sure you always backport to all the branches between trunk and the furthest
branch you are backporting to. For example, let's say we have trunk and branches
7.2 and 7.0, if you backport to 7.0, you have to also backport to 7.2. You can
also choose to backport only to the closest branch, in our example 7.2.

Backport only complete fixes. When you are not sure if the fix is complete or if
there is an possibility that some details such as wording will change, wait with
the backport for a little while and backport all the changes together to reduce
number of commits which needs to be reviewed (right now or in the future). You
can also backport multiple commits from trunk as one commit if you think it is
appropriate.

Include the number of the commit (or commits) you are backporting, into the commit
message, for example: `less verbose messages (backport r89436)`. This will help
matching the file content in between branches and tracking if the commits were
backported.

As a developer you should maintain a list of commits which you plan to backport.
One way how to do it is to subscribe to the
[​grass-commit mailing list](https://lists.osgeo.org/listinfo/grass-commit) and
filter your commits in your email.

Do not add svn merge property to the commit. Review
[tools directory in Addons repository](/grass/browser/grass-addons/tools) for
scripts which will help you with backporting.

## Makefiles

When writing Makefiles, use the current standard.

If you have to use commands, please check for:

| avoid             | use instead                   |
|-------------------|-------------------------------|
| make target       | $(MAKE) target                |
| mkdir target      | $(MKDIR) target               |
| cp  (executable)  | $(INSTALL) -m 755 file target |
| cp  (normal file) | $(INSTALL) -m 644 file target |
| ar                | $(AR)                         |

 `rm`: be VERY careful with recursive remove. Also beware of removing
 `$(FOO)*` if `$(FOO)` has any chance of being empty.

Examples: see below examples or others

[source:grass/trunk/raster/r.info/Makefile](https://trac.osgeo.org/grass/browser/grass/trunk/raster/r.info/Makefile)
[source:grass/trunk/vector/v.edit/Makefile](https://trac.osgeo.org/grass/browser/grass/trunk/vector/v.edit/Makefile)

If you are unsure, please ask on the GRASS Developers list.

### AutoConf

If you need to add support for a different library in the 'configure' script, you
should first seek consent in the grass-dev mailing list (see below), then you need
to expand 'configure.in' and run subsequently `autoconf-2.69` (later versions
will not work) to re-generate 'configure'.

## Naming Conventions

Have a look at [source:grass/trunk/INSTALL](https://trac.osgeo.org/grass/browser/grass/trunk/INSTALL)

For consistency, use `README` rather than `README.txt` for any `README` files.

### Variables

GRASS/Environment variables:

If you add a new variable, please follow the naming convention. All variables
are described in
[source:grass/trunk/lib/init/variables.html](https://trac.osgeo.org/grass/browser/grass/trunk/lib/init/variables.html)

### Modules

Try to use module names which describe shortly the intended purpose of the module.

The first letters for module name should be:

```text
d.    - display commands
db.   - database commands
g.    - general GIS management commands
i.    - imagery commands
m.    - miscellaneous tool commands
ps.   - postscript commands
r.    - raster commands
r3.   - raster3D commands
v.    - vector commands
```

Some additional naming conventions

- export modules: (type).out.(format) eg: `r.out.arc`, `v.out.ascii`
- import module: (type).in.(format) eg: `r.in.arc`, `v.in.ascii`
- conversion modules: (type).to.(type) eg: `r.to.vect`, `v.to.rast`, `r3.to.rast`

Avoid module names with more than two dots in the name. Example: instead of
`r.to.rast3.elev` use `r.to.rast3elev`

## Code Quality

Follow the best writing practices specified by GRASS
[submitting rules](https://trac.osgeo.org/grass/wiki/Submitting) for a given
language.

Write tests for your code. See the [testing guide](https://grass.osgeo.org/grass73/manuals/libpython/gunittest_testing.html)
or existing examples ([g.list](https://trac.osgeo.org/grass/browser/grass/trunk/general/g.list/testsuite?rev=65204).

## Contact us

Tell the other developers about the new code using the following e-mail:

grass-dev@…

To subscribe to this mailing list, see
[​http://lists.osgeo.org/mailman/listinfo/grass-dev](http://lists.osgeo.org/mailman/listinfo/grass-dev)

In case of questions feel free to contact the developers at the above mailing list.

[​http://grass.osgeo.org/development/](http://grass.osgeo.org/development/)
