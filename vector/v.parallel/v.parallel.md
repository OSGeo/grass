## DESCRIPTION

*v.parallel* creates parallel lines to the input vector lines which can
be used as half-buffers.

## NOTES

Usage of **-b** flag will overrule the **side** parameter.

## KNOWN ISSUES

There is a problem with side-offset parallel line generation for inside
corners. To avoid this problem, the **-b** flag might be used.

## SEE ALSO

*[v.buffer](v.buffer.md)*

## AUTHORS

Radim Blazek  
Rewritten by Rosen Matev (with support through the Google Summer of Code
program 2008)
