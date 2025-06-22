"""Robust apply mechanism

Provides a function "call", which can sort out
what arguments a given callable object can take,
and subset the given arguments to match only
those which are acceptable.
"""

import sys

if sys.hexversion >= 0x3000000:
    im_func = "__func__"
    im_self = "__self__"
    im_code = "__code__"
    func_code = "__code__"
else:
    im_func = "im_func"
    im_self = "im_self"
    im_code = "im_code"
    func_code = "func_code"


def function(receiver):
    """Get function-like callable object for given receiver

    returns (function_or_method, codeObject, fromMethod)

    If fromMethod is true, then the callable already
    has its first argument bound
    """
    if hasattr(receiver, "__call__"):
        # Reassign receiver to the actual method that will be called.
        if hasattr(receiver.__call__, im_func) or hasattr(receiver.__call__, im_code):
            receiver = receiver.__call__
    if hasattr(receiver, im_func):
        # an instance-method...
        return receiver, getattr(getattr(receiver, im_func), func_code), 1
    if not hasattr(receiver, func_code):
        raise ValueError("unknown receiver type %s %s" % (receiver, type(receiver)))
    return receiver, getattr(receiver, func_code), 0


VAR_ARGS = 4
VAR_NAMES = 8


def robustApply(receiver, *arguments, **named):
    """Call receiver with arguments and an appropriate subset of named

    The effect of this wrapper is to allow for specifying a large number
    of parameters which may not exist in the final function via named
    parameters, and have those parameters ignored in the final call.
    """
    receiver, codeObject, startIndex = function(receiver)
    has_varargs = bool(codeObject.co_flags & VAR_ARGS)
    has_varnames = bool(codeObject.co_flags & VAR_NAMES)

    posonly_count = getattr(codeObject, "co_posonlyargcount", 0)

    posnamed_arguments = codeObject.co_varnames[posonly_count : codeObject.co_argcount]
    named_onlyarguments = codeObject.co_varnames[
        codeObject.co_argcount : len(codeObject.co_varnames)
        + (-1 if has_varnames else 0)
        + (-1 if has_varargs else 0)
    ]

    # Implements: You can't have a parameter in both args and keywords,
    #             reporting an easily debugged message
    # Implements: You can't have a posonly arg in named (as a side effect of the above)
    for name in codeObject.co_varnames[
        0 : min((len(arguments), codeObject.co_argcount))
    ]:
        if name in named:
            raise TypeError(
                """Argument %r specified both positionally and as a keyword"""
                """ for calling %r""" % (name, receiver)
            )
    # Implements: You can only passed keyword parameters if the parameter exists and is
    # not a positional-only parameter or a varargs or varkeywords parameter.
    # Note that this silently drops TypeErrors for passing the name of the varargs or
    # varkeyword variables because it only allows through the valid arg-names for
    # the function
    if not has_varnames:
        acceptable = (
            posnamed_arguments[len(arguments) - posonly_count :] + named_onlyarguments
        )
        # fc does not have a **kwargs type parameter, therefore
        # remove unacceptable arguments.
        named = {k: v for k, v in named.items() if k in acceptable}
    return receiver(*arguments, **named)
