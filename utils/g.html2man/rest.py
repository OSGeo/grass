import sys


def match(node, tag, attr=None, val=None):
    if not isinstance(node, tuple):
        return False
    if node[0] != tag:
        return False
    if attr is not None:
        attrs = dict(node[1])
        if attr not in attrs:
            return False
        if attrs[attr] != val:
            return False
    return True


def find(node, tag, attr=None, val=None):
    if isinstance(node, tuple):
        node = node[2]
    if not isinstance(node, list):
        raise ValueError('child not found')
    for child in node:
        if match(child, tag, attr, val):
            return child
    raise ValueError('child not found')


def children(node):
    return node[2]


def text(node):
    return children(node)[0]


def _(s):
    return s        # TODO


def rest(root, f=sys.stdout):
    def write(text):
        f.write(text)

    def show(item, italic=False, bold=False):
        if isinstance(item, str):
            spc = ''  # if item[-1] == '\n' else ' '
            fmt = '**' if bold else ('*' if italic else '')
            write('%s%s%s%s' % (fmt, item, fmt, spc))
        elif match(item, 'b'):
            for i in children(item):
                show(i, italic, True)
        elif match(item, 'i') or match(item, 'em'):
            for i in children(item):
                show(i, True, bold)

    html = find(root, 'html')

    title = text(find(find(html, 'head'), 'title'))
    rule = '=' * len(title)
    write('%s\n' % rule)
    write('%s\n' % title)
    write('%s\n' % rule)
    write('\n')
    write(".. figure:: grass_logo.png\n")
    write("   :align: center\n")
    write("   :alt: GRASS logo\n")
    write('\n')

    body = find(html, 'body')
    section = None
    for child in children(body):
        if match(child, 'h2'):
            section = text(child)
            rule = '-' * len(section)
            write("%s\n%s\n" % (section, rule))
        elif section == _('NAME'):
            if match(child, 'em'):
                name = text(find(child, 'b'))
                write("**%s**" % name)
                section = 'desc'
        elif section == 'desc' and isinstance(child, str) and child[:4] == '  - ':
            write(' - ')
            write(child[4:])
            if child[-1] != '\n':
                write('\n')
            write('\n')
            section = None
        elif section == _('KEYWORDS'):
            write(child.strip())
            write('\n\n')
            section = None
        elif section == _('SYNOPSIS'):
            if match(child, 'div', 'id', 'name'):
                name = text(find(child, 'b'))
                write("**%s**\n\n" % name)
                write("**%s** help\n\n" % name)
            elif match(child, 'div', 'id', 'synopsis'):
                for item in children(child):
                    show(item)
                write('\n')
            elif match(child, 'div', 'id', 'flags'):
                header = text(find(child, 'h3'))
                rule = '=' * len(header)
                write('%s\n%s\n' % (header, rule))
                flags = find(child, 'dl')
                for flag in children(flags):
                    if match(flag, 'dt'):
                        item = text(find(flag, 'b'))
                        write('**%s**\n' % item)
                    elif match(flag, 'dd'):
                        write('    %s\n' % text(flag))
                write('\n\n')
            elif match(child, 'div', 'id', 'parameters'):
                header = text(find(child, 'h3'))
                rule = '=' * len(header)
                write('%s\n%s\n' % (header, rule))
                params = find(child, 'dl')
                for param in children(params):
                    if match(param, 'dt'):
                        name = text(children(param)[0])
                        write('**%s** = ' % name)
                        for item in children(param)[2:]:
                            show(item)
                        write('\n\n')
                    elif match(param, 'dd'):
                        write('\t')
                        for item in children(param):
                            show(item)
                        write('\n\n')
                write('\n')
                return
