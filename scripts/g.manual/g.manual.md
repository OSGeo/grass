## DESCRIPTION

*g.manual* displays the manual pages of GRASS in HTML and MAN format.

## NOTES

The name of the browser is defined in the environment variable
`GRASS_HTML_BROWSER`. For most platforms this should be an executable in
your PATH, or the full path to an executable. See
[variables](variables.md) for details.

## EXAMPLES

Show index page in the browser.

```bash
g.manual -i
```

Show manual page of *[d.vect](d.vect.md)* module in the browser.

```bash
g.manual d.vect
```

Show module manual page in terminal.

```bash
g.manual -m d.vect
```

## AUTHOR

Markus Neteler
