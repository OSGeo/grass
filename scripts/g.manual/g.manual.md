## DESCRIPTION

*g.manual* displays the manual pages of GRASS in HTML and MAN format.

## NOTES

The name of the browser is defined in the environment variable
`GRASS_HTML_BROWSER`. For most platforms this should be an executable in
your PATH, or the full path to an executable. See
[variables](variables.html) for details.

## EXAMPLES

Show index page in the browser.

::: code
    g.manual -i
:::

Show manual page of *[d.vect](d.vect.html)* module in the browser.

::: code
    g.manual d.vect
:::

Show module manual page in terminal.

::: code
    g.manual -m d.vect
:::

## AUTHOR

Markus Neteler
