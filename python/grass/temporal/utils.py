"""Utility functions and classes for the temporal framework.

(C) 2026 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

:authors: GRASS Development Team
"""

import json
from datetime import datetime


class TemporalJSONEncoder(json.JSONEncoder):
    """Custom JSON encoder with datetime support for GRASS GIS.

    Handles serialization of datetime objects to ISO 8601 format strings.
    Can be used across GRASS modules that need JSON output with temporal data.

    Example:

    .. code-block:: pycon

        >>> import json
        >>> from datetime import datetime
        >>> data = {"timestamp": datetime(2024, 1, 15, 10, 30)}
        >>> json.dumps(data, cls=TemporalJSONEncoder)
        '{"timestamp": "2024-01-15T10:30:00"}'
    """

    def default(self, obj):
        """Override default JSON encoding for datetime objects.

        :param obj: Object to encode
        :return: ISO 8601 formatted string for datetime objects,
                 or calls parent encoder for other types
        """
        if isinstance(obj, datetime):
            return obj.strftime("%Y-%m-%d %H:%M:%S")
        return super().default(obj)
