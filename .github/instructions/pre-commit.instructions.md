---
applyTo: "**/*"
---
### Using pre-commit

It is highly recommended to install and use [pre-commit](https://pre-commit.com)
before submitting any new or modified code or any other content. The pre-commit
uses Git hooks to check validity and executes automated formatting for
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
pre-commit run --all-files
pre-commit run clang-format --all-files
pre-commit run ruff-format --all-files
```

Or to target a specific set of files:

```bash
pre-commit run --files raster/r.sometool/*
```

The pre-commit hooks set is defined in
[.pre-commit-config.yaml](../../.pre-commit-config.yaml).

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
