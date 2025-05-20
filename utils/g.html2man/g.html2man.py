#!/usr/bin/env python3
from pathlib import Path
import sys
import re
from ghtml import HTMLParser
from ggroff import Formatter

from io import StringIO

entities = {"nbsp": " ", "bull": "*"}

# Remove ToC


def fix(content):
    if isinstance(content, tuple):
        tag, attrs, body = content
        if tag == "div" and ("class", "toc") in attrs:
            return None
        return (tag, attrs, fix(body))
    if isinstance(content, list):
        return [fixed for item in content for fixed in [fix(item)] if fixed is not None]
    return content


def main():
    # parse HTML
    infile = sys.argv[1]
    with open(infile) as inf:
        p = HTMLParser(entities)
        for n, line in enumerate(inf):
            try:
                p.feed(line)
            except Exception as err:
                sys.stderr.write(
                    "%s:%d:0: Error (%s): %s\n" % (infile, n + 1, repr(err), line)
                )
                sys.exit(1)
        p.close()

    # generate groff
    sf = StringIO()
    f = Formatter(infile, sf)
    f.pp(fix(p.data))
    s = sf.getvalue()
    sf.close()

    # strip excess whitespace
    blank_re = re.compile(r"[ \t\n]*\n([ \t]*\n)*")
    s = blank_re.sub("\n", s)
    s = s.lstrip()

    # write groff
    s = s.encode("UTF-8")
    Path(sys.argv[2]).write_bytes(s)


if __name__ == "__main__":
    main()
