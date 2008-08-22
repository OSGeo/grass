#!/usr/bin/env python
# coding=iso-8859-1
import sys
import types
import os
import re
from HTMLParser import HTMLParser
from htmlentitydefs import entitydefs
from StringIO import StringIO

try:
    version = os.environ['VERSION_NUMBER']
except:
    version = ""

entities = {
    'lt': "<",
    'gt': ">",
    'amp': "&",
    'nbsp': " ",
    'copy': "©",
    'quot': "\"",
    'bull': "*"
    }

single = ["area", "base", "basefont", "br", "col", "frame",
	  "hr", "img", "input", "isindex", "link", "meta", "param"]
single = frozenset(single)

heading = ["h1", "h2", "h3", "h4", "h5", "h6"]
fontstyle = ["tt", "i", "b", "u", "s", "strike", "big", "small"]
phrase = [ "em", "strong", "dfn", "code", "samp", "kbd", "var", "cite", "abbr",
	   "acronym"]
special = [ "a", "img", "applet", "object", "font", "basefont", "br", "script",
	    "map", "q", "sub", "sup", "span", "bdo", "iframe"]
formctrl = [ "input", "select", "textarea", "label", "button"]
list = [ "ul", "ol", " dir", "menu"]
head_misc = [ "script", "style", "meta", "link", "object"]
pre_exclusion = [ "img", "object", "applet", "big", "small", "sub", "sup",
		  "font", "basefont"]
block = [ "p", "pre", "dl", "div", "center", "noscript", "noframes",
	  "blockquote", "form", "isindex", "hr", "table", "fieldset",
	  "address"] + heading + list
inline = fontstyle + phrase + special + formctrl
flow = block + inline
html_content = ["head", "body"]
head_content = ["title", "isindex", "base"]

def setify(d):
    return dict([(key, frozenset(val)) for key, val in d.iteritems()])

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
    "table": ["caption", "col", "colgroup", "thead", "tfoot", "tbody",
	      "tr"], # to allow for <table>[implied <tbody>]<tr>
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

styles = {
    'b':        "\\fB@\\fR",
    'i':        "\\fI@\\fR",
    'em':       "\\fI@\\fR",
    'code':     "\\fC@\\fR",
    'span':     "\\fC@\\fR",
    'sup':      "\\u@\\d",
    'hr':       ""
    }

formats = {
    'br':       "\n.br\n",
    'h2':       "\n.SH @",
    'h3':       "\n.SS @",
    'h4':       "\n.SS @",
    'dt':       ("\n.IP \"@\" 4m", 'no_nl'),
    'dd':       "\n.br\n@",
    'ul':       ("\n.RS 4n\n@\n.RE\n", 'in_ul'),
    'menu':     ("\n.RS 4n\n@\n.RE\n", 'in_ul'),
    'dir':      ("\n.RS 4n\n@\n.RE\n", 'in_ul'),
    'ol':       ("\n.IP\n@\n.PP\n", 'index'),
    'p':        "\n.PP\n@",
    'pre':      ("\n.br\n.nf\n\\fC\n@\n\\fR\n.fi\n", 'preformat')
    }

formats.update(styles)

def is_string(x):
    return isinstance(x, types.StringType)

def is_tuple(x):
    return isinstance(x, types.TupleType)

def is_list(x):
    return isinstance(x, types.ListType)

def is_blank(s):
    return is_string(s) and s.strip() == ""

def clean(content):
    return [item for item in content if not is_blank(item)]

class Formatter:
    def __init__(self, stream = sys.stdout):
	self.stream = stream
	self.style = dict(preformat = False,
			  in_ul = False,
			  no_nl = False,
			  in_table = False,
			  in_tr = False,
			  index = [])
	self.stack = []
	self.strip_re = re.compile("\n[ \t]+")

    def warning(self, msg):
	sys.stderr.write(msg + '\n')

    def set(self, var, val):
	self.style[var] = val

    def get(self, var):
	return self.style[var]

    def push(self, **kwargs):
	self.stack.append(self.style.copy())
	self.style.update(**kwargs)

    def pop(self):
	self.style = self.stack.pop()

    def show(self, s):
	self.stream.write(s)

    def pp_with(self, content, var, val):
	self.push()
	self.set(var, val)
	self.pp(content)
	self.pop()

    def fmt(self, format, content, var = None):
	(pre,sep,post) = format.partition("@")
	if pre != "":
	    self.show(pre)
	if sep != "":
	    if var:
		if var == 'index':
		    val = self.get('index') + [0]
		else:
		    val = True
		self.pp_with(content, var, val)
	    else:
		self.pp(content)
	if post != "":
	    self.show(post)

    def pp_li(self, content):
	if self.get('in_ul'):
	    self.fmt("\n.IP \(bu 4n\n@", content)
	else:
	    idx = self.get('index')
	    idx[-1] += 1
	    sec = ".".join(map(str,idx))
	    self.show("\n.IP \\fB%s\\fR\n" % sec)
	    self.set('index', idx)
	    self.pp(content)

    def pp_title(self):
	self.show("\n.TH " +
		  os.path.basename(sys.argv[1]).replace(".html","") +
		  " 1 \"\" \"GRASS " +
		  version +
		  "\" \"Grass User's Manual\"")

    def pp_tr(self, content):
	content = clean(content)
	self.push(in_tr = True)
	col = 0
	for item in content:
	    if not is_tuple(item):
		self.warning("invalid item in table row: %s" % str(item))
		continue
	    (tag, tail) = item
	    if tag not in ['td', 'th']:
		self.warning("invalid tag in table row: %s" % tag)
		continue
	    if col > 0:
		self.show("\t \t")
	    self.show("T{\n")
	    self.pp(tail)
	    self.show("\nT}")
	    col += 1
	self.show("\n")
	self.pop()

    def pp_tbody(self, content):
	for item in content:
	    if is_tuple(item):
		(tag, tail) = item
		if tag in ['thead', 'tbody', 'tfoot']:
		    self.pp_tbody(tail)
		elif tag == 'tr':
		    self.pp_tr(tail)
		    self.show(".sp 1\n")

    def count_cols(self, content):
	cols = 0
	for item in content:
	    n = 0
	    if is_blank(item):
		pass
	    elif is_tuple(item):
		(tag, tail) = item
		if tag in ['thead', 'tbody', 'tfoot']:
		    n = self.count_cols(tail)
		elif tag == 'tr':
		    n = len(clean(tail))
		cols = max(cols, n)
	    else:
		self.warning("invalid item in table: %s" % str(item))
	return cols

    def pp_table(self, content):
	cols = self.count_cols(content)
	if cols == 0:
	    return
	self.show("\n.TS\nexpand;\n")
	self.show(" lw1 ".join(["lw60" for i in range(cols)]) + ".\n")
	self.pp_tbody(content)
	self.show("\n.TE\n")

    def pp_tag(self, tag, content):
	if self.get('in_tr') and tag not in styles:
	    self.pp(content)
	elif tag in formats:
	    spec = formats[tag]
	    if is_string(spec):
		self.fmt(spec, content)
	    else:
		(fmt, var) = spec
		self.fmt(fmt, content, var)
	elif tag == 'table':
	    if self.get('in_table'):
		self.warning("cannot handle nested tables")
		return
	    self.push(in_table = True)
	    self.pp_table(content)
	    self.pop()
	elif tag == 'li':
	    self.pp_li(content)
	elif tag == 'title':
	    self.pp_title()
	else:
	    self.pp(content)

    def pp_string(self, content):
	s = content
	if self.get('no_nl'):
	    s = s.replace("\n"," ")
	s = s.replace("\\", "\\(rs")
	s = s.replace("'", "\\(cq")
	s = s.replace("\"", "\\(dq")
	s = s.replace("`", "\\(ga")
	self.show(s)

    def pp_text(self, content):
	if content != "":
	    if self.get('preformat'):
		self.pp_string(content)
	    else:
		s = self.strip_re.sub('\n', content)
		self.pp_string(s)

    def pp_list(self, content):
	for item in content:
	    self.pp(item)

    def pp(self, content):
	if is_list(content):
	    self.pp_list(content)
	elif is_tuple(content):
	    (head, tail) = content
	    self.pp_tag(head, tail)
	elif is_string(content):
	    self.pp_text(content)

class MyHTMLParser(HTMLParser):
    def __init__(self):
	HTMLParser.__init__(self)
	self.tag_stack = []
	self.excluded = frozenset()
	self.excluded_stack = []
	self.data = []
	self.data_stack = []

    def top(self):
	if self.tag_stack == []:
	    return None
	else:
	    return self.tag_stack[-1]

    def pop(self):
	self.excluded = self.excluded_stack.pop()
	data = self.data
	self.data = self.data_stack.pop()
	tag = self.tag_stack.pop()
	self.append((tag, data))
	return tag

    def push(self, tag):
	self.tag_stack.append(tag)
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
	    self.push(tag)
	else:
	    self.append((tag,None))

    def handle_entityref(self, name):
	if name in entities:
	    self.handle_data(entities[name])
	elif name in entitydefs:
	    self.handle_data(entitydefs[name])
	else:
	    sys.stderr.write("unrecognized entity: %s\n" % name)

    def handle_data(self, data):
	self.append(data)

    def handle_endtag(self, tag):
	while True:
	    if self.pop() == tag:
		break

if __name__ == "__main__":
    # parse HTML
    inf = file(sys.argv[1])
    p = MyHTMLParser()
    p.feed(inf.read())
    p.close()
    inf.close()

    # generate groff
    sf = StringIO()
    f = Formatter(sf)
    f.pp(p.data)
    s = sf.getvalue()
    sf.close()

    # strip excess whitespace
    blank_re = re.compile("[ \t\n]*\n[ \t\n]*")
    s = blank_re.sub('\n', s)
    s = s.lstrip()

    # write groff
    outf = file(sys.argv[2], 'w')
    outf.write(s)
    outf.close()
