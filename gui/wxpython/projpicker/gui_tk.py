"""
This module implements the ProjPicker GUI using tkinter.
"""

import tkinter as tk
from tkinter import ttk
from tkinter import filedialog
from tkinter import messagebox
import textwrap
import webbrowser
import threading
import queue
import functools

# https://stackoverflow.com/a/49480246/16079666
if __package__:
    from .getosm import OpenStreetMap
    from . import projpicker as ppik
    from .gui_common import (get_latlon, get_zoom, get_dzoom, parse_geoms,
                             adjust_lon, calc_geoms_bbox, create_crs_info,
                             find_bbox)
else:
    from getosm import OpenStreetMap
    import projpicker as ppik
    from gui_common import (get_latlon, get_zoom, get_dzoom, parse_geoms,
                            adjust_lon, calc_geoms_bbox, create_crs_info,
                            find_bbox)

projpicker_dzoom_env = "PROJPICKER_DZOOM"


class AutoScrollbar(ttk.Scrollbar):
    """
    Implement the auto-hiding scrollbar widget.
    https://stackoverflow.com/a/66338658/16079666
    """
    def __init__(self, master, **kwargs):
        """
        Construct an auto scrollbar.

        Args:
            master (widget): Parent widget.
            **kwargs (keyword arguments): Keyword arguments for the auto
                scrollbar widget.
        """
        super().__init__(master, **kwargs)
        self.geometry_manager_add = lambda: None
        self.geometry_manager_forget = lambda: None

    def set(self, first, last):
        """
        Override the set() method.

        Args:
            first (float): First slider position between 0 and 1.
            last (float): Last slider position between 0 and 1.
        """
        if float(first) <= 0.0 and float(last) >= 1.0:
            self.geometry_manager_forget()
        else:
            self.geometry_manager_add()
        super().set(first, last)

    def grid(self, **kwargs):
        """
        Override the grid() method.

        Args:
            **kwargs (keyword arguments): Keyword arguments for grid().
        """
        self.geometry_manager_add = functools.partial(super().grid, **kwargs)
        self.geometry_manager_forget = super().grid_forget

    def pack(self, **kwargs):
        """
        Override the pack() method.

        Args:
            **kwargs (keyword arguments): Keyword arguments for pack().
        """
        # this method is called only once; let's remember its packing order now
        # https://stackoverflow.com/a/58456449/16079666
        siblings = self.master.pack_slaves()
        n = len(siblings)
        after = siblings[n - 1] if n > 0 else None
        self.geometry_manager_add = functools.partial(super().pack,
                                                      after=after, **kwargs)
        self.geometry_manager_forget = super().pack_forget

    def place(self, **kwargs):
        """
        Override the place() method.

        Args:
            **kwargs (keyword arguments): Keyword arguments for place().
        """
        self.geometry_manager_add = functools.partial(super().place, **kwargs)
        self.geometry_manager_forget = super().place_forget


def start(
        geoms=None,
        bbox=[],
        bbox_or_quit=False,
        single=False,
        format_crs_info=None,
        projpicker_db=None):
    """
    Start the GUI. Parsable geometries by geoms or queried BBox instances by
    bbox can be specified optionally. If both are given, bbox is ignored and
    only geoms is used. If single is True, it returns a single BBox instance in
    a list.

    Args:
        geoms (list or str): Parsable geometries. Defaults to None.
        bbox (list): Queried BBox instances. Defaults to [].
        bbox_or_quit (bool): Whether or not to quit when bbox queried using
            input geoms or input bbox is empty. Defaults to False.
        single (bool): Whether or not a single BBox instance is returned.
            Defaults to False.
        format_crs_info (function): User function used for formatting CRS info.
            Defaults to None in which case the default function is provided.
        projpicker_db (str): projpicker.db path. Defaults to None.

    Returns:
        list, list, list: Lists of selected BBox instances, queried BBox
        instances sorted by area, and parsed geometries.
    """
    sel_crs = []
    prev_crs_items = []
    sel_bbox = []

    doc_url = "https://projpicker.readthedocs.io/"

    tag_map = "map"
    tag_bbox = "bbox"
    tag_geoms = "geoms"
    tag_dragged_bbox = "dragged_bbox"
    tag_doc = "doc"

    zoomer = None
    zoomer_queue = queue.Queue()
    dzoom = get_dzoom()

    dragged = False
    dragging_bbox = False
    dragged_bbox = []
    drawing_bbox = False
    complete_drawing = False
    prev_xy = []
    curr_geom = []

    point_size = 4
    line_width = 2
    fill_stipple = "gray12"
    geoms_color = "blue"
    dragged_bbox_color = "green"
    sel_bbox_color = "red"

    lat, lon = get_latlon()
    zoom = get_zoom()

    def on_drag(event):
        nonlocal dragged, dragging_bbox

        if event.state & 0x4:
            # Control + B1-Motion
            latlon = osm.canvas_to_latlon(event.x, event.y)
            if not dragging_bbox:
                dragging_bbox = True
                dragged_bbox.append(latlon)
            else:
                if len(dragged_bbox) == 2:
                    del dragged_bbox[1]
                dragged_bbox.append(latlon)

                ng = len(dragged_bbox)
                s = dragged_bbox[ng-1][0]
                n = dragged_bbox[0][0]
                w = dragged_bbox[0][1]
                e = dragged_bbox[ng-1][1]

                map_canvas.delete(tag_dragged_bbox)
                for xy in osm.get_bbox_xy((s, n, w, e)):
                    map_canvas.create_rectangle(xy, outline=dragged_bbox_color,
                                                width=line_width,
                                                fill=dragged_bbox_color,
                                                stipple=fill_stipple,
                                                tag=tag_dragged_bbox)
            latlon = osm.canvas_to_latlon(event.x, event.y)
            coor_label.config(text=f"{latlon[0]:.4f}, {latlon[1]:.4f} ")
        else:
            osm.drag(event.x, event.y, False)
            draw_map(event.x, event.y)
        dragged = True

    def on_move(event):
        latlon = osm.canvas_to_latlon(event.x, event.y)
        coor_label.config(text=f"{latlon[0]:.4f}, {latlon[1]:.4f} ")
        draw_geoms(event.x, event.y)

    def on_draw(event):
        nonlocal dragged, dragging_bbox, drawing_bbox, complete_drawing

        if dragging_bbox:
            ng = len(dragged_bbox)
            s = min(dragged_bbox[0][0], dragged_bbox[ng-1][0])
            n = max(dragged_bbox[0][0], dragged_bbox[ng-1][0])
            w = dragged_bbox[0][1]
            e = dragged_bbox[ng-1][1]
            if s == n:
                n += 0.0001
            if w == e:
                e += 0.0001
            osm.zoom_to_bbox([s, n, w, e], False)
            dragged_bbox.clear()
            dragging_bbox = False
            map_canvas.delete(tag_dragged_bbox)
            draw_map(event.x, event.y)
        elif complete_drawing:
            query = ""
            geom = []
            if drawing_bbox:
                if len(curr_geom) == 2:
                    s = min(curr_geom[0][0], curr_geom[1][0])
                    n = max(curr_geom[0][0], curr_geom[1][0])
                    w = curr_geom[0][1]
                    e = curr_geom[1][1]
                    geom.extend(["bbox", [s, n, w, e]])
                    query = f"bbox {s:.4f},{n:.4f},{w:.4f},{e:.4f}"
                drawing_bbox = False
            elif len(curr_geom) == 1:
                lat, lon = curr_geom[0]
                geom.extend(["point", [lat, lon]])
                query = f"point {lat:.4f},{lon:.4f}"
            elif curr_geom:
                geom.extend(["poly", curr_geom.copy()])
                query = "poly"
                for g in curr_geom:
                    lat, lon = g
                    query += f" {lat:.4f},{lon:.4f}"
            geoms.extend(geom)
            curr_geom.clear()
            prev_xy.clear()
            if query:
                query += "\n"
                # https://stackoverflow.com/a/35855352/16079666
                # don't use .selection_get()
                ranges = query_text.tag_ranges(tk.SEL)
                if ranges:
                    name = query_text.get(*ranges).strip()
                    if name and not name.endswith(":"):
                        if not name.startswith(":"):
                            name = f":{name}:"
                        else:
                            name = ""
                    if name and ppik.geom_var_re.match(name):
                        query = query.replace(" ", f" {name} ", 1)
                    index = ranges[0].string
                else:
                    index = query_text.index(tk.INSERT)
                line, col = list(map(lambda x: int(x), index.split(".")))
                if col > 0:
                    query = "\n" + query
                    line += 1
                if ranges:
                    query_text.replace(*ranges, query)
                else:
                    query_text.insert(tk.INSERT, query)
                query_text.mark_set(tk.INSERT, f"{line+1}.0")
                notebook.select(query_frame)
                draw_geoms()
        elif not dragged:
            # https://anzeljg.github.io/rin2/book2/2405/docs/tkinter/event-handlers.html
            if event.state & 0x4:
                # Control + ButtonRelease-1
                drawing_bbox = True
                curr_geom.clear()
            elif drawing_bbox and len(curr_geom) == 2:
                del curr_geom[1]
            latlon = list(osm.canvas_to_latlon(event.x, event.y))
            if not drawing_bbox:
                if prev_xy:
                    latlon[1] = adjust_lon(prev_xy[0], event.x,
                                           curr_geom[len(curr_geom)-1][1],
                                           latlon[1])
                prev_xy.clear()
                prev_xy.extend([event.x, event.y])
            curr_geom.append(latlon)

        dragged = False
        complete_drawing = False

    def on_complete_drawing(event):
        nonlocal complete_drawing

        # XXX: sometimes, double-click events occur for both clicks and there
        # is no reliable way to register the first click only using
        # complete_drawing; a hacky way to handle such cases
        if not curr_geom:
            curr_geom.append(osm.canvas_to_latlon(event.x, event.y))
            prev_xy = [event.x, event.y]
        complete_drawing = True

    def on_cancel_drawing(event):
        nonlocal drawing_bbox

        drawing_bbox = False
        curr_geom.clear()
        prev_xy.clear()
        draw_geoms()

    def on_resize(event):
        osm.resize(event.width, event.height)
        x = root.winfo_pointerx() - root.winfo_rootx()
        y = root.winfo_pointery() - root.winfo_rooty()
        draw_map(x, y)

    def on_select_crs(event):
        nonlocal prev_crs_items

        curr_crs_items = crs_treeview.selection()

        if single:
            prev_crs_items.clear()

        curr_crs_item = None
        if len(curr_crs_items) > len(prev_crs_items):
            # selected a new crs
            curr_crs_item = list(set(curr_crs_items) - set(prev_crs_items))[0]
            prev_crs_items.append(curr_crs_item)
        elif len(curr_crs_items) < len(prev_crs_items):
            # deselected an existing crs
            item = list(set(prev_crs_items) - set(curr_crs_items))[0]
            del prev_crs_items[prev_crs_items.index(item)]
            l = len(prev_crs_items)
            if l > 0:
                curr_crs_item = prev_crs_items[l-1]
        elif curr_crs_items:
            prev_crs_items.clear()
            prev_crs_items.extend(curr_crs_items)
            curr_crs_item = prev_crs_items[len(prev_crs_items)-1]

        crs_info_text.delete("1.0", tk.END)
        sel_bbox.clear()
        if curr_crs_item:
            crs_id = crs_treeview.item(curr_crs_item)["values"][0]
            b = find_bbox(crs_id, bbox)
            crs_info = create_crs_info(b, format_crs_info)
            crs_info_text.insert(tk.END, crs_info)

            s, n, w, e = b.south_lat, b.north_lat, b.west_lon, b.east_lon
            s, n, w, e = osm.zoom_to_bbox([s, n, w, e])
            sel_bbox.extend([s, n, w, e])

            notebook.select(crs_info_frame)
        draw_geoms()
        draw_bbox()

    def draw_map(x, y):
        osm.draw()
        draw_geoms(x, y)
        draw_bbox()

    def draw_geoms(x=None, y=None):
        all_geoms = geoms.copy()
        if curr_geom and x and y:
            latlon = list(osm.canvas_to_latlon(x, y))

            g = curr_geom.copy()
            g.append(latlon)

            if drawing_bbox:
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
                    all_geoms.extend(["bbox", [s, n, w, e]])
            elif g:
                if prev_xy:
                    latlon[1] = adjust_lon(prev_xy[0], x,
                                           curr_geom[len(curr_geom)-1][1],
                                           latlon[1])
                g.append(latlon)
                all_geoms.extend(["poly", g])

        map_canvas.delete(tag_geoms)

        point_half_size = point_size // 2
        geom_type = "point"
        g = 0
        ngeoms = len(all_geoms)
        while g < ngeoms:
            geom = all_geoms[g]
            if geom in ("point", "poly", "bbox"):
                geom_type = geom
                g += 1
                geom = all_geoms[g]
            if type(geom) == list:
                if geom_type == "point":
                    for xy in osm.get_xy([geom]):
                        x, y = xy[0]
                        oval = (x - point_half_size, y - point_half_size,
                                x + point_half_size, y + point_half_size)
                        map_canvas.create_oval(oval, outline=geoms_color,
                                               width=line_width,
                                               fill=geoms_color, tag=tag_geoms)
                elif geom_type == "poly":
                    for xy in osm.get_xy(geom):
                        map_canvas.create_polygon(xy, outline=geoms_color,
                                                  width=line_width,
                                                  fill=geoms_color,
                                                  stipple=fill_stipple,
                                                  tag=tag_geoms)
                else:
                    for xy in osm.get_bbox_xy(geom):
                        map_canvas.create_rectangle(xy, outline=geoms_color,
                                                    width=line_width,
                                                    fill=geoms_color,
                                                    stipple=fill_stipple,
                                                    tag=tag_geoms)
            g += 1

    def draw_bbox():
        map_canvas.delete(tag_bbox)
        for xy in osm.get_bbox_xy(sel_bbox):
            map_canvas.create_rectangle(xy, outline=sel_bbox_color,
                                        width=line_width, fill=sel_bbox_color,
                                        stipple=fill_stipple, tag=tag_bbox)

    def zoom_map(x, y, dz, state):
        def zoom(x, y, dz, cancel_event):
            if not cancel_event.wait(0.01) and osm.redownload():
                # ready to draw map; how? event_generate()? maybe...
                # https://www.tcl.tk/man/tcl8.7/TkCmd/event.html#M7
                # https://mail.python.org/pipermail/python-list/2003-December/197985.html
                # https://bytes.com/topic/python/answers/735362-tkinter-threads-communication
                # however...
                # http://stupidpythonideas.blogspot.com/2013/10/why-your-gui-app-freezes.html?view=classic
                # https://bugs.python.org/issue33412
                # when cancel_event is set after cancel_event.wait() but before
                # event_generate(), event_generate() hangs and zoomer.join()
                # never returns; I tried cancel_event.wait() here again, but it
                # still hung sometimes with when=tail
                # map_canvas.event_generate("<<Zoomed>>", when="tail")

                # let's use queue and after_idle()
                zoomer_queue.put((draw_map, x, y))

        def check_zoomer():
            nonlocal zoomer

            try:
                draw_map, x, y = zoomer_queue.get_nowait()
            except:
                zoomer.checker = map_canvas.after_idle(check_zoomer)
            else:
                draw_map(x, y)

        # https://stackoverflow.com/a/63305873/16079666
        # https://stackoverflow.com/a/26703844/16079666
        # https://wiki.tcl-lang.org/page/Tcl+event+loop
        # first, I tried multi-threading in the OpenStreetMap class, but
        # map_canvas flickered too much; according to the above references,
        # tkinter doesn't like threading, so I decided to use its native
        # after() and after_cancel() to keep only the last zooming event;
        # however, once osm.zoom() started, there was no way to cancel
        # downloading, so I separated GUI code from download code in the
        # OpenStreetMap class and split the draw_map() function into
        # download_map() and draw_map()
        nonlocal zoomer

        if state & 0x4:
            # Control + MouseWheel
            if dz > 0:
                geoms_bbox = calc_geoms_bbox(geoms)
                if None not in geoms_bbox:
                    osm.zoom_to_bbox(geoms_bbox, False)
            else:
                osm.zoom(x, y, osm.z_min - osm.z, False)
            draw_map(x, y)
            return

        if zoomer:
            # cancel the previous zoomer thread
            zoomer.cancel_event.set()

            # cancel the previous map download; why do I set it here instead of
            # inside the zoommer like "if cancel_event, osm.cancel = True else
            # osm.zoom"? if it's inside the zoomer, unless timeout is long
            # enough to wait until another consecutive zoom event occurs,
            # osm.zoom() in the else block will start and once it started,
            # osm.cancel = True in the if block will never happen because the
            # zoomer runs only once; in other words, osm.zoom() that already
            # started will never see osm.cancel = True; to avoid this problem,
            # osm.cancel is set outside the zoomer
            osm.cancel = True
            zoomer.join()
            osm.cancel = False
            map_canvas.after_cancel(zoomer.checker)

            cancel_event = zoomer.cancel_event
            # need to clear to reuse it
            cancel_event.clear()
        else:
            cancel_event = threading.Event()

        osm.rescale(x, y, dz)
        zoomer = threading.Thread(target=zoom, args=(x, y, dz, cancel_event))
        zoomer.cancel_event = cancel_event
        zoomer.checker = map_canvas.after_idle(check_zoomer)
        zoomer.start()

    def import_query():
        try:
            f = filedialog.askopenfile(title="Import query",
                                       filetypes=file_types)
            if f:
                query_text.delete("1.0", tk.END)
                query_text.insert(tk.INSERT, f.read())
                f.close()
        except Exception as e:
            messagebox.showerror("Import query error", e)

    def export_query():
        try:
            f = filedialog.asksaveasfile(title="Export query",
                                         filetypes=file_types)
            if f:
                # https://stackoverflow.com/a/66753640/16079666
                # text widget always appends a new line at the end
                query_to_export = query_text.get("1.0", "end-1c")
                f.write(query_to_export)
                f.close()
        except Exception as e:
            messagebox.showerror("Export query error", e)

    def query():
        nonlocal bbox

        query = query_text.get("1.0", tk.END)
        geoms.clear()
        log_text.delete("1.0", tk.END)
        try:
            geoms.extend(ppik.parse_mixed_geoms(query))
            bbox = ppik.query_mixed_geoms(geoms, projpicker_db)
        except Exception as e:
            log_text.insert(tk.END, e)
            notebook.select(log_frame)

        populate_crs_list(bbox)
        search()
        draw_geoms()

    def populate_crs_list(bbox):
        crs_treeview.delete(*crs_treeview.get_children())
        for b in bbox:
            crs_treeview.insert("", tk.END, values=(
                                f"{b.crs_auth_name}:{b.crs_code}", b.crs_name))
        sel_bbox.clear()
        draw_bbox()

    def search():
        text = [x.strip() for x in search_text.get().split(";")]
        if "".join(text):
            filt_bbox = ppik.search_bbox(bbox, text)
            populate_crs_list(filt_bbox)
            prev_crs_items.clear()

    def select():
        nonlocal sel_crs

        for item in crs_treeview.selection():
            sel_crs.append(crs_treeview.item(item)["values"][0])
        root.destroy()

    # parse geometries if given
    if geoms:
        geoms, query_string = parse_geoms(geoms)
        bbox = ppik.query_mixed_geoms(geoms, projpicker_db)
    else:
        geoms = []
        query_string = ""

    if bbox_or_quit and not bbox:
        return [], bbox, geoms

    #####
    # GUI

    # root window
    root = tk.Tk(className="ProjPickerGUI")
    root_width = 800
    root_height = root_width
    root.geometry(f"{root_width}x{root_height}")
    root.title("ProjPicker tkGUI")
    # https://stackoverflow.com/a/5871414/16079666
    root.bind_class("Text", "<Control-a>",
                    lambda e: e.widget.tag_add(tk.SEL, "1.0", tk.END))


    ###########
    # top frame
    map_canvas_width = root_width
    map_canvas_height = root_height // 2

    map_canvas = tk.Canvas(root, height=map_canvas_height)
    map_canvas.pack(fill=tk.BOTH)

    osm = OpenStreetMap(
            lambda width, height: map_canvas.delete(tag_map),
            lambda image: map_canvas.tag_lower(tag_map),
            lambda data: tk.PhotoImage(data=data),
            lambda image, tile, x, y:
                map_canvas.create_image(x, y, anchor=tk.NW, image=tile,
                                        tag=tag_map),
            lambda tile, dz: tile.zoom(2**abs(dz)) if dz > 0 else
                             tile.subsample(2**abs(dz)),
            map_canvas_width, map_canvas_height,
            lat, lon, zoom, ppik.is_verbose())

    map_canvas.bind("<ButtonPress-1>", lambda e: osm.grab(e.x, e.y))
    map_canvas.bind("<B1-Motion>", on_drag)
    # Linux
    # https://anzeljg.github.io/rin2/book2/2405/docs/tkinter/event-types.html
    map_canvas.bind("<Button-4>", lambda e: zoom_map(e.x, e.y, dzoom, e.state))
    map_canvas.bind("<Button-5>", lambda e: zoom_map(e.x, e.y, -dzoom,
                                                     e.state))
    # Windows and macOS
    # https://anzeljg.github.io/rin2/book2/2405/docs/tkinter/event-types.html
    map_canvas.bind("<MouseWheel>",
                    lambda e: zoom_map(e.x, e.y,
                                       dzoom if e.delta > 0 else -dzoom,
                                       e.state))
    map_canvas.bind("<Motion>", on_move)
    map_canvas.bind("<ButtonRelease-1>", on_draw)
    map_canvas.bind("<Double-Button-1>", on_complete_drawing)
    map_canvas.bind("<ButtonRelease-3>", on_cancel_drawing)
    map_canvas.bind("<Double-Button-3>", lambda e: geoms.clear())
    map_canvas.bind("<Configure>", on_resize)

    # draw geometries if given
    if geoms:
        geoms_bbox = calc_geoms_bbox(geoms)
        if None not in geoms_bbox:
            osm.zoom_to_bbox(geoms_bbox)
        draw_geoms()

    ##############
    # bottom frame
    bottom_frame_height = root_height - map_canvas_height
    bottom_frame = ttk.Frame(root, height=bottom_frame_height)
    bottom_frame.pack_propagate(False)
    bottom_frame.pack(side=tk.BOTTOM, fill=tk.BOTH, expand=True)

    ###################
    # bottom-left frame
    bottom_left_frame_width = root_width // 2
    bottom_left_frame = ttk.Frame(bottom_frame, width=bottom_left_frame_width)
    bottom_left_frame.pack_propagate(False)
    bottom_left_frame.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)

    #######################
    # bottom-left-top frame
    bottom_left_top_frame = ttk.Frame(bottom_left_frame)
    bottom_left_top_frame.pack(fill=tk.BOTH, expand=True)

    # list of CRSs
    id_width = 100
    name_width = bottom_left_frame_width - id_width - 15
    crs_cols = {"ID": id_width, "Name": name_width}

    crs_treeview = ttk.Treeview(
            bottom_left_top_frame, columns=list(crs_cols.keys()),
            show="headings", selectmode=tk.BROWSE if single else tk.EXTENDED)
    for name, width in crs_cols.items():
        crs_treeview.heading(name, text=name)
        crs_treeview.column(name, width=width)

    populate_crs_list(bbox)

    crs_treeview.bind("<<TreeviewSelect>>", on_select_crs)
    crs_treeview.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)

    # vertical scroll bar for CRS list
    crs_treeview_vscrollbar = AutoScrollbar(bottom_left_top_frame)
    crs_treeview_vscrollbar.config(command=crs_treeview.yview)
    crs_treeview_vscrollbar.pack(side=tk.LEFT, fill=tk.Y)
    crs_treeview.config(yscrollcommand=crs_treeview_vscrollbar.set)

    ##########################
    # bottom-left-middle frame

    # horizontal scroll bar for CRS list
    crs_treeview_hscrollbar = AutoScrollbar(bottom_left_frame,
                                        orient=tk.HORIZONTAL)
    crs_treeview_hscrollbar.config(command=crs_treeview.xview)
    crs_treeview_hscrollbar.pack(fill=tk.X)
    crs_treeview.config(xscrollcommand=crs_treeview_hscrollbar.set)

    # text for search
    search_text = tk.Entry(bottom_left_frame)
    search_text.pack(fill=tk.X, ipady=4)
    search_text.bind("<KeyRelease>", lambda e: search())

    ####################
    # bottom-right frame
    notebook_width = root_width - bottom_left_frame_width
    notebook = ttk.Notebook(bottom_frame, width=notebook_width)
    notebook.pack_propagate(False)

    query_frame = ttk.Frame(notebook)
    notebook.add(query_frame, text="Query")

    crs_info_frame = ttk.Frame(notebook)
    notebook.add(crs_info_frame, text="CRS Info")

    log_frame = ttk.Frame(notebook)
    notebook.add(log_frame, text="Log")

    help_frame = ttk.Frame(notebook)
    notebook.add(help_frame, text="Help")

    notebook.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)

    #############
    # query frame
    query_top_frame = ttk.Frame(query_frame)
    query_top_frame.pack(fill=tk.BOTH, expand=True)

    # text for query
    query_text = tk.Text(query_top_frame, width=20, height=1, wrap=tk.NONE)
    query_text.insert(tk.INSERT, query_string)
    query_text.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)

    # pop-up menu
    file_types = (("ProjPicker query files", "*.ppik"), ("All files", "*.*"))
    menu = tk.Menu(query_text, tearoff=0)
    menu.add_command(label="Import query", command=import_query)
    menu.add_command(label="Export query", command=export_query)
    query_text.bind("<Button-3>", lambda e: menu.post(e.x_root, e.y_root))
    # left click or escape anywhere to close the menu
    root.bind("<Button-1>", lambda e: menu.unpost())
    root.bind("<Escape>", lambda e: menu.unpost())

    # vertical scroll bar for query
    query_vscrollbar = AutoScrollbar(query_top_frame)
    query_vscrollbar.config(command=query_text.yview)
    query_vscrollbar.pack(side=tk.LEFT, fill=tk.Y)
    query_text.config(yscrollcommand=query_vscrollbar.set)

    # horizontal scroll bar for query
    query_hscrollbar = AutoScrollbar(query_frame, orient=tk.HORIZONTAL)
    query_hscrollbar.config(command=query_text.xview)
    query_hscrollbar.pack(fill=tk.X)
    query_text.config(xscrollcommand=query_hscrollbar.set)

    query_bottom_frame = ttk.Frame(query_frame)
    query_bottom_frame.pack(fill=tk.BOTH)

    # buttons
    ttk.Button(query_bottom_frame, text="Query", command=query).pack(
               side=tk.LEFT, expand=True)
    ttk.Button(query_bottom_frame, text="Cancel", command=root.destroy).pack(
               side=tk.LEFT, expand=True)

    ################
    # CRS info frame

    # text for CRS info
    crs_info_text = tk.Text(crs_info_frame, width=20, height=1, wrap=tk.NONE)
    crs_info_text.bind("<Key>", lambda e: "break" if e.state == 0 else None)
    crs_info_text.pack(fill=tk.BOTH, expand=True)

    # horizontal scroll bar for CRS info
    crs_info_hscrollbar = AutoScrollbar(crs_info_frame, orient=tk.HORIZONTAL)
    crs_info_hscrollbar.config(command=crs_info_text.xview)
    crs_info_hscrollbar.pack(fill=tk.X)
    crs_info_text.config(xscrollcommand=crs_info_hscrollbar.set)

    crs_info_bottom_frame = ttk.Frame(crs_info_frame)
    crs_info_bottom_frame.pack(fill=tk.BOTH)

    # buttons
    ttk.Button(crs_info_bottom_frame, text="Select", command=select).pack(
               side=tk.LEFT, expand=True)
    ttk.Button(crs_info_bottom_frame, text="Cancel",
               command=root.destroy).pack(side=tk.LEFT, expand=True)


    ###########
    # log frame
    log_top_frame = ttk.Frame(log_frame)
    log_top_frame.pack(fill=tk.BOTH, expand=True)

    # text for log
    log_text = tk.Text(log_top_frame, width=20, height=1, wrap=tk.NONE)
    log_text.bind("<Key>", lambda e: "break" if e.state == 0 else None)
    log_text.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)

    # vertical scroll bar for log
    log_vscrollbar = AutoScrollbar(log_top_frame)
    log_vscrollbar.config(command=log_text.yview)
    log_vscrollbar.pack(side=tk.LEFT, fill=tk.Y)
    log_text.config(yscrollcommand=log_vscrollbar.set)

    # horizontal scroll bar for log
    log_hscrollbar = AutoScrollbar(log_frame, orient=tk.HORIZONTAL)
    log_hscrollbar.config(command=log_text.xview)
    log_hscrollbar.pack(fill=tk.X)
    log_text.config(xscrollcommand=log_hscrollbar.set)

    ############
    # help frame

    # text for help
    help_text = tk.Text(help_frame, width=20, height=1, wrap=tk.NONE)
    help_text.insert(tk.END, textwrap.dedent(f"""\
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

            See {doc_url} to learn more."""))
    help_text.tag_add(tag_doc, "end-1l+4c", "end-16c")
    help_text.tag_config(tag_doc, foreground="blue", underline=True)
    help_text.tag_bind(tag_doc, "<Enter>",
                       lambda e: help_text.config(cursor="hand2"))
    help_text.tag_bind(tag_doc, "<Leave>",
                       lambda e: help_text.config(cursor=""))
    help_text.tag_bind(tag_doc, "<Button-1>",
                       lambda e: webbrowser.open(doc_url))
    help_text.config(state=tk.DISABLED)
    help_text.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)

    help_vscrollbar = AutoScrollbar(help_frame)
    help_vscrollbar.config(command=help_text.yview)
    help_vscrollbar.pack(side=tk.RIGHT, fill=tk.Y)
    help_text.config(yscrollcommand=help_vscrollbar.set)

    # label for coordinates
    coor_label = ttk.Label(notebook)
    coor_label.place(relx=1, rely=0, anchor=tk.NE)

    #########
    # run GUI
    root.mainloop()

    return [find_bbox(crs_id, bbox) for crs_id in sel_crs], bbox, geoms
