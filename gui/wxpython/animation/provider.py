"""
@package animation.provider

@brief Animation files and bitmaps management

Classes:
 - mapwindow::BitmapProvider
 - mapwindow::BitmapRenderer
 - mapwindow::BitmapComposer
 - mapwindow::DictRefCounter
 - mapwindow::MapFilesPool
 - mapwindow::BitmapPool
 - mapwindow::CleanUp

(C) 2013 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Anna Petrasova <kratochanna gmail.com>
"""
import os
import sys
import wx
import tempfile
from multiprocessing import Process, Queue

from core.gcmd import GException, DecodeString
from core.settings import UserSettings
from core.debug import Debug
from core.utils import autoCropImageFromFile

from animation.utils import HashCmd, HashCmds, GetFileFromCmd, GetFileFromCmds
from gui_core.wrap import EmptyBitmap, BitmapFromImage

import grass.script.core as gcore
from grass.script.task import cmdlist_to_tuple
from grass.pydispatch.signal import Signal


class BitmapProvider:
    """Class for management of image files and bitmaps.

    There is one instance of this class in the application.
    It handles both 2D and 3D animations.
    """

    def __init__(
        self, bitmapPool, mapFilesPool, tempDir, imageWidth=640, imageHeight=480
    ):

        self._bitmapPool = bitmapPool
        self._mapFilesPool = mapFilesPool
        self.imageWidth = (
            imageWidth  # width of the image to render with d.rast or d.vect
        )
        # height of the image to render with d.rast or d.vect
        self.imageHeight = imageHeight
        self._tempDir = tempDir

        self._uniqueCmds = []
        self._cmdsForComposition = []
        self._opacities = []

        self._cmds3D = []
        self._regionFor3D = None
        self._regions = []
        self._regionsForUniqueCmds = []

        self._renderer = BitmapRenderer(
            self._mapFilesPool, self._tempDir, self.imageWidth, self.imageHeight
        )
        self._composer = BitmapComposer(
            self._tempDir,
            self._mapFilesPool,
            self._bitmapPool,
            self.imageWidth,
            self.imageHeight,
        )
        self.renderingStarted = Signal("BitmapProvider.renderingStarted")
        self.compositionStarted = Signal("BitmapProvider.compositionStarted")
        self.renderingContinues = Signal("BitmapProvider.renderingContinues")
        self.compositionContinues = Signal("BitmapProvider.compositionContinues")
        self.renderingFinished = Signal("BitmapProvider.renderingFinished")
        self.compositionFinished = Signal("BitmapProvider.compositionFinished")
        self.mapsLoaded = Signal("BitmapProvider.mapsLoaded")

        self._renderer.renderingContinues.connect(self.renderingContinues)
        self._composer.compositionContinues.connect(self.compositionContinues)

    def SetCmds(self, cmdsForComposition, opacities, regions=None):
        """Sets commands to be rendered with opacity levels.
        Applies to 2D mode.

        :param cmdsForComposition: list of lists of command lists
                                   [[['d.rast', 'map=elev_2001'], ['d.vect', 'map=points']], # g.pnmcomp
                                   [['d.rast', 'map=elev_2002'], ['d.vect', 'map=points']],
                                   ...]
        :param opacities: list of opacity values
        :param regions: list of regions
        """
        Debug.msg(
            2, "BitmapProvider.SetCmds: {n} lists".format(n=len(cmdsForComposition))
        )
        self._cmdsForComposition.extend(cmdsForComposition)
        self._opacities.extend(opacities)
        self._regions.extend(regions)

        self._getUniqueCmds()

    def SetCmds3D(self, cmds, region):
        """Sets commands for 3D rendering.

        :param cmds: list of commands m.nviz.image (cmd as a list)
        :param region: for 3D rendering
        """
        Debug.msg(2, "BitmapProvider.SetCmds3D: {c} commands".format(c=len(cmds)))
        self._cmds3D = cmds
        self._regionFor3D = region

    def _getUniqueCmds(self):
        """Returns list of unique commands.
        Takes into account the region assigned."""
        unique = list()
        for cmdList, region in zip(self._cmdsForComposition, self._regions):
            for cmd in cmdList:
                if region:
                    unique.append((tuple(cmd), tuple(sorted(region.items()))))
                else:
                    unique.append((tuple(cmd), None))
        unique = list(set(unique))
        self._uniqueCmds = [cmdAndRegion[0] for cmdAndRegion in unique]
        self._regionsForUniqueCmds.extend(
            [
                dict(cmdAndRegion[1]) if cmdAndRegion[1] else None
                for cmdAndRegion in unique
            ]
        )

    def Unload(self):
        """Unloads currently loaded data.
        Needs to be called before setting new data.
        """
        Debug.msg(2, "BitmapProvider.Unload")
        if self._cmdsForComposition:
            for cmd, region in zip(self._uniqueCmds, self._regionsForUniqueCmds):
                del self._mapFilesPool[HashCmd(cmd, region)]

            for cmdList, region in zip(self._cmdsForComposition, self._regions):
                del self._bitmapPool[HashCmds(cmdList, region)]
            self._uniqueCmds = []
            self._cmdsForComposition = []
            self._opacities = []
            self._regions = []
            self._regionsForUniqueCmds = []
        if self._cmds3D:
            self._cmds3D = []
            self._regionFor3D = None

    def _dryRender(self, uniqueCmds, regions, force):
        """Determines how many files will be rendered.

        :param uniqueCmds: list of commands which are to be rendered
        :param force: if forced rerendering
        :param regions: list of regions assigned to the commands
        """
        count = 0
        for cmd, region in zip(uniqueCmds, regions):
            filename = GetFileFromCmd(self._tempDir, cmd, region)
            if (
                not force
                and os.path.exists(filename)
                and self._mapFilesPool.GetSize(HashCmd(cmd, region))
                == (self.imageWidth, self.imageHeight)
            ):
                continue
            count += 1

        Debug.msg(
            3, "BitmapProvider._dryRender: {c} files to be rendered".format(c=count)
        )

        return count

    def _dryCompose(self, cmdLists, regions, force):
        """Determines how many lists of (commands) files
        will be composed (with g.pnmcomp).

        :param cmdLists: list of commands lists which are to be composed
        :param regions: list of regions assigned to the commands
        :param force: if forced rerendering
        """
        count = 0
        for cmdList, region in zip(cmdLists, regions):
            if (
                not force
                and HashCmds(cmdList, region) in self._bitmapPool
                and self._bitmapPool[HashCmds(cmdList, region)].GetSize()
                == (self.imageWidth, self.imageHeight)
            ):
                continue
            count += 1

        Debug.msg(
            2, "BitmapProvider._dryCompose: {c} files to be composed".format(c=count)
        )

        return count

    def Load(self, force=False, bgcolor=(255, 255, 255), nprocs=4):
        """Loads data, both 2D and 3D. In case of 2D, it creates composites,
        even when there is only 1 layer to compose (to be changed for speedup)

        :param force: if True reload all data, otherwise only missing data
        :param bgcolor: background color as a tuple of 3 values 0 to 255
        :param nprocs: number of procs to be used for rendering
        """
        Debug.msg(
            2,
            "BitmapProvider.Load: "
            "force={f}, bgcolor={b}, nprocs={n}".format(f=force, b=bgcolor, n=nprocs),
        )
        cmds = []
        regions = []
        if self._uniqueCmds:
            cmds.extend(self._uniqueCmds)
            regions.extend(self._regionsForUniqueCmds)
        if self._cmds3D:
            cmds.extend(self._cmds3D)
            regions.extend([None] * len(self._cmds3D))

        count = self._dryRender(cmds, regions, force=force)
        self.renderingStarted.emit(count=count)

        # create no data bitmap
        if None not in self._bitmapPool or force:
            self._bitmapPool[None] = createNoDataBitmap(
                self.imageWidth, self.imageHeight
            )

        ok = self._renderer.Render(
            cmds,
            regions,
            regionFor3D=self._regionFor3D,
            bgcolor=bgcolor,
            force=force,
            nprocs=nprocs,
        )
        self.renderingFinished.emit()
        if not ok:
            self.mapsLoaded.emit()  # what to do here?
            return
        if self._cmdsForComposition:
            count = self._dryCompose(
                self._cmdsForComposition, self._regions, force=force
            )
            self.compositionStarted.emit(count=count)
            self._composer.Compose(
                self._cmdsForComposition,
                self._regions,
                self._opacities,
                bgcolor=bgcolor,
                force=force,
                nprocs=nprocs,
            )
            self.compositionFinished.emit()
        if self._cmds3D:
            for cmd in self._cmds3D:
                self._bitmapPool[HashCmds([cmd], None)] = wx.Bitmap(
                    GetFileFromCmd(self._tempDir, cmd, None)
                )

        self.mapsLoaded.emit()

    def RequestStopRendering(self):
        """Requests to stop rendering/composition"""
        Debug.msg(2, "BitmapProvider.RequestStopRendering")
        self._renderer.RequestStopRendering()
        self._composer.RequestStopComposing()

    def GetBitmap(self, dataId):
        """Returns bitmap with given key
        or 'no data' bitmap if no such key exists.

        :param dataId: name of bitmap
        """
        try:
            bitmap = self._bitmapPool[dataId]
        except KeyError:
            bitmap = self._bitmapPool[None]
        return bitmap

    def WindowSizeChanged(self, width, height):
        """Sets size when size of related window changes."""
        Debug.msg(
            5,
            "BitmapProvider.WindowSizeChanged: w={w}, h={h}".format(w=width, h=height),
        )

        self.imageWidth, self.imageHeight = width, height

        self._composer.imageWidth = self._renderer.imageWidth = width
        self._composer.imageHeight = self._renderer.imageHeight = height

    def LoadOverlay(self, cmd):
        """Creates raster legend with d.legend

        :param cmd: d.legend command as a list

        :return: bitmap with legend
        """
        Debug.msg(5, "BitmapProvider.LoadOverlay: cmd={c}".format(c=cmd))

        fileHandler, filename = tempfile.mkstemp(suffix=".png")
        os.close(fileHandler)
        # Set the environment variables for this process
        _setEnvironment(
            self.imageWidth,
            self.imageHeight,
            filename,
            transparent=True,
            bgcolor=(0, 0, 0),
        )

        Debug.msg(1, "Render raster legend " + str(filename))
        cmdTuple = cmdlist_to_tuple(cmd)
        returncode, stdout, messages = read2_command(cmdTuple[0], **cmdTuple[1])

        if returncode == 0:
            return BitmapFromImage(autoCropImageFromFile(filename))
        else:
            os.remove(filename)
            raise GException(messages)


class BitmapRenderer:
    """Class which renders 2D and 3D images to files."""

    def __init__(self, mapFilesPool, tempDir, imageWidth, imageHeight):
        self._mapFilesPool = mapFilesPool
        self._tempDir = tempDir
        self.imageWidth = imageWidth
        self.imageHeight = imageHeight

        self.renderingContinues = Signal("BitmapRenderer.renderingContinues")
        self._stopRendering = False
        self._isRendering = False

    def Render(self, cmdList, regions, regionFor3D, bgcolor, force, nprocs):
        """Renders all maps and stores files.

        :param cmdList: list of rendering commands to run
        :param regions: regions for 2D rendering assigned to commands
        :param regionFor3D: region for setting 3D view
        :param bgcolor: background color as a tuple of 3 values 0 to 255
        :param force: if True reload all data, otherwise only missing data
        :param nprocs: number of procs to be used for rendering
        """
        Debug.msg(3, "BitmapRenderer.Render")
        count = 0

        # Variables for parallel rendering
        proc_count = 0
        proc_list = []
        queue_list = []
        cmd_list = []

        filteredCmdList = []
        for cmd, region in zip(cmdList, regions):
            if cmd[0] == "m.nviz.image":
                region = None
            filename = GetFileFromCmd(self._tempDir, cmd, region)
            if (
                not force
                and os.path.exists(filename)
                and self._mapFilesPool.GetSize(HashCmd(cmd, region))
                == (self.imageWidth, self.imageHeight)
            ):
                # for reference counting
                self._mapFilesPool[HashCmd(cmd, region)] = filename
                continue
            filteredCmdList.append((cmd, region))

        mapNum = len(filteredCmdList)
        stopped = False
        self._isRendering = True
        for cmd, region in filteredCmdList:
            count += 1

            # Queue object for interprocess communication
            q = Queue()
            # The separate render process
            if cmd[0] == "m.nviz.image":
                p = Process(
                    target=RenderProcess3D,
                    args=(
                        self.imageWidth,
                        self.imageHeight,
                        self._tempDir,
                        cmd,
                        regionFor3D,
                        bgcolor,
                        q,
                    ),
                )
            else:
                p = Process(
                    target=RenderProcess2D,
                    args=(
                        self.imageWidth,
                        self.imageHeight,
                        self._tempDir,
                        cmd,
                        region,
                        bgcolor,
                        q,
                    ),
                )
            p.start()

            queue_list.append(q)
            proc_list.append(p)
            cmd_list.append((cmd, region))

            proc_count += 1
            # Wait for all running processes and read/store the created images
            if proc_count == nprocs or count == mapNum:
                for i in range(len(cmd_list)):
                    proc_list[i].join()
                    filename = queue_list[i].get()
                    self._mapFilesPool[
                        HashCmd(cmd_list[i][0], cmd_list[i][1])
                    ] = filename
                    self._mapFilesPool.SetSize(
                        HashCmd(cmd_list[i][0], cmd_list[i][1]),
                        (self.imageWidth, self.imageHeight),
                    )

                proc_count = 0
                proc_list = []
                queue_list = []
                cmd_list = []

            self.renderingContinues.emit(current=count, text=_("Rendering map layers"))
            if self._stopRendering:
                self._stopRendering = False
                stopped = True
                break

        self._isRendering = False
        return not stopped

    def RequestStopRendering(self):
        """Requests to stop rendering."""
        if self._isRendering:
            self._stopRendering = True


class BitmapComposer:
    """Class which handles the composition of image files with g.pnmcomp."""

    def __init__(self, tempDir, mapFilesPool, bitmapPool, imageWidth, imageHeight):
        self._mapFilesPool = mapFilesPool
        self._bitmapPool = bitmapPool
        self._tempDir = tempDir
        self.imageWidth = imageWidth
        self.imageHeight = imageHeight

        self.compositionContinues = Signal("BitmapComposer.composingContinues")
        self._stopComposing = False
        self._isComposing = False

    def Compose(self, cmdLists, regions, opacityList, bgcolor, force, nprocs):
        """Performs the composition of ppm/pgm files.

        :param cmdLists: lists of rendering commands lists to compose
        :param regions: regions for 2D rendering assigned to commands
        :param opacityList: list of lists of opacity values
        :param bgcolor: background color as a tuple of 3 values 0 to 255
        :param force: if True reload all data, otherwise only missing data
        :param nprocs: number of procs to be used for rendering
        """
        Debug.msg(3, "BitmapComposer.Compose")

        count = 0

        # Variables for parallel rendering
        proc_count = 0
        proc_list = []
        queue_list = []
        cmd_lists = []

        filteredCmdLists = []
        for cmdList, region in zip(cmdLists, regions):
            if (
                not force
                and HashCmds(cmdList, region) in self._bitmapPool
                and self._bitmapPool[HashCmds(cmdList, region)].GetSize()
                == (self.imageWidth, self.imageHeight)
            ):
                # TODO: find a better way than to assign the same to increase
                # the reference
                self._bitmapPool[HashCmds(cmdList, region)] = self._bitmapPool[
                    HashCmds(cmdList, region)
                ]
                continue
            filteredCmdLists.append((cmdList, region))

        num = len(filteredCmdLists)

        self._isComposing = True
        for cmdList, region in filteredCmdLists:
            count += 1
            # Queue object for interprocess communication
            q = Queue()
            # The separate render process
            p = Process(
                target=CompositeProcess,
                args=(
                    self.imageWidth,
                    self.imageHeight,
                    self._tempDir,
                    cmdList,
                    region,
                    opacityList,
                    bgcolor,
                    q,
                ),
            )
            p.start()

            queue_list.append(q)
            proc_list.append(p)
            cmd_lists.append((cmdList, region))

            proc_count += 1

            # Wait for all running processes and read/store the created images
            if proc_count == nprocs or count == num:
                for i in range(len(cmd_lists)):
                    proc_list[i].join()
                    filename = queue_list[i].get()
                    if filename is None:
                        self._bitmapPool[
                            HashCmds(cmd_lists[i][0], cmd_lists[i][1])
                        ] = createNoDataBitmap(
                            self.imageWidth, self.imageHeight, text="Failed to render"
                        )
                    else:
                        self._bitmapPool[
                            HashCmds(cmd_lists[i][0], cmd_lists[i][1])
                        ] = BitmapFromImage(wx.Image(filename))
                        os.remove(filename)
                proc_count = 0
                proc_list = []
                queue_list = []
                cmd_lists = []

            self.compositionContinues.emit(
                current=count, text=_("Overlaying map layers")
            )
            if self._stopComposing:
                self._stopComposing = False
                break

        self._isComposing = False

    def RequestStopComposing(self):
        """Requests to stop the composition."""
        if self._isComposing:
            self._stopComposing = True


def RenderProcess2D(imageWidth, imageHeight, tempDir, cmd, region, bgcolor, fileQueue):
    """Render raster or vector files as ppm image and write the
       resulting ppm filename in the provided file queue

    :param imageWidth: image width
    :param imageHeight: image height
    :param tempDir: directory for rendering
    :param cmd: d.rast/d.vect command as a list
    :param region: region as a dict or None
    :param bgcolor: background color as a tuple of 3 values 0 to 255
    :param fileQueue: the inter process communication queue
                      storing the file name of the image
    """

    filename = GetFileFromCmd(tempDir, cmd, region)
    transparency = True

    # Set the environment variables for this process
    _setEnvironment(
        imageWidth, imageHeight, filename, transparent=transparency, bgcolor=bgcolor
    )
    if region:
        os.environ["GRASS_REGION"] = gcore.region_env(**region)
    cmdTuple = cmdlist_to_tuple(cmd)
    returncode, stdout, messages = read2_command(cmdTuple[0], **cmdTuple[1])
    if returncode != 0:
        gcore.warning("Rendering failed:\n" + messages)
        fileQueue.put(None)
        if region:
            os.environ.pop("GRASS_REGION")
        os.remove(filename)
        return

    if region:
        os.environ.pop("GRASS_REGION")
    fileQueue.put(filename)


def RenderProcess3D(imageWidth, imageHeight, tempDir, cmd, region, bgcolor, fileQueue):
    """Renders image with m.nviz.image and writes the
       resulting ppm filename in the provided file queue

    :param imageWidth: image width
    :param imageHeight: image height
    :param tempDir: directory for rendering
    :param cmd: m.nviz.image command as a list
    :param region: region as a dict
    :param bgcolor: background color as a tuple of 3 values 0 to 255
    :param fileQueue: the inter process communication queue
                      storing the file name of the image
    """

    filename = GetFileFromCmd(tempDir, cmd, None)
    os.environ["GRASS_REGION"] = gcore.region_env(region3d=True, **region)
    Debug.msg(1, "Render image to file " + str(filename))

    cmdTuple = cmdlist_to_tuple(cmd)
    cmdTuple[1]["output"] = os.path.splitext(filename)[0]
    # set size
    cmdTuple[1]["size"] = "%d,%d" % (imageWidth, imageHeight)
    # set format
    cmdTuple[1]["format"] = "ppm"
    cmdTuple[1]["bgcolor"] = bgcolor = ":".join([str(part) for part in bgcolor])
    returncode, stdout, messages = read2_command(cmdTuple[0], **cmdTuple[1])
    if returncode != 0:
        gcore.warning("Rendering failed:\n" + messages)
        fileQueue.put(None)
        os.environ.pop("GRASS_REGION")
        return

    os.environ.pop("GRASS_REGION")
    fileQueue.put(filename)


def CompositeProcess(
    imageWidth, imageHeight, tempDir, cmdList, region, opacities, bgcolor, fileQueue
):
    """Performs the composition of image ppm files and writes the
       resulting ppm filename in the provided file queue

    :param imageWidth: image width
    :param imageHeight: image height
    :param tempDir: directory for rendering
    :param cmdList: list of d.rast/d.vect commands
    :param region: region as a dict or None
    :param opacites: list of opacities
    :param bgcolor: background color as a tuple of 3 values 0 to 255
    :param fileQueue: the inter process communication queue
                      storing the file name of the image
    """

    maps = []
    masks = []
    for cmd in cmdList:
        maps.append(GetFileFromCmd(tempDir, cmd, region))
        masks.append(GetFileFromCmd(tempDir, cmd, region, "pgm"))
    filename = GetFileFromCmds(tempDir, cmdList, region)
    # Set the environment variables for this process
    _setEnvironment(
        imageWidth, imageHeight, filename, transparent=False, bgcolor=bgcolor
    )

    opacities = [str(op) for op in opacities]
    bgcolor = ":".join([str(part) for part in bgcolor])
    returncode, stdout, messages = read2_command(
        "g.pnmcomp",
        overwrite=True,
        input="%s" % ",".join(reversed(maps)),
        mask="%s" % ",".join(reversed(masks)),
        opacity="%s" % ",".join(reversed(opacities)),
        bgcolor=bgcolor,
        width=imageWidth,
        height=imageHeight,
        output=filename,
    )

    if returncode != 0:
        gcore.warning("Rendering composite failed:\n" + messages)
        fileQueue.put(None)
        os.remove(filename)
        return

    fileQueue.put(filename)


class DictRefCounter:
    """Base class storing map files/bitmaps (emulates dictionary).
    Counts the references to know which files/bitmaps to delete.
    """

    def __init__(self):
        self.dictionary = {}
        self.referenceCount = {}

    def __getitem__(self, key):
        return self.dictionary[key]

    def __setitem__(self, key, value):
        self.dictionary[key] = value
        if key not in self.referenceCount:
            self.referenceCount[key] = 1
        else:
            self.referenceCount[key] += 1
        Debug.msg(5, "DictRefCounter.__setitem__: +1 for key {k}".format(k=key))

    def __contains__(self, key):
        return key in self.dictionary

    def __delitem__(self, key):
        self.referenceCount[key] -= 1
        Debug.msg(5, "DictRefCounter.__delitem__: -1 for key {k}".format(k=key))

    def keys(self):
        return self.dictionary.keys()

    def Clear(self):
        """Clears items which are not needed any more."""
        Debug.msg(4, "DictRefCounter.Clear")
        for key in self.dictionary.copy().keys():
            if key is not None:
                if self.referenceCount[key] <= 0:
                    del self.dictionary[key]
                    del self.referenceCount[key]


class MapFilesPool(DictRefCounter):
    """Stores rendered images as files."""

    def __init__(self):
        DictRefCounter.__init__(self)
        self.size = {}

    def SetSize(self, key, size):
        self.size[key] = size

    def GetSize(self, key):
        return self.size[key]

    def Clear(self):
        """Removes files which are not needed anymore.
        Removes both ppm and pgm.
        """
        Debug.msg(4, "MapFilesPool.Clear")

        for key in list(self.dictionary.keys()):
            if self.referenceCount[key] <= 0:
                name, ext = os.path.splitext(self.dictionary[key])
                os.remove(self.dictionary[key])
                if ext == ".ppm":
                    os.remove(name + ".pgm")
                del self.dictionary[key]
                del self.referenceCount[key]
                del self.size[key]


class BitmapPool(DictRefCounter):
    """Class storing bitmaps (emulates dictionary)"""

    def __init__(self):
        DictRefCounter.__init__(self)


class CleanUp:
    """Responsible for cleaning up the files."""

    def __init__(self, tempDir):
        self._tempDir = tempDir

    def __call__(self):
        import shutil

        if os.path.exists(self._tempDir):
            try:
                shutil.rmtree(self._tempDir)
                Debug.msg(5, "CleanUp: removed directory {t}".format(t=self._tempDir))
            except OSError:
                gcore.warning(_("Directory {t} not removed.").format(t=self._tempDir))


def _setEnvironment(width, height, filename, transparent, bgcolor):
    """Sets environmental variables for 2D rendering.

    :param width: rendering width
    :param height: rendering height
    :param filename: file name
    :param transparent: use transparency
    :param bgcolor: background color as a tuple of 3 values 0 to 255
    """
    Debug.msg(
        5,
        "_setEnvironment: width={w}, height={h}, "
        "filename={f}, transparent={t}, bgcolor={b}".format(
            w=width, h=height, f=filename, t=transparent, b=bgcolor
        ),
    )

    os.environ["GRASS_RENDER_WIDTH"] = str(width)
    os.environ["GRASS_RENDER_HEIGHT"] = str(height)
    driver = UserSettings.Get(group="display", key="driver", subkey="type")
    os.environ["GRASS_RENDER_IMMEDIATE"] = driver
    os.environ["GRASS_RENDER_BACKGROUNDCOLOR"] = "{r:02x}{g:02x}{b:02x}".format(
        r=bgcolor[0], g=bgcolor[1], b=bgcolor[2]
    )
    os.environ["GRASS_RENDER_TRUECOLOR"] = "TRUE"
    if transparent:
        os.environ["GRASS_RENDER_TRANSPARENT"] = "TRUE"
    else:
        os.environ["GRASS_RENDER_TRANSPARENT"] = "FALSE"
    os.environ["GRASS_RENDER_FILE"] = str(filename)


def createNoDataBitmap(imageWidth, imageHeight, text="No data"):
    """Creates 'no data' bitmap.

    Used when requested bitmap is not available (loading data was not successful) or
    we want to show 'no data' bitmap.

    :param imageWidth: image width
    :param imageHeight: image height
    """
    Debug.msg(
        4,
        "createNoDataBitmap: w={w}, h={h}, text={t}".format(
            w=imageWidth, h=imageHeight, t=text
        ),
    )
    bitmap = EmptyBitmap(imageWidth, imageHeight)
    dc = wx.MemoryDC()
    dc.SelectObject(bitmap)
    dc.Clear()
    text = _(text)
    dc.SetFont(
        wx.Font(
            pointSize=40,
            family=wx.FONTFAMILY_SCRIPT,
            style=wx.FONTSTYLE_NORMAL,
            weight=wx.FONTWEIGHT_BOLD,
        )
    )
    tw, th = dc.GetTextExtent(text)
    dc.DrawText(text, (imageWidth - tw) / 2, (imageHeight - th) / 2)
    dc.SelectObject(wx.NullBitmap)
    return bitmap


def read2_command(*args, **kwargs):
    kwargs["stdout"] = gcore.PIPE
    kwargs["stderr"] = gcore.PIPE
    ps = gcore.start_command(*args, **kwargs)
    stdout, stderr = ps.communicate()
    return ps.returncode, DecodeString(stdout), DecodeString(stderr)


def test():
    import shutil

    from core.layerlist import LayerList, Layer
    from animation.data import AnimLayer
    from animation.utils import layerListToCmdsMatrix
    import grass.temporal as tgis

    tgis.init()

    layerList = LayerList()
    layer = AnimLayer()
    layer.mapType = "strds"
    layer.name = "JR"
    layer.cmd = ["d.rast", "map=elev_2007_1m"]
    layerList.AddLayer(layer)

    layer = Layer()
    layer.mapType = "vector"
    layer.name = "buildings_2009_approx"
    layer.cmd = ["d.vect", "map=buildings_2009_approx", "color=grey"]
    layer.opacity = 50
    layerList.AddLayer(layer)

    bPool = BitmapPool()
    mapFilesPool = MapFilesPool()

    tempDir = "/tmp/test"
    if os.path.exists(tempDir):
        shutil.rmtree(tempDir)
    os.mkdir(tempDir)
    # comment this line to keep the directory after prgm ends
    #    cleanUp = CleanUp(tempDir)
    #    import atexit
    #    atexit.register(cleanUp)

    prov = BitmapProvider(bPool, mapFilesPool, tempDir, imageWidth=640, imageHeight=480)
    prov.renderingStarted.connect(
        lambda count: sys.stdout.write("Total number of maps: {c}\n".format(c=count))
    )
    prov.renderingContinues.connect(
        lambda current, text: sys.stdout.write(
            "Current number: {c}\n".format(c=current)
        )
    )
    prov.compositionStarted.connect(
        lambda count: sys.stdout.write(
            "Composition: total number of maps: {c}\n".format(c=count)
        )
    )
    prov.compositionContinues.connect(
        lambda current, text: sys.stdout.write(
            "Composition: Current number: {c}\n".format(c=current)
        )
    )
    prov.mapsLoaded.connect(lambda: sys.stdout.write("Maps loading finished\n"))
    cmdMatrix = layerListToCmdsMatrix(layerList)
    prov.SetCmds(cmdMatrix, [layer.opacity for layer in layerList])
    app = wx.App()

    prov.Load(bgcolor=(13, 156, 230), nprocs=4)

    for key in bPool.keys():
        if key is not None:
            bPool[key].SaveFile(os.path.join(tempDir, key + ".png"), wx.BITMAP_TYPE_PNG)


#    prov.Unload()
#    prov.SetCmds(cmdMatrix, [l.opacity for l in layerList])
#    prov.Load(bgcolor=(13, 156, 230))
#    prov.Unload()
#    newList = LayerList()
#    prov.SetCmds(layerListToCmdsMatrix(newList), [l.opacity for l in newList])
#    prov.Load()
#    prov.Unload()
#    mapFilesPool.Clear()
#    bPool.Clear()
#    print bPool.keys(), mapFilesPool.keys()


if __name__ == "__main__":
    test()
