#!/usr/bin/env python3
"""
This module implements the ProjPickerPanel widget for wxPython.
"""

import wx.lib.newevent
import wx.lib.statbmp
import io
import threading
import queue
import textwrap
import webbrowser
import locale
import sys

import grass.projpicker as ppik
from grass.getosm import OpenStreetMap

from .gui_common import (get_latlon, get_zoom, get_dzoom, parse_geoms,
                         adjust_lon, calc_geoms_bbox, create_crs_info,
                         find_bbox)

ItemSelectedEvent, EVT_ITEM_SELECTED = wx.lib.newevent.NewEvent()
ItemDeselectedEvent, EVT_ITEM_DESELECTED = wx.lib.newevent.NewEvent()


class ProjPickerPanel(wx.Panel):
    def __init__(self, *args, filter_bbox=None, **kwargs):
        super().__init__(*args, **kwargs)

        self.filter_bbox = filter_bbox

        self.sel_crs = None
        self.sel_bbox = []

        self.zoomer = None
        # timer delay must be positive on macOS
        self.zoomer_delay = 1 if sys.platform == "darwin" else 0
        self.zoomer_queue = queue.Queue()
        self.dzoom = get_dzoom()

        self.dragged = False
        self.dragging_bbox = False
        self.dragged_bbox = []
        self.prev_xy = []
        self.bbox = []
        self.geoms = []
        self.curr_geom = []
        self.all_geoms = []

        self.line_width = 2
        self.fill_alpha = 50
        self.geoms_color = "blue"
        self.dragged_bbox_color = "green"
        self.sel_bbox_color = "red"

        width, height = kwargs.pop("size", (800, 800))
        width = min(width, self.Parent.Size.Width)
        height = min(height, self.Parent.Size.Height)

        doc_url = "https://projpicker.readthedocs.io/"

        lat, lon = get_latlon()
        zoom = get_zoom()

        main_box = wx.BoxSizer(wx.HORIZONTAL)

        ###########
        # map frame
        map_box = wx.BoxSizer(wx.VERTICAL)

        map_canvas_width = width // 2
        map_canvas_height = height
        map_canvas_size = (map_canvas_width, map_canvas_height)

        self.map_canvas = wx.lib.statbmp.GenStaticBitmap(
                        self, wx.ID_ANY, wx.NullBitmap, size=map_canvas_size)

        self.osm = OpenStreetMap(
                self.create_image,
                self.draw_image,
                lambda data: wx.Image(io.BytesIO(data)),
                lambda image, tile, x, y: image.Paste(tile, x, y),
                lambda tile, dz: tile.Scale(tile.Width*2**dz,
                                            tile.Height*2**dz),
                map_canvas_width, map_canvas_height,
                lat, lon, zoom)

        self.map_canvas.Bind(wx.EVT_LEFT_DOWN, self.on_grab)
        self.map_canvas.Bind(wx.EVT_LEFT_UP, self.on_draw)
        self.map_canvas.Bind(wx.EVT_RIGHT_UP, self.on_cancel_drawing)
        self.map_canvas.Bind(wx.EVT_RIGHT_DCLICK, self.on_clear_drawing)
        self.map_canvas.Bind(wx.EVT_MOTION, self.on_move)
        self.map_canvas.Bind(wx.EVT_MOUSEWHEEL, self.on_zoom)
        self.map_canvas.Bind(wx.EVT_SIZE, self.on_resize)
        self.map_canvas.Bind(wx.EVT_PAINT, self.on_paint)
        map_box.Add(self.map_canvas, 1, wx.EXPAND)

        #######################
        # label for coordinates

        # label=" "*40 hack to reserve enough space for coordinates
        self.coor_label = wx.StaticText(self, label=" "*40)
        # to avoid redrawing the entire map just to refresh coordinates; use a
        # vertical box sizer for right alignment, which is not allowed with a
        # horizontal box sizer
        self.coor_label.box = wx.BoxSizer(wx.VERTICAL)
        self.coor_label.box.Add(self.coor_label, 0, wx.ALIGN_RIGHT)
        map_box.Add(self.coor_label.box, 0, wx.ALIGN_RIGHT)

        main_box.Add(map_box, 1, wx.EXPAND)

        ####################
        # bottom/right frame
        bottom_box = wx.BoxSizer(wx.HORIZONTAL)

        mono_font = wx.Font(9, wx.FONTFAMILY_TELETYPE, wx.FONTSTYLE_NORMAL,
                            wx.FONTWEIGHT_NORMAL)

        # bottom/right notebook
        self.notebook = wx.Notebook(self)

        # list of CRSs tab
        self.crs_panel = wx.Panel(self.notebook)
        self.crs_panel.page = self.notebook.PageCount
        self.notebook.AddPage(self.crs_panel, _("CRS List"))

        # help tab
        help_panel = wx.Panel(self.notebook)
        help_panel.page = self.notebook.PageCount
        self.notebook.AddPage(help_panel, _("Help"))

        # list of CRSs
        crs_box = wx.BoxSizer(wx.VERTICAL)
        self.crs_list = wx.ListCtrl(self.crs_panel, style=wx.LC_REPORT |
                                                          wx.LC_SINGLE_SEL)

        self.crs_list.AppendColumn(_("ID"))
        self.crs_list.AppendColumn(_("Name"))

        self.crs_list.Bind(wx.EVT_LIST_ITEM_SELECTED, self.on_select_crs)

        crs_box.Add(self.crs_list, 1, wx.EXPAND)

        # text for search
        self.search_text = wx.SearchCtrl(self.crs_panel, size=(40, 30))
        self.search_text.ShowCancelButton(True)
        self.search_text.Bind(wx.EVT_TEXT, lambda e: self.search())
        crs_box.Add(self.search_text, 0, wx.EXPAND)

        self.crs_panel.SetSizer(crs_box)

        ############
        # help panel
        help_box = wx.BoxSizer()

        # text for help
        help_text = wx.TextCtrl(
                help_panel, value=textwrap.dedent(_(f"""\
                Pan:               Left drag
                Zoom:              Scroll
                Zoom to bboxes:    Ctrl + scroll up
                Zoom to the world: Ctrl + scroll down
                Zoom to a bbox:    Ctrl + left drag
                Draw a bbox:       Left click
                Cancel drawing:    Right click
                Clear:             Double right click

                Search words can be a CRS ID or
                separated by a semicolon to search
                multiple fields using the logical AND
                operator.

                See {doc_url}
                to learn more.""")),
                style=wx.TE_MULTILINE | wx.TE_READONLY | wx.HSCROLL |
                      wx.TE_AUTO_URL)
        help_text.Bind(wx.EVT_TEXT_URL,
                       lambda e: webbrowser.open(doc_url)
                                 if e.MouseEvent.LeftIsDown() else None)
        help_text.SetFont(mono_font)
        help_box.Add(help_text, 1, wx.EXPAND)
        help_panel.SetSizer(help_box)

        bottom_box.Add(self.notebook, 1, wx.EXPAND)

        main_box.Add(bottom_box, 1, wx.EXPAND)

        self.SetSizer(main_box)

    def on_grab(self, event):
        self.osm.grab(event.x, event.y)

    def on_draw(self, event):
        if self.dragging_bbox:
            ng = len(self.dragged_bbox)
            s = min(self.dragged_bbox[0][0], self.dragged_bbox[ng-1][0])
            n = max(self.dragged_bbox[0][0], self.dragged_bbox[ng-1][0])
            w = self.dragged_bbox[0][1]
            e = self.dragged_bbox[ng-1][1]
            if s == n:
                n += 0.0001
            if w == e:
                e += 0.0001
            self.osm.zoom_to_bbox([s, n, w, e], False)
            self.dragged_bbox.clear()
            self.dragging_bbox = False
            self.draw_map(event.x, event.y)
        elif not self.dragged:
            latlon = list(self.osm.canvas_to_latlon(event.x, event.y))
            self.curr_geom.append(latlon)
            if len(self.curr_geom) == 2:
                query = ""
                geom = []
                if len(self.curr_geom) == 2:
                    s = min(self.curr_geom[0][0], self.curr_geom[1][0])
                    n = max(self.curr_geom[0][0], self.curr_geom[1][0])
                    w = self.curr_geom[0][1]
                    e = self.curr_geom[1][1]
                    if s == n:
                        n += 0.0001
                    if w == e:
                        e += 0.0001
                    geom.extend(["bbox", [s, n, w, e]])
                    query = f"bbox {s:.4f},{n:.4f},{w:.4f},{e:.4f}"
                self.geoms.extend(geom)
                self.curr_geom.clear()
                self.prev_xy.clear()
                if query:
                    self.query()
                    self.draw_geoms()
        self.dragged = False

    def on_cancel_drawing(self, event):
        self.curr_geom.clear()
        self.prev_xy.clear()
        self.draw_geoms()

    def on_clear_drawing(self, event):
        self.clear()
        self.geoms.clear()

    def on_move(self, event):
        if event.ControlDown() and event.LeftIsDown() and event.Dragging():
            latlon = self.osm.canvas_to_latlon(event.x, event.y)
            if not self.dragging_bbox:
                self.dragging_bbox = True
                self.dragged_bbox.append(latlon)
            else:
                if len(self.dragged_bbox) == 2:
                    del self.dragged_bbox[1]
                self.dragged_bbox.append(latlon)
                self.map_canvas.Refresh()
        elif event.LeftIsDown() and event.Dragging():
            dx, dy = self.osm.drag(event.x, event.y)
            if self.prev_xy:
                self.prev_xy[0] += dx
                self.prev_xy[1] += dy
            self.dragged = True
        else:
            latlon = self.osm.canvas_to_latlon(event.x, event.y)
            self.coor_label.SetLabel(f"{latlon[0]:.4f}, {latlon[1]:.4f} ")
            self.coor_label.box.Layout()
            if self.curr_geom:
                self.draw_map(event.x, event.y)

    def on_zoom(self, event):
        def zoom(x, y, dz, cancel_event):
            if not cancel_event.wait(0.01) and self.osm.redownload():
                self.zoomer_queue.put(self.osm.draw)

        def check_zoomer():
            try:
                draw_map = self.zoomer_queue.get_nowait()
            except queue.Empty:
                self.zoomer.checker = wx.CallLater(self.zoomer_delay,
                        check_zoomer)
            else:
                draw_map()

        dz = event.WheelRotation / event.WheelDelta * self.dzoom

        if event.ControlDown():
            if dz > 0:
                geoms_bbox = calc_geoms_bbox(self.geoms)
                if None not in geoms_bbox:
                    self.osm.zoom_to_bbox(geoms_bbox, False)
            else:
                self.osm.zoom(event.x, event.y, self.osm.z_min - self.osm.z,
                              False)
            self.draw_map(event.x, event.y)
            return

        if self.zoomer:
            self.zoomer.cancel_event.set()
            self.osm.cancel = True
            self.zoomer.join()
            self.osm.cancel = False
            self.zoomer.checker.Stop()

            cancel_event = self.zoomer.cancel_event
            cancel_event.clear()
        else:
            cancel_event = threading.Event()

        # if used without osm.draw(), it works; otherwise, only osm.draw()
        # is visible; timing?
        self.osm.rescale(event.x, event.y, dz)
        self.zoomer = threading.Thread(target=zoom, args=(event.x, event.y, dz,
                                                          cancel_event))
        self.zoomer.cancel_event = cancel_event
        self.zoomer.checker = wx.CallLater(self.zoomer_delay, check_zoomer)
        self.zoomer.start()

    def on_resize(self, event):
        if self.Parent.IsShown():
            self.osm.resize(*event.Size)
            x, y = self.Parent.ScreenToClient(wx.GetMousePosition())
            self.draw_map(x, y)

    def on_paint(self, event):
        def set_pen_brush(color):
            outline = wx.Colour(color)
            fill = wx.Colour(outline.Red(), outline.Green(), outline.Blue(),
                             self.fill_alpha)

            dc.SetPen(wx.Pen(outline, width=self.line_width))

            # not all platforms support alpha?
            # https://wxpython.org/Phoenix/docs/html/wx.Colour.html#wx.Colour.Alpha
            if fill.Alpha() == wx.ALPHA_OPAQUE:
                dc.SetBrush(wx.Brush(fill, wx.BRUSHSTYLE_CROSSDIAG_HATCH))
            else:
                dc.SetBrush(wx.Brush(fill))

        # https://wxpython.org/Phoenix/docs/html/wx.BufferedPaintDC.html
        # use wx.BufferedPaintDC() to enable automatic double-buffered drawing;
        # then use wx.GCDC() to enable the alpha channel; ideally,
        # wx.AutoBufferedPaintDC() would be better because it is only enabled
        # when the native platform does not support double-buffered drawing,
        # but wx.GCDC() does not aceept it;
        # map_canvas.SetBackgroundStyle(wx.BG_STYLE_PAINT) was not needed
        dc = wx.GCDC(wx.BufferedPaintDC(self.map_canvas))
        dc.DrawBitmap(self.map_canvas.bitmap, 0, 0)

        set_pen_brush(self.geoms_color)

        geom_type = "bbox"
        g = 0
        ngeoms = len(self.all_geoms)
        while g < ngeoms:
            geom = self.all_geoms[g]
            if geom == "bbox":
                g += 1
                geom = self.all_geoms[g]
            if type(geom) == list:
                for xy in self.osm.get_bbox_xy(geom):
                    x, y = xy[0]
                    w, h = xy[1][0] - x, xy[1][1] - y
                    dc.DrawRectangle(x, y, w, h)
            g += 1

        if self.dragged_bbox:
            set_pen_brush(self.dragged_bbox_color)

            s = self.dragged_bbox[1][0]
            n = self.dragged_bbox[0][0]
            w = self.dragged_bbox[0][1]
            e = self.dragged_bbox[1][1]

            for xy in self.osm.get_bbox_xy((s, n, w, e)):
                x, y = xy[0]
                w, h = xy[1][0] - x, xy[1][1] - y
                dc.DrawRectangle(x, y, w, h)

        if self.sel_bbox:
            set_pen_brush(self.sel_bbox_color)

            for xy in self.osm.get_bbox_xy(self.sel_bbox):
                x, y = xy[0]
                w, h = xy[1][0] - x, xy[1][1] - y
                dc.DrawRectangle(x, y, w, h)

    def on_select_crs(self, event):
        self.sel_bbox.clear()

        item = self.crs_list.GetFirstSelected()
        self.sel_crs = self.crs_list.GetItemText(item, 0)

        b = find_bbox(self.sel_crs, self.bbox)
        s, n, w, e = b.south_lat, b.north_lat, b.west_lon, b.east_lon
        s, n, w, e = self.osm.zoom_to_bbox([s, n, w, e])
        self.sel_bbox.extend([s, n, w, e])

        self.draw_geoms()
        self.post_item_selected(b)

    def post_item_selected(self, bbox):
        wx.PostEvent(self.GetEventHandler(), ItemSelectedEvent(crs=bbox))

    def post_item_deselected(self):
        wx.PostEvent(self.GetEventHandler(), ItemDeselectedEvent())

    def draw_map(self, x, y):
        self.osm.draw()
        self.draw_geoms(x, y)

    def draw_geoms(self, x=None, y=None):
        self.all_geoms.clear()
        self.all_geoms.extend(self.geoms)
        if self.curr_geom and x and y:
            latlon = list(self.osm.canvas_to_latlon(x, y))

            g = self.curr_geom.copy()
            g.append(latlon)

            ng = len(g)
            s = min(g[0][0], g[ng-1][0])
            n = max(g[0][0], g[ng-1][0])
            w = g[0][1]
            e = g[ng-1][1]
            if s == n:
                n += 0.0001
            if w == e:
                e += 0.0001
            self.all_geoms.extend(["bbox", [s, n, w, e]])

        self.map_canvas.Refresh()

    def create_image(self, width, height):
        image = wx.Image(width, height)
        image.Replace(0, 0, 0, *self.Parent.BackgroundColour[:3])
        return image

    def draw_image(self, image):
        # just save the map image for now; later, double-buffered drawing will
        # be done in on_paint() triggered by map_canvas.Refresh()
        self.map_canvas.bitmap = wx.Bitmap(image)
        self.map_canvas.Refresh()

    def query(self):
        try:
            self.bbox = ppik.query_mixed_geoms(self.geoms)
        except Exception as e:
            wx.MessageDialog(self, str(e), _("Query error")).ShowModal()

        self.populate_crs_list(self.bbox)
        self.search()
        self.draw_geoms()

    def populate_crs_list(self, bbox):
        self.crs_list.DeleteAllItems()
        for b in bbox:
            if not self.filter_bbox or self.filter_bbox(b):
                self.crs_list.Append((f"{b.crs_auth_name}:{b.crs_code}",
                                      b.crs_name))
        self.sel_bbox.clear()
        self.post_item_deselected()

        self.crs_list.SetColumnWidth(0, wx.LIST_AUTOSIZE_USEHEADER)
        self.crs_list.SetColumnWidth(1, wx.LIST_AUTOSIZE_USEHEADER)

    def search(self):
        text = [x.strip() for x in self.search_text.Value.split(";")]
        if "".join(text):
            filt_bbox = ppik.search_bbox(self.bbox, text)
            self.populate_crs_list(filt_bbox)

    def clear(self):
        self.sel_crs = None
        self.geoms.clear()
        self.all_geoms.clear()
        self.curr_geom.clear()
        self.prev_xy.clear()
        self.sel_bbox.clear()
        self.draw_geoms()
        self.crs_list.DeleteAllItems()
        self.post_item_deselected()
