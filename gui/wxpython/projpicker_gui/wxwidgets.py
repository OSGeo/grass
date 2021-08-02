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

import grass.projpicker as ppik
if __package__:
    from .gui_common import (get_latlon, get_zoom, get_dzoom, parse_geoms,
                             adjust_lon, calc_geoms_bbox, create_crs_info,
                             find_bbox)
    from .getosm import OpenStreetMap
else:
    from gui_common import (get_latlon, get_zoom, get_dzoom, parse_geoms,
                            adjust_lon, calc_geoms_bbox, create_crs_info,
                            find_bbox)
    from getosm import OpenStreetMap

ItemSelectedEvent, EVT_ITEM_SELECTED = wx.lib.newevent.NewEvent()
ItemDeselectedEvent, EVT_ITEM_DESELECTED = wx.lib.newevent.NewEvent()


class ProjPickerPanel(wx.Panel):
    def __init__(self, *args, layout="full", filter_bbox=None, **kwargs):
        if layout not in ("full", "map_top", "map_left"):
            raise Exception(f"{layout}: Layout not full, map_top, or map_left")

        super().__init__(*args, **kwargs)
#        self.Parent.SetBackgroundColour(wx.Colour("lightgray"))

        self.layout = layout
        self.filter_bbox = filter_bbox

        self.sel_crs = None
        self.sel_bbox = []

        self.zoomer = None
        self.zoomer_queue = queue.Queue()
        self.dzoom = get_dzoom()

        self.dragged = False
        self.dragging_bbox = False
        self.dragged_bbox = []
        self.drawing_bbox = False
        self.complete_drawing = False
        self.prev_xy = []
        self.bbox = []
        self.geoms = []
        self.curr_geom = []
        self.all_geoms = []

        self.point_size = 4
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

        main_box = wx.BoxSizer(wx.HORIZONTAL if layout == "map_left" else
                               wx.VERTICAL)

        ###########
        # map frame
        map_box = wx.BoxSizer(wx.VERTICAL)

        if layout == "map_left":
            map_canvas_width = width // 2
            map_canvas_height = height
        else:
            map_canvas_width = width
            map_canvas_height = height // 2
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
        self.map_canvas.Bind(wx.EVT_LEFT_DCLICK, self.on_complete_drawing)
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

        if self.layout == "full":
            ###################
            # bottom-left frame
            bottom_left_box = wx.BoxSizer(wx.VERTICAL)

            # list of CRSs
            self.crs_list = wx.ListCtrl(self, style=wx.LC_REPORT |
                                                    wx.LC_SINGLE_SEL)

            self.crs_list.AppendColumn("ID")
            self.crs_list.AppendColumn("Name")
            self.crs_list.SetColumnWidth(0, 110)
            self.crs_list.SetColumnWidth(1, wx.LIST_AUTOSIZE_USEHEADER)
            bottom_left_box.Add(self.crs_list, 1, wx.EXPAND)

            self.crs_list.Bind(wx.EVT_LIST_ITEM_SELECTED, self.on_select_crs)

            # add text for search later to get the button height

            bottom_box.Add(bottom_left_box, 1, wx.EXPAND)

            ####################
            # bottom-right frame
            self.notebook = wx.Notebook(self)

            # query tab
            self.query_panel = wx.Panel(self.notebook)
            self.query_panel.page = self.notebook.PageCount
            self.notebook.AddPage(self.query_panel, "Query")

            # CRS info tab
            self.crs_info_panel = wx.Panel(self.notebook)
            self.crs_info_panel.page = self.notebook.PageCount
            self.notebook.AddPage(self.crs_info_panel, "CRS Info")

            # log tab
            self.log_panel = wx.Panel(self.notebook)
            self.log_panel.page = self.notebook.PageCount
            self.notebook.AddPage(self.log_panel, "Log")

            # help tab
            help_panel = wx.Panel(self.notebook)
            help_panel.page = self.notebook.PageCount
            self.notebook.AddPage(help_panel, "Help")

            #############
            # query panel
            query_box = wx.BoxSizer(wx.VERTICAL)

            # text for query
            self.query_text = wx.TextCtrl(self.query_panel,
                                     style=wx.TE_MULTILINE | wx.HSCROLL)
            # https://dzone.com/articles/wxpython-learning-use-fonts
            self.query_text.SetFont(mono_font)
            query_box.Add(self.query_text, 1, wx.EXPAND)

            # pop-up menu
            menu = wx.Menu()
            import_menuitem = wx.MenuItem(menu, wx.ID_ANY, "Import query")
            export_menuitem = wx.MenuItem(menu, wx.ID_ANY, "Export query")
            menu.Append(import_menuitem)
            menu.Append(export_menuitem)

            file_types = ("ProjPicker query files (*.ppik)|*.ppik|"
                          "All files (*.*)|*.*")
            self.query_text.Bind(wx.EVT_MENU, lambda e: self.import_query(),
                                 import_menuitem)
            self.query_text.Bind(wx.EVT_MENU, lambda e: self.export_query(),
                                 export_menuitem)

            self.query_text.Bind(
                        wx.EVT_RIGHT_DOWN,
                        lambda e: self.query_text.PopupMenu(menu, e.Position))

            # buttons
            query_button = wx.Button(self.query_panel, label="Query")
            query_button.Bind(wx.EVT_BUTTON, lambda e: self.query())

            clear_button = wx.Button(self.query_panel, label="Clear")
            clear_button.Bind(wx.EVT_BUTTON, lambda e: self.clear())

            query_bottom_box = wx.BoxSizer(wx.HORIZONTAL)
            query_bottom_box.Add(query_button, 1)
            query_bottom_box.AddStretchSpacer()
            query_bottom_box.Add(clear_button, 1)
            query_box.Add(query_bottom_box, 0, wx.ALIGN_CENTER)
            self.query_panel.SetSizer(query_box)

            # back to bottom-left frame to use the button height
            # text for search
            self.search_text = wx.SearchCtrl(
                                self, size=(0, query_button.Size.Height + 1))
            self.search_text.ShowCancelButton(True)
            self.search_text.Bind(wx.EVT_TEXT, lambda e: self.search())
            bottom_left_box.Add(self.search_text, 0, wx.EXPAND)

            ################
            # CRS info panel
            crs_info_box = wx.BoxSizer(wx.VERTICAL)

            # text for CRS info
            self.crs_info_text = wx.TextCtrl(
                    self.crs_info_panel,
                    style=wx.TE_MULTILINE | wx.TE_READONLY | wx.HSCROLL)
            # https://dzone.com/articles/wxpython-learning-use-fonts
            self.crs_info_text.SetFont(mono_font)
            crs_info_box.Add(self.crs_info_text, 1, wx.EXPAND)

            # button
            clear_button = wx.Button(self.crs_info_panel, label="Clear")
            clear_button.Bind(wx.EVT_BUTTON, lambda e: self.clear())
            crs_info_box.Add(clear_button, 0, wx.ALIGN_CENTER)
            self.crs_info_panel.SetSizer(crs_info_box)

            ###########
            # log panel
            log_box = wx.BoxSizer()

            # text for CRS info
            self.log_text = wx.TextCtrl(
                        self.log_panel,
                        style=wx.TE_MULTILINE | wx.TE_READONLY | wx.HSCROLL)
            # https://dzone.com/articles/wxpython-learning-use-fonts
            self.log_text.SetFont(mono_font)
            log_box.Add(self.log_text, 1, wx.EXPAND)
            self.log_panel.SetSizer(log_box)

            ############
            # help panel
            help_box = wx.BoxSizer()

            # text for help
            help_text = wx.TextCtrl(
                    help_panel, value=textwrap.dedent(f"""\
                    Pan:                        Left drag
                    Zoom:                       Scroll
                    Zoom to geometries:         Ctrl + scroll up
                    Zoom to the world:          Ctrl + scroll down
                    Draw/zoom to a bbox:        Ctrl + left drag
                    Draw a point:               Double left click
                    Start drawing a poly:       Left click
                    Start drawing a bbox:       Ctrl + left click
                    Complete a poly/bbox:       Double left click
                    Cancel drawing a poly/bbox: Right click
                    Clear geometries:           Double right click

                    To define a geometry variable, type and highlight
                    a name in the query builder, then create a geometry.

                    Query files (*.ppik) can be imported or exported
                    by right clicking on the query builder.

                    Search words can be a CRS ID or separated by
                    a semicolon to search multiple fields using
                    the logical AND operator.

                    See {doc_url} to learn more."""),
                    style=wx.TE_MULTILINE | wx.TE_READONLY | wx.HSCROLL |
                          wx.TE_AUTO_URL)
            help_text.Bind(wx.EVT_TEXT_URL,
                           lambda e: webbrowser.open(doc_url)
                                     if e.MouseEvent.LeftIsDown() else None)
            help_text.SetFont(mono_font)
            help_box.Add(help_text, 1, wx.EXPAND)
            help_panel.SetSizer(help_box)

            bottom_box.Add(self.notebook, 1, wx.EXPAND)
        else:
            # bottom/right notebook
            self.notebook = wx.Notebook(self)

            # list of CRSs tab
            self.crs_panel = wx.Panel(self.notebook)
            self.crs_panel.page = self.notebook.PageCount
            self.notebook.AddPage(self.crs_panel, "CRS List")

            # help tab
            help_panel = wx.Panel(self.notebook)
            help_panel.page = self.notebook.PageCount
            self.notebook.AddPage(help_panel, "Help")

            # list of CRSs
            crs_box = wx.BoxSizer(wx.VERTICAL)
            self.crs_list = wx.ListCtrl(self.crs_panel, style=wx.LC_REPORT |
                                                              wx.LC_SINGLE_SEL)

            self.crs_list.AppendColumn("ID")
            self.crs_list.AppendColumn("Name")

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
                    help_panel, value=textwrap.dedent(f"""\
                    Pan:                        Left drag
                    Zoom:                       Scroll
                    Zoom to geometries:         Ctrl + scroll up
                    Zoom to the world:          Ctrl + scroll down
                    Draw/zoom to a bbox:        Ctrl + left drag
                    Draw a point:               Double left click
                    Start drawing a poly:       Left click
                    Start drawing a bbox:       Ctrl + left click
                    Complete a poly/bbox:       Double left click
                    Cancel drawing a poly/bbox: Right click
                    Clear geometries:           Double right click

                    Search words can be a CRS ID or separated by
                    a semicolon to search multiple fields using
                    the logical AND operator.

                    See {doc_url} to learn more."""),
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
        elif self.complete_drawing:
            query = ""
            geom = []
            if self.drawing_bbox:
                if len(self.curr_geom) == 2:
                    s = min(self.curr_geom[0][0], self.curr_geom[1][0])
                    n = max(self.curr_geom[0][0], self.curr_geom[1][0])
                    w = self.curr_geom[0][1]
                    e = self.curr_geom[1][1]
                    geom.extend(["bbox", [s, n, w, e]])
                    query = f"bbox {s:.4f},{n:.4f},{w:.4f},{e:.4f}"
                self.drawing_bbox = False
            elif len(self.curr_geom) == 1:
                lat, lon = self.curr_geom[0]
                geom.extend(["point", [lat, lon]])
                query = f"point {lat:.4f},{lon:.4f}"
            elif self.curr_geom:
                geom.extend(["poly", self.curr_geom.copy()])
                query = "poly"
                for g in self.curr_geom:
                    lat, lon = g
                    query += f" {lat:.4f},{lon:.4f}"
            self.geoms.extend(geom)
            self.curr_geom.clear()
            self.prev_xy.clear()
            if query:
                if self.layout == "full":
                    query += "\n"
                    name = self.query_text.StringSelection
                    if name and not name.endswith(":"):
                        if not name.startswith(":"):
                            name = f":{name}:"
                        else:
                            name = ""
                    if name and ppik.geom_var_re.match(name):
                        query = query.replace(" ", f" {name} ", 1)
                    sel = self.query_text.GetSelection()
                    if (sel[0] > 0 and
                        self.query_text.GetRange(sel[0] - 1, sel[0]) != "\n"):
                        query = "\n" + query
                    self.query_text.Replace(sel[0], sel[1], query)
                    self.notebook.ChangeSelection(self.query_panel.page)
                else:
                    self.query()
                self.draw_geoms()
        elif not self.dragged:
            if event.ControlDown():
                self.drawing_bbox = True
                self.curr_geom.clear()
            elif self.drawing_bbox and len(self.curr_geom) == 2:
                del self.curr_geom[1]
            latlon = list(self.osm.canvas_to_latlon(event.x, event.y))
            if not self.drawing_bbox:
                if self.prev_xy:
                    latlon[1] = adjust_lon(
                            self.prev_xy[0], event.x,
                            self.curr_geom[len(self.curr_geom)-1][1],
                            latlon[1])
                self.prev_xy.clear()
                self.prev_xy.extend([event.x, event.y])
            self.curr_geom.append(latlon)

        self.dragged = False
        self.complete_drawing = False

    def on_complete_drawing(self, event):
        self.complete_drawing = True

    def on_cancel_drawing(self, event):
        self.drawing_bbox = False
        self.curr_geom.clear()
        self.prev_xy.clear()
        self.draw_geoms()

    def on_clear_drawing(self, event):
        if self.layout == "full":
            self.query_text.Clear()
        else:
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
            self.osm.drag(event.x, event.y)
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
            except:
                self.zoomer.checker = wx.CallLater(0, check_zoomer)
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
        self.zoomer.checker = wx.CallLater(0, check_zoomer)
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

        point_half_size = self.point_size // 2

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

        geom_type = "point"
        g = 0
        ngeoms = len(self.all_geoms)
        while g < ngeoms:
            geom = self.all_geoms[g]
            if geom in ("point", "poly", "bbox"):
                geom_type = geom
                g += 1
                geom = self.all_geoms[g]
            if type(geom) == list:
                if geom_type == "point":
                    for xy in self.osm.get_xy([geom]):
                        dc.DrawCircle(*xy[0], point_half_size)
                elif geom_type == "poly":
                    for xy in self.osm.get_xy(geom):
                        dc.DrawPolygon(xy)
                else:
                    for xy in self.osm.get_bbox_xy(geom):
                        x, y = xy[0]
                        w, h = xy[1][0] - x, xy[1][1] - y
                        dc.DrawRectangle(x, y, w, h)
            g += 1

        if self.dragged_bbox:
            set_pen_brush(self.dragged_bbox_color)

            ng = len(self.dragged_bbox)
            s = self.dragged_bbox[ng-1][0]
            n = self.dragged_bbox[0][0]
            w = self.dragged_bbox[0][1]
            e = self.dragged_bbox[ng-1][1]

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
        if self.layout == "full":
            self.crs_info_text.Clear()
        self.sel_bbox.clear()

        item = self.crs_list.GetFirstSelected()
        self.sel_crs = self.crs_list.GetItemText(item, 0)

        b = find_bbox(self.sel_crs, self.bbox)
        s, n, w, e = b.south_lat, b.north_lat, b.west_lon, b.east_lon
        s, n, w, e = self.osm.zoom_to_bbox([s, n, w, e])
        self.sel_bbox.extend([s, n, w, e])

        if self.layout == "full":
            crs_info = create_crs_info(b)
            self.crs_info_text.SetValue(crs_info)
            self.notebook.ChangeSelection(self.crs_info_panel.page)

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

            if self.drawing_bbox:
                ng = len(g)
                if ng > 0:
                    s = min(g[0][0], g[ng-1][0])
                    n = max(g[0][0], g[ng-1][0])
                    w = g[0][1]
                    e = g[ng-1][1]
                    if s == n:
                        n += 0.0001
                    if w == e:
                        e += 0.0001
                    self.all_geoms.extend(["bbox", [s, n, w, e]])
            elif g:
                if self.prev_xy:
                    latlon[1] = adjust_lon(
                            self.prev_xy[0], x,
                            self.curr_geom[len(self.curr_geom)-1][1],
                            latlon[1])
                g.append(latlon)
                self.all_geoms.extend(["poly", g])

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

    def import_query(self):
        with wx.FileDialog(self.query_text, "Import query",
                           wildcard=file_types,
                           style=wx.FD_OPEN | wx.FD_FILE_MUST_EXIST) as fd:
            if fd.ShowModal() != wx.ID_CANCEL:
                try:
                    with open(fd.Path) as f:
                        self.query_text.SetValue(f.read())
                        f.close()
                except Exception as e:
                    wx.MessageDialog(self.query_text, str(e),
                                     "Import query error").ShowModal()

    def export_query(self):
        with wx.FileDialog(self.query_text, "Export query",
                           wildcard=file_types, defaultFile="query",
                           style=wx.FD_SAVE | wx.FD_OVERWRITE_PROMPT) as fd:
            if fd.ShowModal() != wx.ID_CANCEL:
                query_to_export = self.query_text.Value
                try:
                    with open(fd.Path, "w") as f:
                        f.write(query_to_export)
                except Exception as e:
                    wx.MessageDialog(self.query_text, str(e),
                                     "Export query error").ShowModal()

    def query(self):
        if self.layout == "full":
            query = self.query_text.Value
            self.geoms.clear()
            try:
                self.geoms.extend(ppik.parse_mixed_geoms(query))
                self.bbox = ppik.query_mixed_geoms(self.geoms)
            except Exception as e:
                self.log_text.SetValue(str(e))
                self.notebook.ChangeSelection(self.log_panel.page)
            else:
                self.log_text.SetValue("")
        else:
            try:
                self.bbox = ppik.query_mixed_geoms(self.geoms)
            except Exception as e:
                wx.MessageDialog(self, str(e), "Query error").ShowModal()

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

        if not self.layout == "full":
            self.crs_list.SetColumnWidth(0, wx.LIST_AUTOSIZE_USEHEADER)
            self.crs_list.SetColumnWidth(1, wx.LIST_AUTOSIZE_USEHEADER)

    def search(self):
        text = [x.strip() for x in self.search_text.Value.split(";")]
        if "".join(text):
            filt_bbox = ppik.search_bbox(self.bbox, text)
            self.populate_crs_list(filt_bbox)

    def clear(self):
        if self.layout == "full":
            self.query_text.Clear()
            self.crs_info_text.Clear()

        self.sel_crs = None
        self.geoms.clear()
        self.all_geoms.clear()
        self.curr_geom.clear()
        self.prev_xy.clear()
        self.sel_bbox.clear()
        self.draw_geoms()
        self.crs_list.DeleteAllItems()
        self.post_item_deselected()

    def get_crs_id(self):
        return self.sel_crs


if __name__ == "__main__":
    app = wx.App()
    root = wx.Frame(None, title="ProjPickerPanel Demo")
    root.SetClientSize(800, 830)

    ppik_panel = ProjPickerPanel(root,
                                 filter_bbox=lambda b: b.crs_auth_name=="EPSG")

    ppik_panel.Bind(EVT_ITEM_SELECTED, lambda e: print(e.crs))
    ppik_panel.Bind(EVT_ITEM_DESELECTED, lambda e: print("---DESELECTED---"))

    root.Show()
    app.MainLoop()

    crs_id = ppik_panel.get_crs_id()
    if crs_id:
        print(crs_id)
