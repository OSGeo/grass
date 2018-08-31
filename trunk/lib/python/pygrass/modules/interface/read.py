# -*- coding: utf-8 -*-
"""
Created on Tue Apr  2 18:30:34 2013

@author: pietro
"""
from __future__ import (nested_scopes, generators, division, absolute_import,
                        with_statement, print_function, unicode_literals)


def do_nothing(p):
    return p


def get_None(p):
    return None


def get_dict(p):
    return dict(p.items())


def get_values(p):
    return [e.text.strip() for e in p.findall('value/name')]


def read_text(p):
    return p.text.strip()


def read_keydesc(par):
    name = par.text.strip()
    items = [e.text.strip() for e in par.findall('item')]
    return name, tuple(items) if len(items) > 1 else None


GETFROMTAG = {
    'description': read_text,
    'keydesc': read_keydesc,
    'gisprompt': get_dict,
    'default': read_text,
    'values': get_values,
    'value': get_None,
    'guisection': read_text,
    'label': read_text,
    'suppress_required': get_None,
    'keywords': read_text,
    'guidependency': read_text,
    'rules': get_None,
}

GETTYPE = {
    'string': str,
    'integer': int,
    'float': float,
    'double': float,
    'file': str,
    'all': do_nothing,
}


def element2dict(xparameter):
    diz = dict(xparameter.items())
    for p in xparameter:
        if p.tag in GETFROMTAG:
            diz[p.tag] = GETFROMTAG[p.tag](p)
        else:
            print('New tag: %s, ignored' % p.tag)
    return diz


# dictionary used to create docstring for the objects
DOC = {
    #------------------------------------------------------------
    # head
    'head': """{cmd_name}({cmd_params})

Parameters
----------

""",
    #------------------------------------------------------------
    # param
    'param': """{name}: {default}{required}{multi}{ptype}
    {description}{values}{keydescvalues}""",
    #------------------------------------------------------------
    # flag_head
    'flag_head': """
Flags
------
""",
    #------------------------------------------------------------
    # flag
    'flag': """{name}: {default}, {supress}
    {description}""",
    #------------------------------------------------------------
    # foot
    'foot': """
Special Parameters
------------------

The Module class have some optional parameters which are distinct using a final
underscore.

run_: True, optional
    If True execute the module.
finish_: True, optional
    If True wait until the end of the module execution, and store the module
    outputs into stdout, stderr attributes of the class.
stdin_: PIPE, optional
    Set the standard input.
env_: dictionary, optional
    Set the environment variables.
"""}
