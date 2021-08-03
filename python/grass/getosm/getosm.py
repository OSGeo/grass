###############################################################################
# Project:  GetOSM <https://github.com/HuidaeCho/getosm>
# Authors:  Huidae Cho
# Since:    July 11, 2021
#
# Copyright (C) 2021 Huidae Cho <https://idea.isnew.info/>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
###############################################################################
"""
This module provides an OpenStreetMap tile downloader.
"""

import sys
import math
import urllib.request


class Tile:
    """
    Provide the referencing data structure for tiles. The key attribute is used
    to find cached raw tile data in OpenStreetMap.cached_tiles, a dictionary of
    keys to CachedTile objects. The x and y attributes store the location of
    the tile in pixels relative to the upper-left corner of the canvas. These
    and z attributes are known when a new tile is first downloaded and its
    location is computed by OpenStreetMap.download(). However, x and y can
    change when the tile's rescaling parameters are precomputed by
    OpenStreetMap.rescale(). One of the rescaling parameters is the delta zoom
    level dz which indicates how many zoom levels the tile should be rescaled
    from its original zoom level z. OpenStreetMap.draw_rescaled() actually
    rescales the tile and stores its image in the rescaled_image attribute.
    Only key and z never change once they are set. Initially, dz is set to 0
    and rescaled_image to None, meaning that the original raw tile is not
    rescaled. Once the tile is rescaled x, y, dz, and rescaled_image are
    updated.
    """
    def __init__(self, key, x, y, z):
        self.key = key
        self.x = x
        self.y = y
        self.z = z
        self.dz = 0
        self.rescaled_image = None


class CachedTile:
    """
    Provide the data structure for cached tiles. Initially, when a tile is
    first downloaded in OpenStreetMap.download_tile(), its raw data is stored
    in the image attribute and the raw flag is set to True. Later, when the GUI
    framework needs to draw the tile on the canvas in OpenStreetMap.draw(), it
    needs to convert the raw data to its own native image object. This
    converted image object is stored back to the image attribute and the raw
    flag is now set to False. There is no way to go back to the original raw
    data.
    """
    def __init__(self, image, raw):
        self.image = image
        self.raw = raw


class OpenStreetMap:
    """
    Provide the public-facing API for downloading, dragging, zooming, and
    coordinate conversions.
    """
    def __init__(self, create_image, draw_image, create_tile, draw_tile,
                 resample_tile, width=256, height=256, lat=0, lon=0, z=0,
                 verbose=False):
        """
        Instantiate a new instance of the OpenStreetMap class.

        Args:
            create_image (function): Function(width, height) for creating a
                new image for the entire map. This function should take the
                width and height of the image. Since GetOSM is an OpenStreetMap
                downloader and drawing helper, it does not draw anything by
                itself. Instead, it takes drawing functions that are specific
                to a GUI framework as callback functions, and calls them. In
                this way, tile downloading can take place in a separate thread
                from the main GUI thread, and GetOSM does not have to worry
                about how to draw the map.
            draw_image (function): Function(image) for actually drawing the
                created map image onto the canvas. It should take the output
                image returned by the create_image() function.
            create_tile (function): Function(data) for creating a tile image
                using raw data stored in self.cached_tiles[].image with its raw
                flag set to True (see the CachedTile clsas). It should take the
                data argument that is raw from the OpenStreetMap server.
            draw_tile (function): Function(image, tile, x, y) for patching a
                tile on the image at x and y. This function should not crop the
                tile image and should only place it at the given location. It
                should take the image from create_image(), a tile from
                create_tile(), and the x and y of the tile (see the Tile
                class).
            resample_tile (function): Function(tile, dz) for rescaling a tile
                image by resampling its pixel values. It should scale up (zoom
                in) or down (zoom out) if dz is posiive or negative,
                respectively. The dz argument is always an int which can be
                converted to a float scaling factor of 2**dz. This funciton
                should take a tile from create_tile() stored in
                self.cached_tiles[].image with the raw flag set to False (see
                the CachedTile class) and a delta zoom level dz (see the Tile
                class).
            width (int): Canvas width in pixels. Defaults to 256.
            height (int): Canvas height in pixels. Defaults to 256.
            lat (float): Center latitude in decimal degrees. Defaults to 0.
            lon (float): Center longitude in decimal degrees. Defaults to 0.
            z (int): Zoom level. Defaults to 0.
            verbose (bool): Whether or not to print debugging messages.
                Defaults to False.
        """
        self.create_image = create_image
        self.draw_image = draw_image
        self.create_tile = create_tile
        self.draw_tile = draw_tile
        self.resample_tile = resample_tile
        self.width = width
        self.height = height
        self.lat = lat
        self.lon = lon
        self.z = z
        self.verbose = verbose

        self.z_min = 0
        self.z_max = 18
        self.lat_min = -85.0511
        self.lat_max = 85.0511
        self.dz = 0
        self.tiles = []
        self.rescaled_tiles = []
        # TODO: Tile caching mechanism
        self.cached_tiles = {}
        self.cancel = False

        self.redownload()
        self.draw()

    def message(self, *args, end=None):
        """
        Print args to stderr immediately if self.verbose is True.

        Args:
            args (str): Arguments to print. Passed to print().
            end (str): Passed to print(). Defaults to None.
        """
        if self.verbose:
            print(*args, end=end, file=sys.stderr, flush=True)

    def resize(self, width, height):
        """
        Resize the canvas and redownload tiles at the same location and zoom
        level.

        Args:
            width (int): New canvas width in pixels.
            height (int): New canvas height in pixels.
        """
        if width != self.width or height != self.height:
            self.width = width
            self.height = height
            self.max_cached_tiles = int(2 * (width / 256) * (height / 256))
            self.redownload()

    # Adapted from https://stackoverflow.com/a/62607111/16079666
    # https://wiki.openstreetmap.org/wiki/Slippy_map_tilenames
    # x from 0 at lon=-180 to 2**z-1 at lon=180
    # y from 0 at lat=85.0511 to 2**z-1 at lat=-85.0511
    # tilex, tiley = int(x), int(y)
    def latlon_to_tile(self, lat, lon, z):
        """
        Convert latitude and lonngitude to tile x and y at the zoom level z.
        Tile x and y are not ints, but they are floats to be able to tell
        locations within a tile. To convert them to x and y for downloading the
        tile, type cast them to int. These int x and y would correspond to the
        upper-left cornder of the tile. Both x and y increase by 1 when we move
        from one tile to the next. In other words, x and y are not pixel
        coordinates, but they are rather the number or fractional number of
        tiles starting from latitude 85.0511 and longitude -180 at the given
        zoom level.

        Args:
            lat (float): Latitude in decimal degrees.
            lon (float): Longitude in decimal degrees.
            z (int): Zoom level.

        Returns:
            float, float: Tile x, y starting from lon,lon=-180,85.0511 growing
            towards the east and south by 1 for each tile.
        """
        lat = min(max(lat, self.lat_min), self.lat_max)
        lat = math.radians(lat)
        n = 2**z
        x = (lon+180)/360*n
        y = (1-math.log(math.tan(lat)+(1/math.cos(lat)))/math.pi)/2*n
        return x, y

    def tile_to_latlon(self, x, y, z):
        """
        Convert tile x,y to latitude and longitude at the zoom level z. Tile x
        and y don't have to be ints (see latlon_to_tile()) and are not pixel
        coordinates. The are the number or fractional number of tiles starting
        from latitude 85.0511 and longitude -180 at the given zoom level.

        Args:
            x (float): Tile x starting from lon=-180 growing towards the east
                by 1 for every tile.
            y (float): Tile y starting from lat=85.0511 growing towards the
                south by 1 for each tile.
            z (int): Zoom level.

        Returns:
            float, float: Latitude and longitude in decimal degrees.
        """
        n = 2**z
        lat = math.degrees(math.atan(math.sinh(math.pi*(1-2*y/n))))
        lon = x/n*360-180
        return lat, lon

    def latlon_to_canvas(self, lat, lon):
        """
        Convert latitude and lonngitude to canvas x and y in pixels in the
        current map centered at self.lat,self.lon at the zoom level self.z.
        These x and y are pixel coordinates on the canvas starting from the
        upper-left corner at 0,0. For now, these are floats to keep their
        precision, but it can change in the future.

        Args:
            lat (float): Latitude in decimal degrees.
            lon (float): Longitude in decimal degrees.

        Returns:
            float, float: Canvas x and y in pixels starting from the upper-left
            corner at x,y=0,0 growing towards the right and bottom by 1 for
            each pixel.
        """
        x, y = self.latlon_to_tile(lat, lon, self.z)
        x = self.xoff + (x - self.x) * 256
        y = self.yoff + (y - self.y) * 256
        return x, y

    def canvas_to_latlon(self, x, y):
        """
        Convert canvas x and y in pixels to latitude and longitude in decimal
        degrees in the current map centered at self.lat,self.lon at the zoom
        level self.z. Input x and y don't need to be ints (see
        latlon_to_canvas()). They start from the upper-left corner at 0,0 on
        the canvas.

        Args:
            x (float): Canvas x starting from the left at x=0 growing towards
                the right by 1 for each pixel.
            y (float): Canvas y starting from the top at y=0 growing towards
                the bottom by 1 for each pixel.

        Returns:
            float, float: Latitude and longitude in decimal degrees.
        """
        x = self.x + (x - self.xoff) / 256
        y = self.y + (y - self.yoff) / 256
        lat, lon = self.tile_to_latlon(x, y, self.z)
        while lon < -180:
            lon += 360
        while lon > 180:
            lon -= 360
        return lat, lon

    def get_tile_url(self, x, y, z):
        """
        Get the URL for the tile at tile x,y (see latlon_to_tile()) at the zoom
        level z. These x and y must be ints because they are used to construct
        a URL for tile downloading.

        Args:
            x (int): Tile x starting from lon=-180 growing towards the east by
                1 for every tile.
            y (int): Tile y starting from lat=85.0511 growing towards the south
                by 1 for each tile.
            z (int): Zoom level.

        Returns:
            str: URL for the tile at tile x,y at the zoom level z.
        """
        return f"http://a.tile.openstreetmap.org/{z}/{x}/{y}.png"

    def download_tile(self, x, y, z):
        """
        Download the tile at tile x,y (see latlon_to_tile()) at the zoom level
        z and return its key in z/x/y. These x and y must be ints because they
        are used to construct a URL for tile downloading. The raw data of the
        tile is stored in self.cached_tiles with its key z/x/y (see the
        CachedTile class). The function returns the tile key.

        Args:
            x (int): Tile x starting from lon=-180 growing towards the east by
                1 for every tile.
            y (int): Tile y starting from lat=85.0511 growing towards the south
                by 1 for each tile.
            z (int): Zoom level.

        Returns:
            str: URL for the tile at tile x,y at the zoom level z.
        """
        tile_url = self.get_tile_url(x, y, z)
        tile_key = f"{z}/{x}/{y}"
        if tile_key not in self.cached_tiles:
            # need this header to successfully download tiles from the server
            req = urllib.request.Request(tile_url, headers={
                "User-Agent": "urllib.request"
            })
            try:
                with urllib.request.urlopen(req) as f:
                    self.cached_tiles[tile_key] = CachedTile(f.read(), True)
                    self.message(f"{tile_url} downloaded")
            except Exception as e:
                self.message(f"{tile_url}: Failed to download")
        return tile_key

    def download(self, lat, lon, z):
        """
        Download all tiles needed to cover the entire canvas centered at
        latitude and lonitude at the zoom level z. Downloaded tiles are saved
        in self.cached_tiles (see download_tile()). The function returns
        whether or not the download session was canceled by the main thread by
        setting self.cancel to True.

        Args:
            lat (float): Latitude in decimal degrees.
            lon (float): Longitude in decimal degrees.
            z (int): Zoom level.

        Returns:
            bool: Whether or not the download session was canceled externally
            by the main thread by setting the cancel attribute to True. It's
            the responsibility of the main thread to reset the cancel attribute
            to False. When the user scrolls the mouse wheel continuously to
            zoom faster, the main thread needs to cancel any previous download
            sessions to save data traffic and CPU time.
        """
        z = min(max(z, self.z_min), self.z_max)
        ntiles = 2**z

        # calculate x,y offsets to lat,lon within width,height
        xc, yc = self.latlon_to_tile(lat, lon, z)
        x, y = int(xc), int(yc)
        n, w = self.tile_to_latlon(x, y, z)
        s, e = self.tile_to_latlon(x + 1, y + 1, z)
        xo, yo = self.latlon_to_tile(n, w, z)

        xoff = int(self.width / 2 - (xc - xo) * 256)
        yoff = int(self.height / 2 - (yc - yo) * 256)

        xmin = x - math.ceil(xoff / 256)
        ymin = max(y - math.ceil(yoff / 256), 0)
        xmax = x + math.ceil((self.width - xoff - 256) / 256)
        ymax = min(y + math.ceil((self.height - yoff - 256) / 256), ntiles - 1)

        self.lat = lat
        self.lon = lon
        self.x = x
        self.y = y
        self.z = z
        self.ntiles = ntiles
        self.xmin = xmin
        self.xmax = xmax
        self.ymin = ymin
        self.ymax = ymax
        self.xoff = xoff
        self.yoff = yoff

        self.message("image size:", self.width, self.height)
        self.tiles.clear()

        for xi in range(xmin, xmax + 1):
            xt = xi % ntiles
            for yi in range(ymin, ymax + 1):
                if self.cancel:
                    self.message("download_map canceled")
                    break
                tile_url = self.get_tile_url(xt, yi, z)
                tile_key = self.download_tile(xt, yi, z)
                tile_x = xoff + (xi - x) * 256
                while tile_x <= -256:
                    tile_x += 256 * ntiles
                while tile_x > self.width:
                    tile_x -= 256 * ntiles
                tile_y = yoff + (yi - y) * 256
                self.tiles.append(Tile(tile_key, tile_x, tile_y, z))
            if self.cancel:
                break
        return not self.cancel

    def redownload(self):
        """
        Provide a shortcut for self.download(self.lat, self.lon, self.z). This
        function can be used to redownload already downloaded tiles as its name
        suggests or to download tiles for the first time when self.lat,
        self.lon, and self.z are already set.
        """
        return self.download(self.lat, self.lon, self.z)

    def draw(self):
        """
        Draw cached tiles stored in tiles on the canvas by calling callback
        functions including self.create_image(), self.create_tile(),
        self.draw_tile(), and self.draw_image() (see the constructor). This
        function converts raw tile data to the GUI framework's native image
        format and sets its raw flag to False.
        """
        image = self.create_image(self.width, self.height)
        for tile in self.tiles:
            if tile.key in self.cached_tiles:
                cached_tile = self.cached_tiles[tile.key]
                if cached_tile.raw:
                    cached_tile.image = self.create_tile(cached_tile.image)
                    cached_tile.raw = False
                self.draw_tile(image, cached_tile.image, tile.x, tile.y)
        self.draw_image(image)
        self.rescaled_tiles = self.tiles.copy()

    def draw_rescaled(self):
        """
        Draw rescaled tiles stored in self.rescaled_tiles on the canvas by
        calling callback functions including self.create_image(),
        self.create_tile(), self.draw_tile(), self.resample_tile(), and
        self.draw_image() (see the constructor). This function may convert raw
        tile data to the GUI framework's native image format using
        self.create_tile() and sets its raw flag to False.
        """
        image = self.create_image(self.width, self.height)
        for tile in self.rescaled_tiles:
            if tile.rescaled_image:
                tile_image = tile.rescaled_image
            else:
                cached_tile = self.cached_tiles[tile.key]
                if cached_tile.raw:
                    cached_tile.image = self.create_tile(cached_tile.image)
                    cached_tile.raw = False
                tile_image = cached_tile.image

            tile.rescaled_image = self.resample_tile(tile_image, tile.dz)
            self.draw_tile(image, tile.rescaled_image, tile.x, tile.y)
        self.draw_image(image)

    def grab(self, x, y):
        """
        Set self.grab_x and self.grab_y to x and y, respectively. This function
        is used to signal the start of dragging events. self.drag() uses these
        two attributes to calculate the x and y deltas of a dragging event.
        Both x and y are mostly ints because they are canvas coordinates in
        pixels, but they can also be floats.

        Args:
            x (float): Canvas x starting from the left at x=0 growing towards
                the right by 1 for each pixel.
            y (float): Canvas y starting from the top at y=0 growing towards
                the bottom by 1 for each pixel.
        """
        self.grab_x = x
        self.grab_y = y

    def drag(self, x, y, draw=True):
        """
        Drag the map by x-self.drag_x and y-self.drag_y, and download necessary
        tiles using self.download(). The location at x,y follows the mouse
        cursor. By default, it draws the map using self.draw(). However, if
        draw is False, it only downloads tiles and doesn't draw the map. It
        returns x-self.drag_x and y-self.drag_y.

        Args:
            x (float): Canvas x starting from the left at x=0 growing towards
                the right by 1 for each pixel.
            y (float): Canvas y starting from the top at y=0 growing towards
                the bottom by 1 for each pixel.
            draw (bool): Whether or not to draw the map. Defaults to True.

        Returns:
            float, float: Drag amounts in x and y in pixels.
        """
        dx = x - self.grab_x
        dy = y - self.grab_y
        self.grab(x, y)
        x = self.width / 2 - dx
        y = self.height / 2 - dy
        lat, lon = self.canvas_to_latlon(x, y)
        old_lat = self.lat
        self.download(lat, lon, self.z)
        if abs(old_lat - self.lat) <= sys.float_info.epsilon:
            dy = 0
        if draw:
            self.draw()
        return dx, dy

    def reset_zoom(self):
        """
        Reset the delta zoom level self.dz to restart a zooming event.
        """
        self.dz = 0

    def zoom(self, x, y, dz, draw=True):
        """
        Zoom the map by the delta zoom level dz relative to x,y and download
        necessary tiles using self.download() if the map is zoomed. The delta
        zoom level dz is a float that is accumulated in self.dz. Once self.dz
        reaches 1 or -1, zooming starts. The location at x,y stays at the same
        location x,y. By default, it draws the map using self.draw(). However,
        if draw is False, it only downloads tiles and doesn't draw the map. It
        returns whether or not the map was zoomed.

        Args:
            x (float): Canvas x starting from the left at x=0 growing towards
                the right by 1 for each pixel.
            y (float): Canvas y starting from the top at y=0 growing towards
                the bottom by 1 for each pixel.
            dz (float): Delta zoom level.
            draw (bool): Whether or not to draw the map. Defaults to True.

        Returns:
            bool: Whether or now the map was zoomed.
        """
        zoomed = False
        self.dz += dz
        if ((self.z < self.z_max and self.dz >= 1) or
            (self.z > self.z_min and self.dz <= -1)):
            dz = int(self.dz)
            self.message("rescale:", self.z, dz)
            z = self.z + dz

            # pinned zoom at x,y
            xc, yc = self.width / 2, self.height / 2
            for i in range(0, abs(dz)):
                if dz > 0:
                    # each zoom-in doubles
                    xc = (x + xc) / 2
                    yc = (y + yc) / 2
                else:
                    # each zoom-out halves
                    xc = 2 * xc - x
                    yc = 2 * yc - y

            # lat,lon at xc,yc
            lat, lon = self.canvas_to_latlon(xc, yc)
            zoomed = True
        elif ((self.z == self.z_max and self.dz >= 1) or
              (self.z == self.z_min and self.dz <= -1)):
            # need to download map for z_max or z_min because when the first
            # event of either zoom level was canceled, there are no cached
            # tiles
            lat, lon, z = self.lat, self.lon, self.z
            zoomed = True

        if zoomed:
            self.download(lat, lon, z)
            self.reset_zoom()
            if draw:
                self.draw()
        return zoomed

    def zoom_to_bbox(self, bbox, draw=True):
        """
        Zoom the map to the given bounding box bbox in south, north, west, and
        east in decimal degrees. South must be less than north, but west may
        not be less than east if the end point of the bbox is located to the
        west of the start point. In this reversed west and east case, the bbox
        becomes the NOT of the bbox that would be formed if west and east were
        switched. By default, it draws the map using self.draw(). However, if
        draw is False, it only downloads tiles and doesn't draw the map. In
        either case, it does not draw the bbox. It returns whether or not the
        map was zoomed. The function returns south, north, and west as is, and
        west plus the delta longitude as east so that east is always greater
        than west. In this case, east can be greater than 180.

        Args:
            bbox (list): List of south, north, west, and east floats in decimal
                degrees.
            draw (bool): Whether or not to draw the map. Defaults to True.

        Returns:
            float, float, float, float: South, north, west, and west plus the
            delta longitude in decimal degrees.
        """
        s, n, w, e = bbox

        lat = (s + n) / 2

        if w == e or (w == -180 and e == 180):
            dlon = 360
            lon = e - 180 if e >= 0 else e
        elif w < e:
            dlon = e - w
            lon = (w + e) / 2
        else:
            dlon = 360 - w + e
            lon = (w + e) / 2
            lon = lon - 180 if lon >= 0 else lon + 180
        e = w + dlon

        # find the center
        l, t = self.latlon_to_tile(n, w, self.z)
        r, b = self.latlon_to_tile(s, e, self.z)
        lat, lon = self.tile_to_latlon((l + r) / 2, (t + b) / 2, self.z)

        # estimate z
        z_lat = math.log2((self.lat_max - self.lat_min) / (n - s))
        z_lon = math.log2(360 / dlon)
        z = math.ceil(min(z_lat, z_lon))

        # check if z covers the entire bbox
        l, t = self.latlon_to_tile(n, w, z)
        r, b = self.latlon_to_tile(s, e, z)
        width = (r - l) * 256
        height = (b - t) * 256

        if 2 * width <= self.width and 2 * height <= self.height:
            # if z is too loose, tighten it
            z += 1
        elif width > self.width or height > self.height:
            # if z is too tight, loosen it
            z -= 1

        self.download(lat, lon, z)
        if draw:
            self.draw()

        return s, n, w, e

    def rescale(self, x, y, dz, draw=True):
        """
        Rescale the map by the delta zoom level dz relative to x,y and download
        necessary tiles using self.download() if the map is rescaled. The delta
        zoom level dz is a float that is accumulated in self.dz. Once self.dz
        reaches 1 or -1, rescaling starts. The location at x,y stays at the
        same location x,y. By default, it draws the map using self.draw().
        However, if draw is False, it only computes necessary parameters and
        downloads tiles without drawing the map. In this case,
        self.draw_rescaled() needs to be called to actually rescale and draw
        the tiles. It returns whether or not the map was rescaled.

        Args:
            x (float): Canvas x starting from the left at x=0 growing towards
                the right by 1 for each pixel.
            y (float): Canvas y starting from the top at y=0 growing towards
                the bottom by 1 for each pixel.
            dz (float): Delta zoom level.
            draw (bool): Whether or not to draw the map. Defaults to True.

        Returns:
            bool: Whether or now the map was rescaled.
        """
        rescaled = False
        self.dz += dz
        if ((self.z < self.z_max and self.dz >= 1) or
            (self.z > self.z_min and self.dz <= -1)):
            dz = int(self.dz)
            self.message("rescale:", self.z, dz)
            z = self.z + dz

            # pinned rescale at x,y
            xc, yc = self.width / 2, self.height / 2
            for i in range(0, abs(dz)):
                if dz > 0:
                    # each zoom-in doubles
                    xc = (x + xc) / 2
                    yc = (y + yc) / 2
                else:
                    # each zoom-out halves
                    xc = 2 * xc - x
                    yc = 2 * yc - y

            for i in reversed(range(len(self.rescaled_tiles))):
                tile = self.rescaled_tiles[i]
                if tile.key not in self.cached_tiles:
                    del self.rescaled_tiles[i]
                    continue

                tile.x = self.width / 2 - 2**dz * (xc - tile.x)
                tile.y = self.height / 2 - 2**dz * (yc - tile.y)
                tile.dz = dz
                tile_size = 2**(z - tile.z) * 256

                if (tile.x + tile_size < 0 or tile.y + tile_size < 0 or
                    tile.x >= self.width or tile.y >= self.height or
                    # rescaling too fast can raise a memory exception:
                    # _tkinter.TclError: not enough free memory for image
                    # buffer; avoid rescaling more than 2**6 = 64 times; this
                    # number is experimental
                    z > tile.z + 6):
                    del self.rescaled_tiles[i]

            lat, lon = self.canvas_to_latlon(xc, yc)
            xt, yt = self.latlon_to_tile(lat, lon, z)
            xi, yi = int(xt), int(yt)
            n, w = self.tile_to_latlon(xi, yi, z)
            s, e = self.tile_to_latlon(xi + 1, yi + 1, z)
            xo, yo = self.latlon_to_tile(n, w, z)

            self.lat = lat
            self.lon = lon
            self.x = xi
            self.y = yi
            self.z = z
            self.xoff = int(self.width / 2 - (xt - xo) * 256)
            self.yoff = int(self.height / 2 - (yt - yo) * 256)

            rescaled = True

        if rescaled:
            self.reset_zoom()
            if draw:
                self.draw_rescaled()
        return rescaled

    def repeat_xy(self, xy):
        """
        Repeat canvas xy points in pixels across the antimeridian. Unlike the
        latitude axis, the longitude axis crosses the antimeridian from west to
        east or from east to west. When the map repeats itself more than once
        horizontally, geometries also need to be repeated. This function is
        used to repeat xy points just enough times to cover the entire canvas.
        The xy points must be in [[x, y], [x, y], ...]. The return list is in
        [[[x, y], [x, y], ...], [[x, y], [x, y], ...], ...].
        same list format.

        Args:
            xy (list): List of lists of canvas x and y in pixels.

        Returns:
            list: List of lists of lists of canvas x and y in pixels.
        """
        outxy = []
        n = self.width // (256 * self.ntiles)
        for i in range(-n // 2 - 1, n // 2 + 2):
            dx = i * 256 * self.ntiles
            p = []
            for coor in xy:
                x, y = coor
                x += dx
                p.append([x, y])
            outxy.append(p)
        return outxy

    def get_xy(self, latlon):
        """
        Converts a list of lists of latitude and longitude in decimal degrees
        to a list of lists of canvas x and y in pixels. The input latlon must
        be in [[lat, lon], [lat, lon], ...].

        Args:
            latlon (list): List of lists of latitude and longitude in decimal
                degrees.

        Returns:
            list: List of lists of canvas x and y floats (see
            latlon_to_canvas()) in pixels.
        """
        outxy = []
        if latlon:
            xy = []
            for coor in latlon:
                xy.append(self.latlon_to_canvas(*coor))
            outxy.extend(self.repeat_xy(xy))
        return outxy

    def get_bbox_xy(self, bbox):
        """
        Converts a list of south, north, west, and east in decimal degrees to a
        list of canvas upper-left and lower-right corner points in pixels. The
        output is in [[left, top], [right, bottom]].

        Args:
            bbox (list): List of south, north, west, and east in decimal
                degrees.

        Returns:
            list: List of canvas upper-left and lower-right corner points in
            pixels in [[left, top], [right, bottom]].
        """
        outxy = []
        if bbox:
            s, n, w, e = bbox
            l, t = self.latlon_to_canvas(n, w)
            r, b = self.latlon_to_canvas(s, e)
            if w > e:
                l -= 256 * self.ntiles
            xy = [[l, t], [r, b]]
            outxy.extend(self.repeat_xy(xy))
        return outxy
