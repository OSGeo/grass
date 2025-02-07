## DESCRIPTION

*g.tempfile* is designed for shell scripts that need to use large
temporary files. GRASS provides a mechanism for temporary files that
does not depend on `/tmp/`. GRASS temporary files are created in the
data base with the assumption that there will be enough space under the
data base for large files. GRASS periodically removes temporary files
that have been left behind by programs that failed to remove them before
terminating.

*g.tempfile* creates an unique file and prints the name. The user is
required to provide a process-id which will be used as part of the name
of the file. Most Unix shells provide a way to get the process id of the
current shell. For `/bin/sh` and `/bin/csh` this is `$$`. It is
recommended that `$$` be specified as the process-id for *g.tempfile*.

## EXAMPLE

For `/bin/sh` scripts the following syntax should be used:

```sh
temp1=`g.tempfile pid=$$`
temp2=`g.tempfile pid=$$`
```

For `/bin/csh` scripts, the following can be used:

```sh
set temp1=`g.tempfile pid=$$`
set temp2=`g.tempfile pid=$$`
```

## NOTES

Each call to *g.tempfile* creates a different (i.e. unique) name.
Although GRASS does eventually get around to removing tempfiles that
have been left behind, the programmer should make every effort to remove
these files. They often get large and take up disk space. If you write
`/bin/sh` scripts, learn to use the `/bin/sh` related `trap` command. If
you write `/bin/csh` scripts, learn to use the `/bin/csh` related
`onintr` command.

## AUTHOR

Michael Shapiro, U.S. Army Construction Engineering Research Laboratory
