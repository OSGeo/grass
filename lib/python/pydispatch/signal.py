# -*- coding: utf-8 -*-
"""
Created on Mon Mar 11 18:39:13 2013

@author Vaclav Petras <wenzeslaus gmail.com>
"""


from grass.pydispatch import dispatcher


def _islambda(function):
    """Tests if object is a lambda function.

    Should work on the most of Python implementations where name of lambda
    function is not unique.
    """
    return isinstance(function, type(lambda: None)) and function.__name__== (lambda: None).__name__


class Signal(object):
    """
    >>> signal1 = Signal('signal1')
    >>> def handler1():
    ...     print "from handler1"
    >>> signal1.connect(handler1)

    >>> signal2 = Signal('signal2')
    >>> def handler2(text):
    ...     print "handler2: %s" % text
    >>> signal2.connect(handler2)

    >>> signal1.emit()
    from handler1

    >>> signal2.emit(text="Hello")
    handler2: Hello

    >>> import sys
    >>> signal2.connect(lambda text: sys.stdout.write('lambda handler: %s\\n' % text))
    >>> signal2.emit(text="Hi")
    handler2: Hi
    lambda handler: Hi

    >>> def handler3():
    ...     print "from handler3"
    >>> signal2.connect(handler3)
    >>> signal2.emit(text="Ciao")
    handler2: Ciao
    lambda handler: Ciao
    from handler3

    >>> signal3 = Signal('signal3')
    >>> signal3.connect(handler3)
    >>> signal1.connect(signal3)
    >>> signal1.emit()
    from handler1
    from handler3

    >>> signal3.disconnect(handler3)
    >>> signal1.emit()
    from handler1
    >>> signal2.disconnect(handler2)
    >>> signal2.disconnect(handler3)
    >>> signal2.emit(text='Hello')
    lambda handler: Hello
    """
    def __init__(self, name):
        self._name = name

    def connect(self, handler, weak=None):
        """
        >>> signal1 = Signal('signal1')
        >>> import sys
        >>> signal1.connect(lambda: sys.stdout.write('will print\\n'))
        >>> signal1.connect(lambda: sys.stdout.write('will print\\n'), weak=False)
        >>> signal1.connect(lambda: sys.stdout.write('will not print'), weak=True)
        >>> signal1.emit()
        will print
        will print
        """
        if weak is None:
            if _islambda(handler):
                weak = False
            else:
                weak = True
        dispatcher.connect(receiver=handler, signal=self, weak=weak)

    def disconnect(self, handler, weak=True):
        dispatcher.disconnect(receiver=handler, signal=self, weak=weak)

    def emit(self, *args, **kwargs):
        dispatcher.send(signal=self, *args, **kwargs)

    def __call__(self, *arguments, **named):
        if 'signal' in named:
            del named['signal']
        self.emit(*arguments, **named)


if __name__ == '__main__':
    import doctest
    doctest.testmod()
