"""!
@package iscatt.plots

@brief Ploting widgets.

Classes:
 - plots::ScatterPlotWidget
 - plots::PolygonDrawer
 - plots::ModestImage
 
(C) 2013 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Stepan Turek <stepan.turek seznam.cz> (mentor: Martin Landa)
"""
import wx
import numpy as np
from math import ceil
from multiprocessing import Process, Queue

from copy import deepcopy
from iscatt.core_c import MergeArrays, ApplyColormap
from iscatt.dialogs import ManageBusyCursorMixin
from core.settings import UserSettings

try:
    import matplotlib
    matplotlib.use('WXAgg')
    from matplotlib.figure import Figure
    from matplotlib.backends.backend_wxagg import \
    FigureCanvasWxAgg as FigCanvas
    from matplotlib.lines import Line2D
    from matplotlib.artist import Artist
    from matplotlib.mlab import dist_point_to_segment
    from matplotlib.patches import Polygon, Ellipse, Rectangle
    import matplotlib.image as mi
    import matplotlib.colors as mcolors
    import matplotlib.cbook as cbook
except ImportError as e:
    raise ImportError(_("Unable to import matplotlib (try to install it).\n%s") % e)

import grass.script as grass
from grass.pydispatch.signal import Signal

class ScatterPlotWidget(wx.Panel, ManageBusyCursorMixin):
    def __init__(self, parent, scatt_id, scatt_mgr, transpose,
                 id = wx.ID_ANY):
        #TODO should not be transpose and scatt_id but x, y
        wx.Panel.__init__(self, parent, id)
        # bacause of aui (if floatable it can not take cursor from parent)        
        ManageBusyCursorMixin.__init__(self, window=self)

        self.parent = parent
        self.full_extend = None
        self.mode = None

        self._createWidgets()
        self._doLayout()
        self.scatt_id = scatt_id
        self.scatt_mgr = scatt_mgr

        self.cidpress = None
        self.cidrelease = None

        self.rend_dt = {}

        self.transpose = transpose

        self.inverse = False

        self.SetSize((200, 100))
        self.Layout()

        self.base_scale = 1.2
        self.Bind(wx.EVT_CLOSE,lambda event : self.CleanUp())

        self.plotClosed = Signal("ScatterPlotWidget.plotClosed")
        self.cursorMove = Signal("ScatterPlotWidget.cursorMove")

        self.contex_menu = ScatterPlotContextMenu(plot = self)

        self.ciddscroll = None

        self.canvas.mpl_connect('motion_notify_event', self.Motion)
        self.canvas.mpl_connect('button_press_event', self.OnPress)
        self.canvas.mpl_connect('button_release_event', self.OnRelease)
        self.canvas.mpl_connect('draw_event', self.DrawCallback)
        self.canvas.mpl_connect('figure_leave_event', self.OnCanvasLeave)

    def DrawCallback(self, event):
        self.polygon_drawer.DrawCallback(event)
        self.axes.draw_artist(self.zoom_rect)

    def _createWidgets(self):

        # Create the mpl Figure and FigCanvas objects. 
        # 5x4 inches, 100 dots-per-inch
        #
        self.dpi = 100
        self.fig = Figure((1.0, 1.0), dpi=self.dpi)
        self.fig.autolayout = True

        self.canvas = FigCanvas(self, -1, self.fig)
        
        self.axes = self.fig.add_axes([0.0,0.0,1,1])

        pol = Polygon(list(zip([0], [0])), animated=True)
        self.axes.add_patch(pol)
        self.polygon_drawer = PolygonDrawer(self.axes, pol = pol, empty_pol = True)

        self.zoom_wheel_coords = None
        self.zoom_rect_coords = None
        self.zoom_rect = Polygon(list(zip([0], [0])), facecolor = 'none')
        self.zoom_rect.set_visible(False)
        self.axes.add_patch(self.zoom_rect)

    def ZoomToExtend(self):
        if self.full_extend:
            self.axes.axis(self.full_extend)
            self.canvas.draw()

    def SetMode(self, mode):
        self._deactivateMode()
        if mode == 'zoom':
            self.ciddscroll = self.canvas.mpl_connect('scroll_event', self.ZoomWheel)
            self.mode = 'zoom'
        elif mode == 'zoom_extend':
            self.mode = 'zoom_extend'
        elif mode == 'pan':
            self.mode = 'pan'
        elif mode:
            self.polygon_drawer.SetMode(mode)

    def SetSelectionPolygonMode(self, activate):
        self.polygon_drawer.SetSelectionPolygonMode(activate)

    def _deactivateMode(self):
        self.mode  = None
        self.polygon_drawer.SetMode(None)

        if self.ciddscroll:
            self.canvas.mpl_disconnect(self.ciddscroll)

        self.zoom_rect.set_visible(False)
        self._stopCategoryEdit()

    def GetCoords(self):

        coords = self.polygon_drawer.GetCoords()
        if coords is None:
            return

        if self.transpose:
            for c in coords:
                tmp = c[0]
                c[0] = c[1]
                c[1] = tmp

        return coords

    def SetEmpty(self):
        return self.polygon_drawer.SetEmpty()

    def OnRelease(self, event):
        if not self.mode == "zoom": return
        self.zoom_rect.set_visible(False)
        self.ZoomRectangle(event)
        self.canvas.draw()
    
    def OnPress(self, event):
        'on button press we will see if the mouse is over us and store some data'
        if not event.inaxes:
            return
        if self.mode == "zoom_extend":
            self.ZoomToExtend()

        if event.xdata and event.ydata:
            self.zoom_wheel_coords = { 'x' : event.xdata, 'y' : event.ydata}
            self.zoom_rect_coords = { 'x' : event.xdata, 'y' : event.ydata}
        else:
            self.zoom_wheel_coords = None
            self.zoom_rect_coords = None

    def _stopCategoryEdit(self):
        'disconnect all the stored connection ids'

        if self.cidpress:
            self.canvas.mpl_disconnect(self.cidpress)
        if self.cidrelease:
            self.canvas.mpl_disconnect(self.cidrelease)
        #self.canvas.mpl_disconnect(self.cidmotion)

    def _doLayout(self):
        
        self.main_sizer = wx.BoxSizer(wx.VERTICAL)
        self.main_sizer.Add(self.canvas, 1, wx.LEFT | wx.TOP | wx.GROW)
        self.SetSizer(self.main_sizer)
        self.main_sizer.Fit(self)
    
    def Plot(self, cats_order, scatts, ellipses, styles):
        """ Redraws the figure
        """

        callafter_list = []

        if self.full_extend:
            cx = self.axes.get_xlim()
            cy = self.axes.get_ylim()
            c = cx + cy
        else:
            c = None

        q = Queue()
        p = Process(target=MergeImg, args=(cats_order, scatts, styles, 
                                           self.rend_dt, q))
        p.start()
        merged_img, self.full_extend, self.rend_dt = q.get()
        p.join()
        
        #merged_img, self.full_extend = MergeImg(cats_order, scatts, styles, None)
        self.axes.clear()
        self.axes.axis('equal')

        if self.transpose:
            merged_img = np.transpose(merged_img, (1, 0, 2))

        img = imshow(self.axes, merged_img,
                     extent= [int(ceil(x)) for x in self.full_extend],
                     origin='lower',
                     interpolation='nearest',
                     aspect="equal")

        callafter_list.append([self.axes.draw_artist, [img]])
        callafter_list.append([grass.try_remove, [merged_img.filename]])

        for cat_id in cats_order:
            if cat_id == 0:
                continue
            if not ellipses.has_key(cat_id):
                continue
                
            e = ellipses[cat_id]
            if not e:
                continue

            colors = styles[cat_id]['color'].split(":")
            if self.transpose:
                e['theta'] = 360 - e['theta'] + 90
                if e['theta'] >= 360:
                    e['theta'] = abs(360 - e['theta']) 
                
                e['pos'] = [e['pos'][1], e['pos'][0]]

            ellip = Ellipse(xy=e['pos'], 
                            width=e['width'], 
                            height=e['height'], 
                            angle=e['theta'], 
                            edgecolor="w",
                            linewidth=1.5, 
                            facecolor='None')
            self.axes.add_artist(ellip)
            callafter_list.append([self.axes.draw_artist, [ellip]])

            color = map(lambda v : int(v)/255.0, styles[cat_id]['color'].split(":"))

            ellip = Ellipse(xy=e['pos'], 
                            width=e['width'], 
                            height=e['height'], 
                            angle=e['theta'], 
                            edgecolor=color,
                            linewidth=1, 
                            facecolor='None')

            self.axes.add_artist(ellip)
            callafter_list.append([self.axes.draw_artist, [ellip]])
            
            center = Line2D([e['pos'][0]], [e['pos'][1]], 
                            marker='x',
                            markeredgecolor='w',
                            #markerfacecolor=color,
                            markersize=2)
            self.axes.add_artist(center)
            callafter_list.append([self.axes.draw_artist, [center]])

        callafter_list.append([self.fig.canvas.blit, []])

        if c:
            self.axes.axis(c)
        wx.CallAfter(lambda : self.CallAfter(callafter_list))
    
    def CallAfter(self, funcs_list):
        while funcs_list: 
            fcn, args = funcs_list.pop(0) 
            fcn(*args) 

        self.canvas.draw()

    def CleanUp(self):
        self.plotClosed.emit(scatt_id = self.scatt_id)
        self.Destroy()

    def ZoomWheel(self, event):
        # get the current x and y limits
        if not event.inaxes:
            return
        # tcaswell
        # http://stackoverflow.com/questions/11551049/matplotlib-plot-zooming-with-scroll-wheel
        cur_xlim = self.axes.get_xlim()
        cur_ylim = self.axes.get_ylim()
        
        xdata = event.xdata
        ydata = event.ydata 
        if event.button == 'up':
            scale_factor = 1/self.base_scale
        elif event.button == 'down':
            scale_factor = self.base_scale
        else:
            scale_factor = 1

        extend = (xdata - (xdata - cur_xlim[0]) * scale_factor,
                  xdata + (cur_xlim[1] - xdata) * scale_factor, 
                  ydata - (ydata - cur_ylim[0]) * scale_factor,
                  ydata + (cur_ylim[1] - ydata) * scale_factor)

        self.axes.axis(extend)
        
        self.canvas.draw()

    def ZoomRectangle(self, event):
        # get the current x and y limits
        if not self.mode == "zoom": return
        if event.inaxes is None: return
        if event.button != 1: return

        cur_xlim = self.axes.get_xlim()
        cur_ylim = self.axes.get_ylim()
        
        x1, y1 = event.xdata, event.ydata
        x2 = deepcopy(self.zoom_rect_coords['x'])
        y2 = deepcopy(self.zoom_rect_coords['y'])

        if x1 == x2 or y1 == y2:
            return

        self.axes.axis((x1, x2, y1, y2))
        #self.axes.set_xlim(x1, x2)#, auto = True)
        #self.axes.set_ylim(y1, y2)#, auto = True)
        self.canvas.draw()

    def Motion(self, event):
        self.PanMotion(event)
        self.ZoomRectMotion(event)
        
        if event.inaxes is None: 
            return
        
        self.cursorMove.emit(x=event.xdata, y=event.ydata, scatt_id=self.scatt_id)

    def OnCanvasLeave(self, event):
        self.cursorMove.emit(x=None, y=None, scatt_id=self.scatt_id)

    def PanMotion(self, event):
        'on mouse movement'
        if not self.mode == "pan": 
            return
        if event.inaxes is None: 
            return
        if event.button != 1: 
            return

        cur_xlim = self.axes.get_xlim()
        cur_ylim = self.axes.get_ylim()

        x,y = event.xdata, event.ydata
        
        mx = (x - self.zoom_wheel_coords['x']) * 0.6
        my = (y - self.zoom_wheel_coords['y']) * 0.6

        extend = (cur_xlim[0] - mx, cur_xlim[1] - mx, cur_ylim[0] - my, cur_ylim[1] - my)

        self.zoom_wheel_coords['x'] = x
        self.zoom_wheel_coords['y'] = y

        self.axes.axis(extend)

        #self.canvas.copy_from_bbox(self.axes.bbox)
        #self.canvas.restore_region(self.background)
        self.canvas.draw()
        
    def ZoomRectMotion(self, event):
        if not self.mode == "zoom": return
        if event.inaxes is None: return
        if event.button != 1: return

        x1, y1 = event.xdata, event.ydata
        self.zoom_rect.set_visible(True)
        x2 = self.zoom_rect_coords['x']
        y2 = self.zoom_rect_coords['y']

        self.zoom_rect.xy = ((x1, y1), (x1, y2), (x2, y2), (x2, y1), (x1, y1))

        #self.axes.draw_artist(self.zoom_rect)
        self.canvas.draw()

def MergeImg(cats_order, scatts, styles, rend_dt, output_queue):

        init = True
        merged_img = None
        merge_tmp = grass.tempfile()
        for cat_id in cats_order:
            if not scatts.has_key(cat_id):
                continue
            scatt = scatts[cat_id]
            #print "color map %d" % cat_id
            #TODO make more general
            if cat_id != 0 and (styles[cat_id]['opacity'] == 0.0 or \
               not styles[cat_id]['show']):
                if rend_dt.has_key(cat_id) and not rend_dt[cat_id]:
                    del rend_dt[cat_id]
                continue
            if init:

                b2_i = scatt['bands_info']['b1']
                b1_i = scatt['bands_info']['b2']

                full_extend = (b1_i['min'] - 0.5, b1_i['max'] + 0.5, b2_i['min'] - 0.5, b2_i['max'] + 0.5) 
            
            # if it does not need to be updated and was already rendered 
            if not _renderCat(cat_id, rend_dt, scatt, styles):
                # is empty - has only zeros
                if rend_dt[cat_id] is None:
                    continue
            else:
                masked_cat = np.ma.masked_less_equal(scatt['np_vals'], 0)
                vmax = np.amax(masked_cat)
                # totally empty -> no need to render
                if vmax == 0:
                    render_cat_ids[cat_id] = None
                    continue

                cmap = _getColorMap(cat_id, styles)
                masked_cat = np.uint8(masked_cat * (255.0 / float(vmax)))

                cmap = np.uint8(cmap._lut * 255)
                sh =masked_cat.shape

                rend_dt[cat_id] = {}
                if cat_id != 0:
                    rend_dt[cat_id]['color'] = styles[cat_id]['color']
                rend_dt[cat_id]['dt'] = np.memmap(grass.tempfile(), dtype='uint8', mode='w+', 
                                                                    shape=(sh[0], sh[1], 4))
                #colored_cat = np.zeros(dtype='uint8', )
                ApplyColormap(masked_cat, masked_cat.mask, cmap, rend_dt[cat_id]['dt'])

                #colored_cat = np.uint8(cmap(masked_cat) * 255)
                del masked_cat
                del cmap
            
            #colored_cat[...,3] = np.choose(masked_cat.mask, (255, 0))
            if init:
                merged_img = np.memmap(merge_tmp, dtype='uint8', mode='w+', 
                                       shape=rend_dt[cat_id]['dt'].shape)
                merged_img[:] = rend_dt[cat_id]['dt']
                init = False
            else:
                MergeArrays(merged_img, rend_dt[cat_id]['dt'], styles[cat_id]['opacity'])

            """
                #c_img_a = np.memmap(grass.tempfile(), dtype="uint16", mode='w+', shape = shape) 
                c_img_a = colored_cat.astype('uint16')[:,:,3] * styles[cat_id]['opacity']

                #TODO apply strides and there will be no need for loop
                #b = as_strided(a, strides=(0, a.strides[3], a.strides[3], a.strides[3]), shape=(3, a.shape[0], a.shape[1]))
                
                for i in range(3):
                    merged_img[:,:,i] = (merged_img[:,:,i] * (255 - c_img_a) + colored_cat[:,:,i] * c_img_a) / 255;
                merged_img[:,:,3] = (merged_img[:,:,3] * (255 - c_img_a) + 255 * c_img_a) / 255;
                
                del c_img_a
            """

        output_queue.put((merged_img, full_extend, rend_dt))

def _renderCat(cat_id, rend_dt, scatt, styles):
    return True

    if not rend_dt.has_key(cat_id):
        return True
    if not rend_dt[cat_id]:
        return False
    if scatt['render']:
        return True
    if cat_id != 0 and \
       rend_dt[cat_id]['color'] != styles[cat_id]['color']:
       return True
    
    return False

def _getColorMap(cat_id, styles):
    cmap = matplotlib.cm.jet
    if cat_id == 0:
        cmap.set_bad('w',1.)
        cmap._init()
        cmap._lut[len(cmap._lut) - 1, -1] = 0
    else:
        colors = styles[cat_id]['color'].split(":")

        cmap.set_bad('w',1.)
        cmap._init()
        cmap._lut[len(cmap._lut) - 1, -1] = 0
        cmap._lut[:, 0] = int(colors[0])/255.0
        cmap._lut[:, 1] = int(colors[1])/255.0
        cmap._lut[:, 2] = int(colors[2])/255.0

    return cmap

class ScatterPlotContextMenu:
    def __init__(self, plot):

        self.plot = plot
        self.canvas = plot.canvas
        self.cidpress = self.canvas.mpl_connect(
            'button_press_event', self.ContexMenu)
   
    def ContexMenu(self, event):
        if not event.inaxes:
            return

        if event.button == 3:
            menu = wx.Menu()       
            menu_items = [["zoom_to_extend", _("Zoom to scatter plot extend"), 
                            lambda event : self.plot.ZoomToExtend()]]

            for item in menu_items:
                item_id = wx.ID_ANY
                menu.Append(item_id, text = item[1])
                menu.Bind(wx.EVT_MENU, item[2], id = item_id)

            wx.CallAfter(self.ShowMenu, menu) 
   
    def ShowMenu(self, menu):
        self.plot.PopupMenu(menu)
        menu.Destroy()
        self.plot.ReleaseMouse() 

class PolygonDrawer:
    """
    An polygon editor.
    """
    def __init__(self, ax, pol, empty_pol):
        if pol.figure is None:
            raise RuntimeError('You must first add the polygon to a figure or canvas before defining the interactor')
        self.ax = ax
        self.canvas = pol.figure.canvas

        self.showverts = True

        self.pol = pol
        self.empty_pol = empty_pol

        x, y = zip(*self.pol.xy)

        style = self._getPolygonStyle()

        self.line = Line2D(x, y, marker='o', markerfacecolor='r', animated=True)
        self.ax.add_line(self.line)
        #self._update_line(pol)

        cid = self.pol.add_callback(self.poly_changed)
        self.moving_ver_idx = None # the active vert

        self.mode = None

        if self.empty_pol:
            self._show(False)

        #self.canvas.mpl_connect('draw_event', self.DrawCallback)
        self.canvas.mpl_connect('button_press_event', self.OnButtonPressed)
        self.canvas.mpl_connect('button_release_event', self.ButtonReleaseCallback)
        self.canvas.mpl_connect('motion_notify_event', self.motion_notify_callback)
    
        self.it = 0

    def _getPolygonStyle(self):
        style = {}
        style['sel_pol'] = UserSettings.Get(group='scatt', 
                                            key='selection', 
                                            subkey='sel_pol')
        style['sel_pol_vertex'] = UserSettings.Get(group='scatt', 
                                                   key='selection', 
                                                   subkey='sel_pol_vertex')

        style['sel_pol'] = [i / 255.0 for i in style['sel_pol']] 
        style['sel_pol_vertex'] = [i / 255.0 for i in style['sel_pol_vertex']] 

        return style

    def _getSnapTresh(self):
        return UserSettings.Get(group='scatt', 
                                key='selection', 
                                subkey='snap_tresh')

    def SetMode(self, mode):
        self.mode = mode

    def SetSelectionPolygonMode(self, activate):
        
        self.Show(activate)
        if not activate and self.mode:
            self.SetMode(None) 

    def Show(self, show):
        if show:
            if not self.empty_pol:
                self._show(True)
        else:
            self._show(False)

    def GetCoords(self):
        if self.empty_pol:
            return None

        coords = deepcopy(self.pol.xy)
        return coords

    def SetEmpty(self):
        self._setEmptyPol(True)

    def _setEmptyPol(self, empty_pol):
        self.empty_pol = empty_pol
        if self.empty_pol:
            #TODO
            self.pol.xy = np.array([[0, 0]])
        self._show(not empty_pol)

    def _show(self, show):

        self.show = show

        self.line.set_visible(self.show)
        self.pol.set_visible(self.show)

        self.Redraw()

    def Redraw(self):
        if self.show:
            self.ax.draw_artist(self.pol)
            self.ax.draw_artist(self.line)
        self.canvas.blit(self.ax.bbox)
        self.canvas.draw()

    def DrawCallback(self, event):

        style=self._getPolygonStyle()
        self.pol.set_facecolor(style['sel_pol'])
        self.line.set_markerfacecolor(style['sel_pol_vertex'])

        self.background = self.canvas.copy_from_bbox(self.ax.bbox)
        self.ax.draw_artist(self.pol)
        self.ax.draw_artist(self.line)
    
    def poly_changed(self, pol):
        'this method is called whenever the polygon object is called'
        # only copy the artist props to the line (except visibility)
        vis = self.line.get_visible()
        Artist.update_from(self.line, pol)
        self.line.set_visible(vis)  # don't use the pol visibility state

    def get_ind_under_point(self, event):
        'get the index of the vertex under point if within treshold'

        # display coords
        xy = np.asarray(self.pol.xy)
        xyt = self.pol.get_transform().transform(xy)
        xt, yt = xyt[:, 0], xyt[:, 1]
        d = np.sqrt((xt-event.x)**2 + (yt-event.y)**2)
        indseq = np.nonzero(np.equal(d, np.amin(d)))[0]
        ind = indseq[0]

        if d[ind]>=self._getSnapTresh():
            ind = None

        return ind

    def OnButtonPressed(self, event):
        if not event.inaxes:
            return

        if event.button in [2, 3]: 
            return

        if self.mode == "delete_vertex":
            self._deleteVertex(event)
        elif self.mode == "add_boundary_vertex":
            self._addVertexOnBoundary(event)
        elif self.mode == "add_vertex":
            self._addVertex(event)
        elif self.mode == "remove_polygon":
            self.SetEmpty()
        self.moving_ver_idx = self.get_ind_under_point(event)

    def ButtonReleaseCallback(self, event):
        'whenever a mouse button is released'
        if not self.showverts: return
        if event.button != 1: return
        self.moving_ver_idx = None

    def ShowVertices(self, show):
        self.showverts = show
        self.line.set_visible(self.showverts)
        if not self.showverts: self.moving_ver_idx = None

    def _deleteVertex(self, event):
        ind = self.get_ind_under_point(event)

        if ind  is None or self.empty_pol:
            return

        if len(self.pol.xy) <= 2:
            self.empty_pol = True
            self._show(False)
            return

        coords = []
        for i,tup in enumerate(self.pol.xy): 
            if i == ind:
                continue
            elif i == 0 and ind == len(self.pol.xy) - 1:
                continue
            elif i == len(self.pol.xy) - 1 and ind == 0: 
                continue

            coords.append(tup)

        self.pol.xy = coords
        self.line.set_data(zip(*self.pol.xy))

        self.Redraw()

    def _addVertexOnBoundary(self, event):
        if self.empty_pol:
            return

        xys = self.pol.get_transform().transform(self.pol.xy)
        p = event.x, event.y # display coords
        for i in range(len(xys)-1):
            s0 = xys[i]
            s1 = xys[i+1]
            d = dist_point_to_segment(p, s0, s1)

            if d<=self._getSnapTresh():
                self.pol.xy = np.array(
                    list(self.pol.xy[:i + 1]) +
                    [(event.xdata, event.ydata)] +
                    list(self.pol.xy[i + 1:]))
                self.line.set_data(zip(*self.pol.xy))
                break

        self.Redraw()

    def _addVertex(self, event):

        if self.empty_pol:
            pt = (event.xdata, event.ydata)
            self.pol.xy = np.array([pt, pt])
            self._show(True)
            self.empty_pol = False
        else:
            self.pol.xy = np.array(
                        [(event.xdata, event.ydata)] +
                        list(self.pol.xy[1:]) +
                        [(event.xdata, event.ydata)])

        self.line.set_data(zip(*self.pol.xy))
        
        self.Redraw()

    def motion_notify_callback(self, event):
        'on mouse movement'
        if not self.mode == "move_vertex": return
        if not self.showverts: return
        if self.empty_pol: return
        if self.moving_ver_idx is None: return
        if event.inaxes is None: return
        if event.button != 1: return

        self.it += 1

        x,y = event.xdata, event.ydata

        self.pol.xy[self.moving_ver_idx] = x,y
        if self.moving_ver_idx == 0:
            self.pol.xy[len(self.pol.xy) - 1] = x,y
        elif self.moving_ver_idx == len(self.pol.xy) - 1:
            self.pol.xy[0] = x,y

        self.line.set_data(zip(*self.pol.xy))

        self.canvas.restore_region(self.background)

        self.Redraw()

class ModestImage(mi.AxesImage):
    """
    Computationally modest image class.

    ModestImage is an extension of the Matplotlib AxesImage class
    better suited for the interactive display of larger images. Before
    drawing, ModestImage resamples the data array based on the screen
    resolution and view window. This has very little affect on the
    appearance of the image, but can substantially cut down on
    computation since calculations of unresolved or clipped pixels
    are skipped.

    The interface of ModestImage is the same as AxesImage. However, it
    does not currently support setting the 'extent' property. There
    may also be weird coordinate warping operations for images that
    I'm not aware of. Don't expect those to work either.

    Author: Chris Beaumont <beaumont@hawaii.edu>
    """
    def __init__(self, minx=0.0, miny=0.0, *args, **kwargs):
        if 'extent' in kwargs and kwargs['extent'] is not None:
            raise NotImplementedError("ModestImage does not support extents")

        self._full_res = None
        self._sx, self._sy = None, None
        self._bounds = (None, None, None, None)
        self.minx = minx
        self.miny = miny

        super(ModestImage, self).__init__(*args, **kwargs)

    def set_data(self, A):
        """
        Set the image array

        ACCEPTS: numpy/PIL Image A
        """
        self._full_res = A
        self._A = A

        if self._A.dtype != np.uint8 and not np.can_cast(self._A.dtype,
                                                         np.float):
            raise TypeError("Image data can not convert to float")

        if (self._A.ndim not in (2, 3) or
            (self._A.ndim == 3 and self._A.shape[-1] not in (3, 4))):
            raise TypeError("Invalid dimensions for image data")

        self._imcache =None
        self._rgbacache = None
        self._oldxslice = None
        self._oldyslice = None
        self._sx, self._sy = None, None

    def get_array(self):
        """Override to return the full-resolution array"""
        return self._full_res

    def _scale_to_res(self):
        """ Change self._A and _extent to render an image whose
        resolution is matched to the eventual rendering."""

        ax = self.axes
        ext = ax.transAxes.transform([1, 1]) - ax.transAxes.transform([0, 0])
        xlim, ylim = ax.get_xlim(), ax.get_ylim()
        dx, dy = xlim[1] - xlim[0], ylim[1] - ylim[0]

        y0 = max(self.miny, ylim[0] - 5)
        y1 = min(self._full_res.shape[0] + self.miny, ylim[1] + 5)
        x0 = max(self.minx, xlim[0] - 5)
        x1 = min(self._full_res.shape[1] + self.minx, xlim[1] + 5)
        y0, y1, x0, x1 = map(int, [y0, y1, x0, x1])

        sy = int(max(1, min((y1 - y0) / 5., np.ceil(dy / ext[1]))))
        sx = int(max(1, min((x1 - x0) / 5., np.ceil(dx / ext[0]))))

        # have we already calculated what we need?
        if sx == self._sx and sy == self._sy and \
            x0 == self._bounds[0] and x1 == self._bounds[1] and \
            y0 == self._bounds[2] and y1 == self._bounds[3]:
            return

        self._A = self._full_res[y0 - self.miny:y1 - self.miny:sy, 
                                 x0 - self.minx:x1 - self.minx:sx]

        x1 = x0 + self._A.shape[1] * sx
        y1 = y0 + self._A.shape[0] * sy

        self.set_extent([x0 - .5, x1 - .5, y0 - .5, y1 - .5])
        self._sx = sx
        self._sy = sy
        self._bounds = (x0, x1, y0, y1)
        self.changed()

    def draw(self, renderer, *args, **kwargs):
        self._scale_to_res()
        super(ModestImage, self).draw(renderer, *args, **kwargs)

def imshow(axes, X, cmap=None, norm=None, aspect=None,
           interpolation=None, alpha=None, vmin=None, vmax=None,
           origin=None, extent=None, shape=None, filternorm=1,
           filterrad=4.0, imlim=None, resample=None, url=None, **kwargs):
    """Similar to matplotlib's imshow command, but produces a ModestImage

    Unlike matplotlib version, must explicitly specify axes
    Author: Chris Beaumont <beaumont@hawaii.edu>
    """

    if not axes._hold:
        axes.cla()
    if norm is not None:
        assert(isinstance(norm, mcolors.Normalize))
    if aspect is None:
        aspect = rcParams['image.aspect']
    axes.set_aspect(aspect)

    if extent:
        minx=extent[0]
        miny=extent[2]
    else:
        minx=0.0
        miny=0.0

    im = ModestImage(minx, miny, axes, cmap, norm, interpolation, origin, extent,
                     filternorm=filternorm,
                     filterrad=filterrad, resample=resample, **kwargs)

    im.set_data(X)
    im.set_alpha(alpha)
    axes._set_artist_props(im)

    if im.get_clip_path() is None:
        # image does not already have clipping set, clip to axes patch
        im.set_clip_path(axes.patch)

    #if norm is None and shape is None:
    #    im.set_clim(vmin, vmax)
    if vmin is not None or vmax is not None:
        im.set_clim(vmin, vmax)
    else:
        im.autoscale_None()
    im.set_url(url)

    # update ax.dataLim, and, if autoscaling, set viewLim
    # to tightly fit the image, regardless of dataLim.
    im.set_extent(im.get_extent())

    axes.images.append(im)
    im._remove_method = lambda h: axes.images.remove(h)

    return im
