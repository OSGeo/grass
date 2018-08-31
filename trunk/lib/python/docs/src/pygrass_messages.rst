PyGRASS message interface
=========================

The PyGRASS message interface is a fast and exit-safe interface to the
`GRASS C-library message functions
<http://grass.osgeo.org/programming7/gis_2error_8c.html>`_.

The :class:`~pygrass.messages.Messenger` class implements a fast and
exit-safe interface to the GRASS C-library message functions like:
``G_message()``, ``G_warning()``, ``G_important_message()``,
``G_verbose_message()``, ``G_percent()`` and ``G_debug()``.

Usage:

    >>> msgr = Messenger()
    >>> msgr.debug(0, "debug 0")
    >>> msgr.verbose("verbose message")
    >>> msgr.message("message")
    >>> msgr.important("important message")
    >>> msgr.percent(1, 1, 1)
    >>> msgr.warning("Ohh")
    >>> msgr.error("Ohh no")

    >>> msgr = Messenger()
    >>> msgr.fatal("Ohh no no no!")
    Traceback (most recent call last):
      File "__init__.py", line 239, in fatal
        sys.exit(1)
    SystemExit: 1

    >>> msgr = Messenger(raise_on_error=True)
    >>> msgr.fatal("Ohh no no no!")
    Traceback (most recent call last):
      File "__init__.py", line 241, in fatal
        raise FatalError(message)
    FatalError: Ohh no no no!

    >>> msgr = Messenger(raise_on_error=True)
    >>> msgr.set_raise_on_error(False)
    >>> msgr.fatal("Ohh no no no!")
    Traceback (most recent call last):
      File "__init__.py", line 239, in fatal
        sys.exit(1)
    SystemExit: 1

    >>> msgr = Messenger(raise_on_error=False)
    >>> msgr.set_raise_on_error(True)
    >>> msgr.fatal("Ohh no no no!")
    Traceback (most recent call last):
      File "__init__.py", line 241, in fatal
        raise FatalError(message)
    FatalError: Ohh no no no!
