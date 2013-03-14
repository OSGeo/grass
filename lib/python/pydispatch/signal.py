# -*- coding: utf-8 -*-
"""
Created on Mon Mar 11 18:39:13 2013

@author Vaclav Petras <wenzeslaus gmail.com>
"""


from grass.pydispatch import dispatcher


def _islambda(function):
    """
    Tests if object is a lambda function.

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

    The signal object is created usually as a instance attribute.
    However, it can be created anywhere.

    >>> signal1 = Signal('signal1')

    The function has to be connected to a signal in order to be called when
    the signal is emitted. The connection can be done where the function is
    defined (e. g., a class) but also on some other place, typically,
    user of a class connects some signal to the method of some other class.

    >>> def handler1():
    ...     print "from handler1"
    >>> signal1.connect(handler1)

    Emitting of the signal is done usually only in the class which has the
    signal as a instance attribute. Again, generally, it can be emitted
    anywhere.

    >>> signal1.emit()
    from handler1

    The signal can have parameters. These parameters are specified when
    emitting but should be documented together with the signal (e.g., in the
    class documentation). Parameters should be keyword arguments and handlers
    must use these names (if the names cannot be the same, lambda function
    can be used to overcome this problem).

    >>> signal2 = Signal('signal2')
    >>> def handler2(text):
    ...     print "handler2: %s" % text
    >>> signal2.connect(handler2)
    >>> signal2.emit(text="Hello")
    handler2: Hello

    Do not emit the same signal with different parameters when emitting at
    different places.

    A handler is the standard function, lambda function, method or any other
    callable object.

    >>> import sys
    >>> signal2.connect(lambda text:
    ...                 sys.stdout.write('lambda handler: %s\\n' % text))
    >>> signal2.emit(text="Hi")
    handler2: Hi
    lambda handler: Hi

    The handler function can have only some of the signal parameters or no
    parameters at all even if the signal has some.

    >>> def handler3():
    ...     print "from handler3"
    >>> signal2.connect(handler3)
    >>> signal2.emit(text="Ciao")
    handler2: Ciao
    lambda handler: Ciao
    from handler3

    It is possible to use signal as a handler. By this, signals can be
    forwarded from one object to another. In other words, one object can
    expose signal of some object.

    >>> signal3 = Signal('signal3')
    >>> signal3.connect(handler3)
    >>> signal1.connect(signal3)
    >>> signal1.emit()
    from handler1
    from handler3

    It is possible to disconnect a particular handler.

    >>> signal3.disconnect(handler3)
    >>> signal1.emit()
    from handler1
    >>> signal2.disconnect(handler2)
    >>> signal2.disconnect(handler3)
    >>> signal2.emit(text='Hello')
    lambda handler: Hello
    """
    # TODO: use the name for debugging
    def __init__(self, name):
        """Creates a signal object.

        The parameter name is used for debugging.
        """
        self._name = name

    def connect(self, handler, weak=None):
        """
        Connects handler to a signal.

        Typically, a signal is defined in some class and the user of this
        class connects to the signal::

            from module import SomeClass
            ...
            self.someObject = SomeClass()
            self.someObject.connect(self.someMethod)

        Usually, it is not needed to set the weak parameter. This method
        creates weak references for all handlers but for lambda functions, it
        automaticaly creates (standard) references (otherwise, lambdas would be
        garbage collected. If you want to force some behaviour, specify the
        weak parameter.

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
        Disconnects a specified handler.

        It is not necessary to disconnect object when it is deleted.
        Underlying PyDispatcher will take care of connections to deleted
        objects.

        >>> signal1 = Signal('signal1')
        >>> import sys
        >>> signal1.connect(sys.stdout.write)
        >>> signal1.disconnect(sys.stdout.write)

        The weak parameter of must have the same value as for connection.
        If you not specified the parameter when connecting,
        you don't have to specify it when disconnecting.

        Disconnecting the not-connected handler will result in error.

        >>> signal1.disconnect(sys.stdout.flush)  #doctest: +ELLIPSIS
        Traceback (most recent call last):
        DispatcherKeyError: 'No receivers found for signal <__main__.Signal object at 0x...> from sender _Any'

        Disconnecting the non-exiting or unknown handler will result in error.
        
        >>> signal1.disconnect(some_function)
        Traceback (most recent call last):
        NameError: name 'some_function' is not defined
        >>> signal1.emit()
        """
        dispatcher.disconnect(receiver=handler, signal=self, weak=weak)

    # TODO: remove args?, make it work for args?
    # TODO: where to put documentation
    def emit(self, *args, **kwargs):
        """
        Emits the signal which means that all connected handlers will be
        called.

        It is advised to have signals as instance attributes and emit signals
        only in the class which owns the signal::

            class Abc(object):
                def __init__(self):
                    self.colorChanged = Signal('Abc.colorChanged')
                    ...
                def setColor(self, color):
                    ...
                    self.colorChanged.emit(oldColor=self.Color, newColor=color)
                    ...

        Documentation of an signal should be placed to the class documentation
        or to the code (this need to be more specified).

        Calling a signal from outside the class is usually not good
        practice. The only case when it is permitted is when signal is the part
        of some globaly shared object and permision to emit is stayed in the
        documentation.

        The parameters of the emit function must be the same as the parameters
        of the handlers. However, handler can ommit some parameters.
        The associated parameters shall be documented for each Signal instance.
        Use only keyword arguments when emitting.

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

    # TODO: remove args?
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
