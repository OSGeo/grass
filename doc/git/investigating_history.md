# Investigating Code History

## Plain git blame

In command line, to investigate commits which introduced a certain
piece of code, you can use `git blame`:

```sh
git blame scripts/r.mask/r.mask.py
```

This will give you current content of the file with commit hash and author
associated with each line.

## Ignore some commits with git blame

Over the course of time, there was a lot of code reformatting
changes which you typically want to ignore. These, often whitespace-only
changes, can be filtered out using a _ignore-revs_ file in the source code
root directory:

```sh
git blame scripts/r.mask/r.mask.py --ignore-revs-file .git-blame-ignore-revs
```

In some cases, ignoring formatting changes may not be appropriate, for example
when investigating automated checks of formatting.
If you think the _ignore-revs_ file contains commits which should be never
ignored with `git blame`, please open an issue.

## On GitHub

GitHub interface does not allow to use the ignore-revs file. However, it is
easy to move over a revision by pressing "View blame prior to this change"
which is the icon between commit info and line numbers.
