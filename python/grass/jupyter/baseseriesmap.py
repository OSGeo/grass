import os
import tempfile
import weakref

import grass.script as gs
from grass.grassdb.data import map_exists

from .map import Map
from .region import RegionManagerForSeries
from .utils import save_gif

class BaseSeriesMap:
    def __init__(
        self, 
        width=None, 
        height=None, 
        env=None, 
        use_region=False, 
        saved_region=None
    ):
        
        #Copy Environment
        if env:
            self._env = env.copy()
        else:
            self._env = os.environ.copy()
        
        self._width = width
        self._height = height
        
        # Create a temporary directory for our PNG images
        # Resource managed by weakref.finalize.
        self._tmpdir = (
            # pylint: disable=consider-using-with
            tempfile.TemporaryDirectory()
        )
        
        def cleanup(tmpdir):
            tmpdir.cleanup()
        
        weakref.finalize(self, cleanup, self._tmpdir)
        
        #Handle regions in respective classes

    def _render_baselayers(self, img):
        """Add collected baselayers to Map instance"""
        for grass_module, kwargs in self._base_layer_calls:
            img.run(grass_module, **kwargs)



