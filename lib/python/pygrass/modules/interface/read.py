# -*- coding: utf-8 -*-
"""
Created on Tue Apr  2 18:30:34 2013

@author: pietro
"""
from __future__ import print_function


def read_keydesc(par):
    name = par.text.strip()
    items = [e.text.strip() for e in par.findall('item')]
    #import ipdb; ipdb.set_trace()
    return name, tuple(items) if len(items) > 1 else None


#
# this dictionary is used to extract the value of interest from the xml
# the lambda experssion is used to define small simple functions,
# is equivalent to: ::
#
# def f(p):
#     return p.text.strip()
#
# and then we call f(p)
#
GETFROMTAG = {
    'description': lambda p: p.text.strip(),
    'keydesc': read_keydesc,
    'gisprompt': lambda p: dict(p.items()),
    'default': lambda p: p.text.strip(),
    'values': lambda p: [e.text.strip() for e in p.findall('value/name')],
    'value': lambda p: None,
    'guisection': lambda p: p.text.strip(),
    'label': lambda p: p.text.strip(),
    'suppress_required': lambda p: None,
    'keywords': lambda p: p.text.strip(),
}

GETTYPE = {
    'string': str,
    'integer': int,
    'float': float,
    'double': float,
    'file': str,
    'all': lambda x: x,
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
    'flag': """{name}: {default}
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
    If True wait untill the end of the module execution, and store the module
    outputs into stdout, stderr attributes of the class.
stdin_: PIPE, optional
    Set the standard input.
env_: dictionary, optional
    Set the evironment variables.
"""}
