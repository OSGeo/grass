from __future__ import (absolute_import, division, generators, nested_scopes,
                        print_function, unicode_literals, with_statement)
import sys

try:
    # Python 2 import
    import HTMLParser as base
    HTMLParseError = base.HTMLParseError
except:
    # Python 3 import
    import html.parser as base
    # TODO: this needs a better fix since HTMLParseError is actually
    # used including its attributes, so that actually fails
    # HTMLParseError is depreciated, parsing is not strict
    HTMLParseError = Exception

try:
    # Python 3
    from html.entities import entitydefs
except ImportError:
    # Python 2
    from htmlentitydefs import entitydefs


__all__ = ["HTMLParser", "HTMLParseError"]

omit_start = ["body", "tbody", "head", "html"]

single = ["area", "base", "basefont", "br", "col", "frame",
          "hr", "img", "input", "isindex", "link", "meta", "param"]
single = frozenset(single)

heading = ["h1", "h2", "h3", "h4", "h5", "h6"]
fontstyle = ["tt", "i", "b", "u", "s", "strike", "big", "small"]
phrase = ["em", "strong", "dfn", "code", "samp", "kbd", "var", "cite", "abbr",
          "acronym"]
special = ["a", "img", "applet", "object", "font", "basefont", "br", "script",
           "map", "q", "sub", "sup", "span", "bdo", "iframe"]
formctrl = ["input", "select", "textarea", "label", "button"]
lists = ["ul", "ol", " dir", "menu"]
head_misc = ["script", "style", "meta", "link", "object"]
pre_exclusion = ["img", "object", "applet", "big", "small", "sub", "sup",
                 "font", "basefont"]
block = ["p", "pre", "dl", "div", "center", "noscript", "noframes",
         "blockquote", "form", "isindex", "hr", "table", "fieldset",
         "address"] + heading + lists
inline = fontstyle + phrase + special + formctrl
flow = block + inline
html_content = ["head", "body"]
head_content = ["title", "isindex", "base"]


def setify(d):
    return dict([(key, frozenset(val)) for key, val in d.items()])


def omit(allowed, tags):
    result = {}
    for k, v in allowed.items():
        for t in tags:
            if t in v:
                v = v.union(allowed[t])
        result[k] = v
    return result

allowed = {
    "a": inline,
    "abbr": inline,
    "acronym": inline,
    "address": inline + ["p"],
    "applet": flow + ["param"],
    "b": inline,
    "bdo": inline,
    "big": inline,
    "blockquote": flow,
    "body": flow + ["ins", "del"],
    "button": flow,
    "caption": inline,
    "center": flow,
    "cite": inline,
    "code": inline,
    "colgroup": ["col"],
    "dd": flow,
    "del": flow,
    "dfn": inline,
    "dir": ["li"],
    "div": flow,
    "dl": ["dt", "dd"],
    "dt": inline,
    "em": inline,
    "fieldset": flow + ["legend"],
    "font": inline,
    "form": flow,
    "frameset": ["frameset", "frame", "noframes"],
    "h1": inline,
    "h2": inline,
    "h3": inline,
    "h4": inline,
    "h5": inline,
    "h6": inline,
    "head": head_content + head_misc,
    "html": html_content,
    "i": inline,
    "iframe": flow,
    "ins": flow,
    "kbd": inline,
    "label": inline,
    "legend": inline,
    "li": flow,
    "map": block + ["area"],
    "menu": ["li"],
    "noframes": flow,
    "noscript": flow,
    "object": flow + ["param"],
    "ol": ["li"],
    "optgroup": ["option"],
    "option": [],
    "p": inline,
    "pre": inline,
    "q": inline,
    "s": inline,
    "samp": inline,
    "script": [],
    "select": ["optgroup", "option"],
    "small": inline,
    "span": inline,
    "strike": inline,
    "strong": inline,
    "style": [],
    "sub": inline,
    "sup": inline,
    "table": ["caption", "col", "colgroup", "thead", "tfoot", "tbody"],
    "tbody": ["tr"],
    "td": flow,
    "textarea": [],
    "tfoot": ["tr"],
    "th": flow,
    "thead": ["tr"],
    "title": [],
    "tr": ["th", "td"],
    "tt": inline,
    "u": inline,
    "ul": ["li"],
    "var": inline
}

allowed = setify(allowed)
allowed = omit(allowed, omit_start)

excluded = {
    "a": ["a"],
    "button": formctrl + ["a", "form", "isindex", "fieldset", "iframe"],
    "dir": block,
    "form": ["form"],
    "label": ["label"],
    "menu": block,
    "pre": pre_exclusion
}

excluded = setify(excluded)


class HTMLParser(base.HTMLParser):

    def __init__(self, entities=None):
        base.HTMLParser.__init__(self)
        self.tag_stack = []
        self.excluded = frozenset()
        self.excluded_stack = []
        self.data = []
        self.data_stack = []
        self.decls = []
        if entities:
            self.entities = entities
        else:
            self.entities = {}

    def top(self):
        if self.tag_stack == []:
            return None
        else:
            return self.tag_stack[-1][0]

    def pop(self):
        self.excluded = self.excluded_stack.pop()
        data = self.data
        self.data = self.data_stack.pop()
        (tag, attrs) = self.tag_stack.pop()
        self.append((tag, attrs, data))
        return tag

    def push(self, tag, attrs):
        self.tag_stack.append((tag, attrs))
        self.excluded_stack.append(self.excluded)
        if tag in excluded:
            self.excluded = self.excluded.union(excluded[tag])
        self.data_stack.append(self.data)
        self.data = []

    def append(self, item):
        self.data.append(item)

    def is_allowed(self, tag):
        return tag not in self.excluded and tag in allowed[self.top()]

    def handle_starttag(self, tag, attrs):
        if self.tag_stack != []:
            while not self.is_allowed(tag):
                self.pop()
        if tag not in single:
            self.push(tag, attrs)
        else:
            self.append((tag, attrs, None))

    def handle_entityref(self, name):
        if name in self.entities:
            self.handle_data(self.entities[name])
        elif name in entitydefs:
            self.handle_data(entitydefs[name])
        else:
            sys.stderr.write("unrecognized entity: %s\n" % name)

    def handle_charref(self, name):
        sys.stderr.write('unsupported character reference <%s>' % name)

    def handle_data(self, data):
        self.append(data)

    def handle_endtag(self, tag):
        while True:
            if self.pop() == tag:
                break

    def handle_decl(self, decl):
        self.decls.append(decl)
