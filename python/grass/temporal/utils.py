"""Utility functions and classes for the temporal framework.

(C) 2025 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

:authors: GRASS Development Team
"""

import json
from datetime import datetime


class TemporalJSONEncoder(json.JSONEncoder):
    """Custom JSON encoder for GRASS temporal framework data types.

    Handles serialization of datetime objects to ISO 8601 format strings.
    Can be reused across temporal modules and potentially other GRASS modules.

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
            return obj.isoformat()
        return super().default(obj)
