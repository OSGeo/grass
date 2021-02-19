# -*- coding: utf-8 -*-


def docstring_property(class_doc):
    """Property attribute for docstrings.
    Took from: https://gist.github.com/bfroehle/4041015

    >>> class A(object):
    ...     '''Main docstring'''
    ...     def __init__(self, x):
    ...         self.x = x
    ...     @docstring_property(__doc__)
    ...     def __doc__(self):
    ...         return "My value of x is %s." % self.x

    >>> A.__doc__
    'Main docstring'

    >>> a = A(10)
    >>> a.__doc__
    'My value of x is 10.'
    """
    def wrapper(fget):
        return DocstringProperty(class_doc, fget)
    return wrapper


class DocstringProperty(object):
    """Property for the `__doc__` attribute.

    Different than `property` in the following two ways:

    * When the attribute is accessed from the main class, it returns the value
      of `class_doc`, *not* the property itself. This is necessary so Sphinx
      and other documentation tools can access the class docstring.

    * Only supports getting the attribute; setting and deleting raise an
      `AttributeError`.
    """

    def __init__(self, class_doc, fget):
        self.class_doc = class_doc
        self.fget = fget

    def __get__(self, obj, type=None):
        if obj is None:
            return self.class_doc
        else:
            return self.fget(obj)

    def __set__(self, obj, value):
        raise AttributeError("can't set attribute")

    def __delete__(self, obj):
        raise AttributeError("can't delete attribute")
