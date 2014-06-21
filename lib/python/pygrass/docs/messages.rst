PyGRASS message interface
=========================

The PyGRASS message interface is a fast and exit-safe
interface to the `GRASS C-library message functions <http://grass.osgeo.org/programming7/gis_2error_8c.html>`_.

.. autoclass:: pygrass.messages.Messenger
    :members:

.. autoclass:: pygrass.messages.FatalError
    :members:

.. autofunction:: pygrass.messages.message_server

.. autofunction:: pygrass.messages.get_msgr
