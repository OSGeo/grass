
import sys
import os
import re

__all__ = ["Formatter"]

try:
    version = os.environ['VERSION_NUMBER']
except:
    version = ""

styles = {
    'b': "\\fB@\\fR",
    'i': "\\fI@\\fR",
    'em': "\\fI@\\fR",
    'code': "\\fC@\\fR",
    'span': "\\fC@\\fR",
    'sup': "\\u@\\d",
    'hr': ""
}

formats = {
    'br': "\n.br\n",
    'h2': "\n.SH @",
    'h3': "\n.SS @",
    'h4': "\n.SS @",
    'dt': ("\n.IP \"@\" 4m", 'no_nl'),
    'dd': "\n.br\n@",
    'ul': ("\n.RS 4n\n@\n.RE\n", 'in_ul'),
    'menu': ("\n.RS 4n\n@\n.RE\n", 'in_ul'),
    'dir': ("\n.RS 4n\n@\n.RE\n", 'in_ul'),
    'ol': ("\n.IP\n@\n.PP\n", 'index'),
    'p': "\n.PP\n@",
    'pre': ("\n.br\n.nf\n\\fC\n@\n\\fR\n.fi\n", 'preformat')
}

formats.update(styles)


def is_string(x):
    return isinstance(x, str)


def is_tuple(x):
    return isinstance(x, tuple)


def is_list(x):
    return isinstance(x, list)


def is_blank(s):
    return is_string(s) and s.strip() == ""


def clean(content):
    return [item for item in content if not is_blank(item)]


class Formatter:

    def __init__(self, filename, stream=sys.stdout):
        self.stream = stream
        self.style = dict(preformat=False,
                          in_ul=False,
                          no_nl=False,
                          in_table=False,
                          in_tr=False,
                          index=[])
        self.stack = []
        self.strip_re = re.compile("^[ \t]+")
        self.filename = filename
        self.at_bol = True

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
        if s != '':
            self.at_bol = s.endswith('\n')

    def pp_with(self, content, var, val):
        self.push()
        self.set(var, val)
        self.pp(content)
        self.pop()

    def fmt(self, format, content, var=None):
        # String.partition is only in 2.5+
        # (pre,sep,post) = format.partition("@")
        if self.get('no_nl') and '\n' in format:
            self.warning("can't handle line breaks in <dt>...</dt>")
            format = "@"
        f = format.split('@', 1)
        pre = f[0]
        if len(f) > 1:
            sep = '@'
            post = f[1]
        else:
            sep = ''
            post = ''

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
            sec = ".".join(map(str, idx))
            self.show("\n.IP \\fB%s\\fR\n" % sec)
            self.set('index', idx)
            self.pp(content)

    def pp_title(self):
        self.show("\n.TH " +
                  os.path.basename(self.filename).replace(".html", "") +
                  " 1 \"\" \"GRASS " +
                  version +
                  "\" \"GRASS GIS User's Manual\"")

    def pp_tr(self, content):
        content = clean(content)
        self.push(in_tr=True)
        col = 0
        for item in content:
            if not is_tuple(item):
                self.warning("invalid item in table row: %s" % str(item))
                continue
            (tag, attrs, body) = item
            if tag not in ['td', 'th']:
                self.warning("invalid tag in table row: %s" % tag)
                continue
            if col > 0:
                self.show("\t \t")
            self.show("T{\n")
            self.pp(body)
            self.show("\nT}")
            col += 1
        self.show("\n")
        self.pop()

    def pp_tbody(self, content):
        for item in content:
            if is_tuple(item):
                (tag, attrs, body) = item
                if tag in ['thead', 'tbody', 'tfoot']:
                    self.pp_tbody(body)
                elif tag == 'tr':
                    self.pp_tr(body)
                    self.show(".sp 1\n")

    def count_cols(self, content):
        cols = 0
        for item in content:
            n = 0
            if is_blank(item):
                pass
            elif is_tuple(item):
                (tag, attrs, body) = item
                if tag in ['thead', 'tbody', 'tfoot']:
                    n = self.count_cols(body)
                elif tag == 'tr':
                    n = len(clean(body))
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
            self.push(in_table=True)
            self.pp_table(content)
            self.pop()
        elif tag == 'li':
            self.pp_li(content)
        elif tag == 'title':
            self.pp_title()
        else:
            self.pp(content)

    def pp_string(self, content):
        if content == "":
            return
        s = content
        if self.get('no_nl'):
            s = s.replace("\n", " ")
        s = s.replace("\\", "\\(rs")
        s = s.replace("'", "\\(cq")
        s = s.replace("\"", "\\(dq")
        s = s.replace("`", "\\(ga")
        s = s.replace("-", "\\-")
        if self.at_bol and s[0] in [".", "'"]:
            s = "\\&" + s
        self.show(s)

    def pp_text(self, content):
        if content == "":
            return
        lines = content.splitlines(True)
        if len(lines) != 1:
            for line in lines:
                self.pp_text(line)
            return
        else:
            content = lines[0]
        if self.at_bol and not self.get('preformat'):
            content = self.strip_re.sub('', content)
        self.pp_string(content)

    def pp_list(self, content):
        for item in content:
            self.pp(item)

    def pp(self, content):
        if is_list(content):
            self.pp_list(content)
        elif is_tuple(content):
            (tag, attrs, body) = content
            self.pp_tag(tag, body)
        elif is_string(content):
            self.pp_text(content)
