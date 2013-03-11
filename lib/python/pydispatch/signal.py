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
    def __init__(self, name):
        self._name = name

    def connect(self, handler, weak=None):
        if weak is None:
            if _islambda(handler):
                weak = False
            else:
                weak = True
        dispatcher.connect(receiver=handler, signal=self, weak=weak)

    def disconnect(self, handler):
        dispatcher.disconnect(receiver=handler, signal=self, weak=None)

    def emit(self, *args, **kwargs):
        dispatcher.send(signal=self, *args, **kwargs)

    def __call__(self, *arguments, **named):
        if 'signal' in named:
            del named['signal']
        self.emit(*arguments, **named)


if __name__ == '__main__':
    def handler1():
        print "handler1"
    def handler2(text):
        print "handler2: %s" % text
    class A(object):
        def showText(self, text):
            print "showing text:", text
        def showMessage(self):
            print "showing message"

    def test():
        import sys
        signal1 = Signal('signal1')
        signal2 = Signal('signal2')
        signal3 = Signal('signal3')
        signal1.connect(handler1)
        signal2.connect(handler2)
        signal2.connect(lambda text: sys.stdout.write('lambda handler 1: %s\n' % text))
        signal2.connect(signal3)
        signal3.connect(handler2)

        a = A()
        signal2.connect(a.showText)
        signal2.connect(a.showMessage)

        signal1.emit()
        signal2.emit(text="Hello")

    test()