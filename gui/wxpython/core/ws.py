"""
@package core.ws

@brief Fetching and preparation of web service data for rendering.

Note: Currently only WMS is implemented.

Classes:
 - ws::RenderWMSMgr
 - ws::GDALRasterMerger

(C) 2012-2015 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Stepan Turek <stepan.turek seznam.cz> (mentor: Martin Landa)
"""
import sys
import copy
import time
import six

import wx

from grass.script.utils import try_remove
from grass.script import core as grass
from grass.exceptions import CalledModuleError

from core.debug import Debug
from core.gthread import gThread

try:
    haveGdal = True
    from osgeo import gdal
except ImportError:
    haveGdal = False

from grass.pydispatch.signal import Signal


class RenderWMSMgr(wx.EvtHandler):
    """Fetch and prepare WMS data for rendering."""

    def __init__(self, layer, env):
        if not haveGdal:
            sys.stderr.write(
                _(
                    "Unable to load GDAL Python bindings.\n"
                    "WMS layers can not be displayed without the bindings.\n"
                )
            )

        self.layer = layer

        wx.EvtHandler.__init__(self)

        # thread for d.wms commands
        self.thread = gThread()

        self._startTime = None
        self.downloading = False
        self.renderedRegion = None
        self.updateMap = True
        self.fetched_data_cmd = None

        self.tempMap = grass.tempfile()
        self.dstSize = {}

        self.dataFetched = Signal("RenderWMSMgr.dataFetched")
        self.updateProgress = Signal("RenderWMSMgr.updateProgress")
        self.renderingFailed = Signal("RenderWMSMgr.renderingFailed")

    def __del__(self):
        try_remove(self.tempMap)

    def Render(self, cmd, env):
        """If it is needed, download missing WMS data.

        .. todo::
            lmgr deletes mapfile and maskfile when order of layers
            was changed (drag and drop) - if deleted, fetch data again
        """
        if not haveGdal:
            return

        Debug.msg(
            1,
            "RenderWMSMgr.Render(%s): force=%d img=%s"
            % (self.layer, self.layer.forceRender, self.layer.mapfile),
        )

        env = copy.copy(env)
        self.dstSize["cols"] = int(env["GRASS_RENDER_WIDTH"])
        self.dstSize["rows"] = int(env["GRASS_RENDER_HEIGHT"])

        region = self._getRegionDict(env)
        self._fitAspect(region, self.dstSize)

        self.updateMap = True
        fetchData = True  # changed to True when calling Render()
        zoomChanged = False

        if self.renderedRegion is None or cmd != self.fetched_data_cmd:
            fetchData = True
        else:
            for c in ["north", "south", "east", "west"]:
                if self.renderedRegion and region[c] != self.renderedRegion[c]:
                    fetchData = True
                    break

            for c in ["e-w resol", "n-s resol"]:
                if self.renderedRegion and region[c] != self.renderedRegion[c]:
                    zoomChanged = True
                    break

        if fetchData:
            self.fetched_data_cmd = None
            self.renderedRegion = region

            try_remove(self.layer.mapfile)
            try_remove(self.tempMap)

            self.currentPid = self.thread.GetId()
            # self.thread.Terminate()
            self.downloading = True

            self.fetching_cmd = cmd

            env["GRASS_RENDER_FILE"] = self.tempMap
            env["GRASS_REGION"] = self._createRegionStr(region)

            cmd_render = copy.deepcopy(cmd)
            cmd_render[1]["quiet"] = True  # be quiet

            self._startTime = time.time()
            self.thread.Run(
                callable=self._render,
                cmd=cmd_render,
                env=env,
                ondone=self.OnRenderDone,
                userdata={"env": env},
            )
            self.layer.forceRender = False

        self.updateProgress.emit(env=env, layer=self.layer)

    def _render(self, cmd, env):
        try:
            # TODO: use errors=status when working
            grass.run_command(cmd[0], env=env, **cmd[1])
            return 0
        except CalledModuleError as e:
            grass.error(e)
            return 1

    def OnRenderDone(self, event):
        """Fetch data"""
        if event.pid != self.currentPid:
            return
        self.downloading = False
        if not self.updateMap:
            self.updateProgress.emit(env=event.userdata["env"], layer=self.layer)
            self.renderedRegion = None
            self.fetched_data_cmd = None
            return

        self.mapMerger = GDALRasterMerger(
            targetFile=self.layer.mapfile,
            region=self.renderedRegion,
            bandsNum=3,
            gdalDriver="PNM",
            fillValue=0,
        )
        self.mapMerger.AddRasterBands(self.tempMap, {1: 1, 2: 2, 3: 3})
        del self.mapMerger

        add_alpha_channel = True
        mask_fill_value = 0
        if self.fetching_cmd[1]["format"] == "jpeg":
            mask_fill_value = (
                255  # white color, g.pnmcomp doesn't apply mask (alpha channel)
            )
            add_alpha_channel = False

        self.maskMerger = GDALRasterMerger(
            targetFile=self.layer.maskfile,
            region=self.renderedRegion,
            bandsNum=1,
            gdalDriver="PNM",
            fillValue=mask_fill_value,
        )
        if add_alpha_channel:
            # {4 : 1} alpha channel (4) to first and only channel (1) in mask
            self.maskMerger.AddRasterBands(self.tempMap, {4: 1})
        del self.maskMerger

        self.fetched_data_cmd = self.fetching_cmd

        Debug.msg(
            1,
            "RenderWMSMgr.OnRenderDone(%s): ret=%d time=%f"
            % (self.layer, event.ret, time.time() - self._startTime),
        )

        self.dataFetched.emit(env=event.userdata["env"], layer=self.layer)

    def _getRegionDict(self, env):
        """Parse string from GRASS_REGION env variable into dict."""
        region = {}
        parsedRegion = env["GRASS_REGION"].split(";")
        for r in parsedRegion:
            r = r.split(":")
            r[0] = r[0].strip()
            if len(r) < 2:
                continue
            try:
                if r[0] in ["e-w resol3", "n-s resol3", "rows3", "cols3", "depths"]:
                    # ignore 3D region values (causing problems in latlong locations)
                    continue
                if r[0] in ["cols", "rows", "zone", "proj"]:
                    region[r[0]] = int(r[1])
                else:
                    region[r[0]] = float(r[1])
            except ValueError:
                region[r[0]] = r[1]

        return region

    def _createRegionStr(self, region):
        """Create string for GRASS_REGION env variable from  dict created by _getRegionDict."""
        regionStr = ""
        for k, v in six.iteritems(region):
            item = k + ": " + str(v)
            if regionStr:
                regionStr += "; "
            regionStr += item

        return regionStr

    def IsDownloading(self):
        """Is it downloading any data from WMS server?"""
        return self.downloading

    def _fitAspect(self, region, size):
        """Compute region parameters to have direction independent resolution."""
        if region["n-s resol"] > region["e-w resol"]:
            delta = region["n-s resol"] * size["cols"] / 2

            center = (region["east"] - region["west"]) / 2

            region["east"] = center + delta + region["west"]
            region["west"] = center - delta + region["west"]
            region["e-w resol"] = region["n-s resol"]

        else:
            delta = region["e-w resol"] * size["rows"] / 2

            center = (region["north"] - region["south"]) / 2

            region["north"] = center + delta + region["south"]
            region["south"] = center - delta + region["south"]
            region["n-s resol"] = region["e-w resol"]

    def Abort(self):
        """Abort rendering process"""
        Debug.msg(1, "RenderWMSMgr({0}).Abort()".format(self.layer))
        self.thread.Terminate()

        # force rendering layer next time
        self.layer.forceRender = True
        self.updateMap = False
        self.thread.Terminate(False)


class GDALRasterMerger:
    """Merge rasters.

    Based on gdal_merge.py utility.
    """

    def __init__(self, targetFile, region, bandsNum, gdalDriver, fillValue=None):
        """Create raster for merging."""
        self.gdalDrvType = gdalDriver

        nsRes = (region["south"] - region["north"]) / region["rows"]
        ewRes = (region["east"] - region["west"]) / region["cols"]

        self.tGeotransform = [region["west"], ewRes, 0, region["north"], 0, nsRes]

        self.tUlx, self.tUly, self.tLrx, self.tLry = self._getCorners(
            self.tGeotransform, region
        )

        driver = gdal.GetDriverByName(self.gdalDrvType)
        self.tDataset = driver.Create(
            targetFile, region["cols"], region["rows"], bandsNum, gdal.GDT_Byte
        )

        if fillValue is not None:
            # fill raster bands with a constant value
            for iBand in range(1, self.tDataset.RasterCount + 1):
                self.tDataset.GetRasterBand(iBand).Fill(fillValue)

    def AddRasterBands(self, sourceFile, sTBands):
        """Add raster bands from sourceFile into the merging raster."""
        sDataset = gdal.Open(sourceFile, gdal.GA_ReadOnly)
        if sDataset is None:
            return

        sGeotransform = sDataset.GetGeoTransform()

        sSize = {"rows": sDataset.RasterYSize, "cols": sDataset.RasterXSize}

        sUlx, sUly, sLrx, sLry = self._getCorners(sGeotransform, sSize)

        # figure out intersection region
        tIntsctUlx = max(self.tUlx, sUlx)
        tIntsctLrx = min(self.tLrx, sLrx)
        if self.tGeotransform[5] < 0:
            tIntsctUly = min(self.tUly, sUly)
            tIntsctLry = max(self.tLry, sLry)
        else:
            tIntsctUly = max(self.tUly, sUly)
            tIntsctLry = min(self.tLry, sLry)

        # do they even intersect?
        if tIntsctUlx >= tIntsctLrx:
            return
        if self.tGeotransform[5] < 0 and tIntsctUly <= tIntsctLry:
            return
        if self.tGeotransform[5] > 0 and tIntsctUly >= tIntsctLry:
            return

        # compute target window in pixel coordinates.
        tXoff = int((tIntsctUlx - self.tGeotransform[0]) / self.tGeotransform[1] + 0.1)
        tYoff = int((tIntsctUly - self.tGeotransform[3]) / self.tGeotransform[5] + 0.1)
        tXsize = (
            int((tIntsctLrx - self.tGeotransform[0]) / self.tGeotransform[1] + 0.5)
            - tXoff
        )
        tYsize = (
            int((tIntsctLry - self.tGeotransform[3]) / self.tGeotransform[5] + 0.5)
            - tYoff
        )

        if tXsize < 1 or tYsize < 1:
            return

        # Compute source window in pixel coordinates.
        sXoff = int((tIntsctUlx - sGeotransform[0]) / sGeotransform[1])
        sYoff = int((tIntsctUly - sGeotransform[3]) / sGeotransform[5])
        sXsize = int((tIntsctLrx - sGeotransform[0]) / sGeotransform[1] + 0.5) - sXoff
        sYsize = int((tIntsctLry - sGeotransform[3]) / sGeotransform[5] + 0.5) - sYoff

        if sXsize < 1 or sYsize < 1:
            return

        for sBandNnum, tBandNum in six.iteritems(sTBands):
            bandData = sDataset.GetRasterBand(sBandNnum).ReadRaster(
                sXoff, sYoff, sXsize, sYsize, tXsize, tYsize, gdal.GDT_Byte
            )
            self.tDataset.GetRasterBand(tBandNum).WriteRaster(
                tXoff, tYoff, tXsize, tYsize, bandData, tXsize, tYsize, gdal.GDT_Byte
            )

    def _getCorners(self, geoTrans, size):

        ulx = geoTrans[0]
        uly = geoTrans[3]
        lrx = geoTrans[0] + size["cols"] * geoTrans[1]
        lry = geoTrans[3] + size["rows"] * geoTrans[5]

        return ulx, uly, lrx, lry

    def SetGeorefAndProj(self):
        """Set georeference and projection to target file"""
        projection = grass.read_command("g.proj", flags="wf")
        self.tDataset.SetProjection(projection)

        self.tDataset.SetGeoTransform(self.tGeotransform)

    def __del__(self):
        self.tDataset = None
