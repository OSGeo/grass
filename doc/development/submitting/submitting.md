# GRASS GIS development

GRASS - Geographic Resources Analysis Support System has been under continual
development since 1982. The strength and success of GRASS GIS relies on the user
community. With this in mind, the philosophy of the GRASS Development Team is to
encourage users to develop their own unique tools and applications for GRASS. If
you develop tools and techniques that you feel would benefit other GRASS users,
please see below how to structure code and documentation for the benefit of all.

## Submitting rules

Be sure to check your code against these rules:

- [General notes](./general.md)
- [C code](./submitting_c.md)
- [Python code](./python.md)
- [wxGUI code](./wxGUI.md) (wxPython-based GUI code)
- [Documentation-related notes](./docs.md) (HTML, MAN)
- [User message standardization](./message_standardization.md): formatting,
  standard phrases, i18N etc.

### Use pre-commit

It is highly recommended to install and use [pre-commit](https://pre-commit.com)
before submitting any new or modified code or any other content. The pre-commit
Git hooks set checks validity and executes automated formatting for
a range of file formats, including C/C++ and Python. Pre-commit installs
all necessary tools in a virtual environment upon first use.

If you never used pre-commit before, you must start by installing it on your
system. You only do it once:

```bash
python -m pip install pre-commit
```

Pre-commit must then be activated in the code repository. Change the directory
to the root folder and use the `install` command:

```bash
cd <grass_source_dir>

# once per repo
pre-commit install
```

Pre-commit will then be automatically triggered by the `git commit` command. If
it finds any problem it will abort the commit and try to solve it automatically.
In that case review the changes and run again `git add` and
`git commit`.

It is also possible to run pre-commit manually, e.g:

```bash
pre-commit run clang-format --all-files
pre-commit run black --all-files
```

Or to target a specific set of files:

```bash
pre-commit run --files raster/r.somemodule/*
```

The pre-commit hooks set is defined in
[.pre-commit-config.yaml](../../../.pre-commit-config.yaml).

It is possible to temporally disable the pre-commit hooks in the repo, e.g. while
working on older branches:

```bash
# backporting...
pre-commit uninstall
```

And to reactivate pre-commit again:

```bash
git switch main
pre-commit install
```

## GRASS GIS programming best practice

There are many unwritten rules how GRASS modules should work, what they should
do and what they shouldn't do. There is always some reason why some things are
considered as "good" or "bad", still they are just noted in some long GRASS
developer mailing list conversations. These pages here aim at collecting such
ideas which are floating around in the
[​GRASS-dev](https://lists.osgeo.org/mailman/listinfo/grass-dev) mailing list (and
other places) to help new module developers/bugfixers to understand many little
tricks how GRASS modules should work.

### New list item adding guide

List items should be short and general. Add only things that are relevant to all
modules or module groups. There should be reason why such rule/hint exists - add
reference to ML archive thread or short description why such rule is important.
Look into the documentation above for already existing specific rules. Feel free
to add code/pseudocode samples, if they apply.

## GRASS best practice list (unsorted)

- Read the rules above
- All GRASS modules should accept map names in format "map@mapset". [​https://lists.osgeo.org/pipermail/grass-dev/2008-February/035629.html](https://lists.osgeo.org/pipermail/grass-dev/2008-February/035629.html)
- Module should **not** write/change maps in other mapsets than current mapset. [​https://lists.osgeo.org/pipermail/grass-dev/2008-February/035637.html](https://lists.osgeo.org/pipermail/grass-dev/2008-February/035637.html)

## GRASS GIS Addons

- Check your code against the **Submitting rules** (see above)
- Upload your code with the git client ([git usage](https://trac.osgeo.org/grass/wiki/HowToGit))
- Once uploaded to the GRASS GIS Addons GitHub repository:
  - Addons appear in the [​Addons manual pages](https://grass.osgeo.org/grass-stable/manuals/addons/)
    when being registered in the parent Makefile
  - note to devs only: the addons are created via cronjobs on the server (user
    can install them via g.extension)

- GRASS GIS Python Addons
  - with dependencies on external, non-standard modules should use lazy imports:
    [​https://lists.osgeo.org/pipermail/grass-dev/2018-October/090321.html](https://lists.osgeo.org/pipermail/grass-dev/2018-October/090321.html)
  - that represent sets of modules and eventually also share functions across
    modules can be grouped into one addon directory, like e.g.:
    - [​https://github.com/OSGeo/grass-addons/tree/master/grass7/gui/wxpython/wx.metadata](https://github.com/OSGeo/grass-addons/tree/master/grass7/gui/wxpython/wx.metadata)
      or
    - [​https://github.com/OSGeo/grass-addons/tree/master/grass7/raster/r.green](https://github.com/OSGeo/grass-addons/tree/master/grass7/raster/r.green)

## Submitting code to GitHub

See [HowToGit](https://trac.osgeo.org/grass/wiki/HowToGit)

## See also

- [​https://grass.osgeo.org/contribute/development/](https://grass.osgeo.org/contribute/development/)
