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
    
    >>> mylambda = lambda x: x*x
    >>> _islambda(mylambda)
    True
    >>> _islambda(_islambda)
    False
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
    >>> signal2.connect(lambda text:
    ...                 sys.stdout.write('lambda handler: %s\\n' % text))
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
        """
        
        >>> signal1 = Signal('signal1')
        >>> import sys
        >>> signal1.connect(sys.stdout.write)
        >>> signal1.disconnect(sys.stdout.write)
        >>> signal1.disconnect(sys.stdout.flush)  #doctest: +ELLIPSIS
        Traceback (most recent call last):
        DispatcherKeyError: 'No receivers found for signal <__main__.Signal object at 0x...> from sender _Any'
        >>> signal1.disconnect(some_function)
        Traceback (most recent call last):
        NameError: name 'some_function' is not defined
        >>> signal1.emit()
        """
        dispatcher.disconnect(receiver=handler, signal=self, weak=weak)

    # TODO: remove args?, make it work for args?
    def emit(self, *args, **kwargs):
        """
        >>> signal1 = Signal('signal1')
        >>> def mywrite(text):
        ...     print text
        >>> signal1.connect(mywrite)
        >>> signal1.emit(text='Hello')
        Hello
        >>> signal1.emit()
        Traceback (most recent call last):
        TypeError: mywrite() takes exactly 1 argument (0 given)
        >>> signal1.emit('Hello')
        Traceback (most recent call last):
        TypeError: send() got multiple values for keyword argument 'signal'
        """
        dispatcher.send(signal=self, *args, **kwargs)

    def __call__(self, *args, **kwargs):
        """Allows to emit signal with function call syntax.

        It allows to handle signal as a function or other callable object.
        So, the signal can be in the list of fuctions or can be connected as
        a handler for another signal.
        However, it is strongly recommended to use emit method for direct
        signal emitting.
        The use of emit method is more explicit than the function call
        and thus it it clear that we are using signal.

        >>> signal1 = Signal('signal1')
        >>> def mywrite(text):
        ...     print text
        >>> signal1.connect(mywrite)
        >>> functions = [signal1, lambda text: mywrite(text + '!')]
        >>> for function in functions:
        ...     function(text='text')
        text
        text!

        The other reason why the function call should not by used when it is
        possible to use emit method is that this function does ugly hack to
        enable calling as a signal handler. The signal parameter is deleted
        when it is in named keyword arguments. As a consequence, when the
        signal is emitted with the signal parameter (which is a very bad
        name for parameter when using signals), the error is much more readable
        when using emit than function call. Concluding remark is that
        emit behaves more predictable.
        >>> signal1.emit(signal='Hello')
        Traceback (most recent call last):
        TypeError: send() got multiple values for keyword argument 'signal'
        >>> signal1(signal='Hello')
        Traceback (most recent call last):
        TypeError: mywrite() takes exactly 1 argument (0 given)
        """
        if 'signal' in kwargs:
            del kwargs['signal']
        self.emit(*args, **kwargs)


if __name__ == '__main__':
    import doctest
    doctest.testmod()
