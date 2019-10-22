#!/usr/bin/env python3

############################################################################
#
# MODULE:       d.rast.edit
# AUTHOR(S):    Glynn Clements <glynn@gclements.plus.com>
# COPYRIGHT:    (C) 2007,2008,2009 Glynn Clements
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
# /

#%module
#% description: Edits cell values in a raster map.
#% keyword: display
#% keyword: editing
#% keyword: raster
#%end
#%option G_OPT_R_INPUT
#%end
#%option G_OPT_R_OUTPUT
#%end
#%option G_OPT_R_INPUT
#% key: aspect
#% required: no
#% description: Name of input aspect raster map
#%end
#%option
#% key: width
#% type: integer
#% required: no
#% multiple: no
#% description: Width of display canvas
#% answer: 640
#%end
#%option
#% key: height
#% type: integer
#% required: no
#% multiple: no
#% description: Height of display canvas
#% answer: 480
#%end
#%option
#% key: size
#% type: integer
#% required: no
#% multiple: no
#% description: Minimum size of each cell
#% answer: 12
#%end
#%option
#% key: rows
#% type: integer
#% required: no
#% multiple: no
#% description: Maximum number of rows to load
#% answer: 100
#%end
#%option
#% key: cols
#% type: integer
#% required: no
#% multiple: no
#% description: Maximum number of columns to load
#% answer: 100
#%end

import sys
import math
import atexit
import grass.script as grass

try:
    import wxversion
    wxversion.select(['3.0', '2.8', '2.6'])
    import wx
except Exception:
    # ensure that --help, --interface-description etc work even without wx
    if __name__ == "__main__":
        if len(sys.argv) == 2:
            arg = sys.argv[1]
            if arg[0:2] == '--' or arg in ["help", "-help"]:
                grass.parser()
    # Either we didn't call g.parser, or it returned
    # At this point, there's nothing to be done except re-raise the exception
    raise

wind_keys = {
    'north': ('n', float),
    'south': ('s', float),
    'east': ('e', float),
    'west': ('w', float),
    'nsres': ('nsres', float),
    'ewres': ('ewres', float),
    'rows': ('rows', int),
    'cols': ('cols', int),
}

gray12_bits = '\x00\x00\x22\x22\x00\x00\x88\x88\x00\x00\x22\x22\x00\x00\x88\x88\x00\x00\x22\x22\x00\x00\x88\x88\x00\x00\x22\x22\x00\x00\x88\x88'


def run(cmd, **kwargs):
    grass.run_command(cmd, quiet=True, **kwargs)


class OverviewCanvas(wx.ScrolledWindow):

    def __init__(self, app, parent):
        wx.ScrolledWindow.__init__(self, parent)
        self.app = app

        self.width = app.total['cols']
        self.height = app.total['rows']

        self.SetVirtualSize((self.width, self.height))
        self.SetScrollRate(1, 1)

        self.Bind(wx.EVT_LEFT_DOWN, self.OnMouse)
        self.Bind(wx.EVT_MOTION, self.OnMouse)
        self.Bind(wx.EVT_LEFT_UP, self.OnMouse)
        self.Bind(wx.EVT_PAINT, self.OnPaint)

        run('r.out.ppm', input=app.inmap, output=app.tempfile)

        self.image = wx.BitmapFromImage(wx.Image(app.tempfile))
        grass.try_remove(app.tempfile)

        app.force_window()

    def OnPaint(self, ev):
        x0 = self.app.origin_x
        y0 = self.app.origin_y
        dx = self.app.cols
        dy = self.app.rows

        dc = wx.PaintDC(self)
        self.DoPrepareDC(dc)

        src = wx.MemoryDC()
        src.SelectObjectAsSource(self.image)
        dc.Blit(0, 0, self.width, self.height, src, 0, 0)
        src.SelectObjectAsSource(wx.NullBitmap)

        dc.SetPen(wx.Pen('red', style=wx.LONG_DASH))
        dc.SetBrush(wx.Brush('black', style=wx.TRANSPARENT))
        dc.DrawRectangle(x0, y0, dx, dy)
        dc.SetBrush(wx.NullBrush)
        dc.SetPen(wx.NullPen)

    def OnMouse(self, ev):
        if ev.Moving():
            return
        x = ev.GetX()
        y = ev.GetY()
        (x, y) = self.CalcUnscrolledPosition(x, y)
        self.set_window(x, y)
        if ev.ButtonUp():
            self.app.change_window()

    def set_window(self, x, y):
        self.app.origin_x = x - app.cols / 2
        self.app.origin_y = y - app.rows / 2
        self.app.force_window()
        self.Refresh()


class OverviewWindow(wx.Frame):

    def __init__(self, app):
        wx.Frame.__init__(
            self,
            None,
            title="d.rast.edit overview (%s)" %
            app.inmap)
        self.app = app

        self.canvas = OverviewCanvas(app, parent=self)

        self.Bind(wx.EVT_CLOSE, self.OnClose)

    def OnClose(self, ev):
        self.app.finalize()


class Canvas(wx.ScrolledWindow):

    def __init__(self, app, parent):
        wx.ScrolledWindow.__init__(self, parent)
        self.app = app

        self.size = app.size

        w = app.cols * self.size
        h = app.rows * self.size

        self.SetVirtualSize((w, h))
        self.SetVirtualSizeHints(50, 50)
        self.SetScrollRate(1, 1)

        self.Bind(wx.EVT_LEFT_DOWN, self.OnMouse)
        self.Bind(wx.EVT_RIGHT_DOWN, self.OnMouse)
        self.Bind(wx.EVT_MOTION, self.OnMouse)
        self.Bind(wx.EVT_PAINT, self.OnPaint2)

        self.row = 0
        self.col = 0

        self.gray12 = wx.BitmapFromBits(gray12_bits, 16, 16)

    def OnMouse(self, ev):
        oldrow = self.row
        oldcol = self.col

        x = ev.GetX()
        y = ev.GetY()
        (x, y) = self.CalcUnscrolledPosition(x, y)

        col = x / self.size
        row = y / self.size

        self.row = row
        self.col = col

        if ev.Moving():
            self.refresh_cell(oldrow, oldcol)
            self.refresh_cell(row, col)
        elif ev.ButtonDown(wx.MOUSE_BTN_LEFT):
            self.cell_set()
        elif ev.ButtonDown(wx.MOUSE_BTN_RIGHT):
            self.cell_get()

    def paint_cell(self, dc, r, c):
        if r < 0 or r >= self.app.rows or c < 0 or c >= self.app.cols:
            return

        val = self.app.values[r][c]
        if val is None:
            fill = 'black'
            stipple = self.gray12
        else:
            fill = self.app.get_color(val)
            stipple = None

        if r == self.row and c == self.col:
            outline = 'red'
        elif self.app.changed[r][c]:
            outline = 'white'
        else:
            outline = 'black'

        dc.SetPen(wx.Pen(outline))

        if stipple:
            brush = wx.Brush(fill, style=wx.STIPPLE)
            brush.SetStipple(stipple)
        else:
            brush = wx.Brush(fill, style=wx.SOLID)

        x0 = c * self.size + 1
        x1 = x0 + self.size - 1
        y0 = r * self.size + 1
        y1 = y0 + self.size - 1

        dc.SetBrush(brush)
        dc.DrawRectangle(x0, y0, x1 - x0, y1 - y0)
        dc.SetPen(wx.NullPen)
        dc.SetBrush(wx.NullBrush)

        if not self.app.angles:
            return

        if self.app.angles[r][c] == '*':
            return

        cx = (x0 + x1) / 2
        cy = (y0 + y1) / 2

        a = math.radians(float(self.app.angles[r][c]))
        dx = math.cos(a) * self.size / 2
        dy = -math.sin(a) * self.size / 2

        x0 = cx - dx
        y0 = cy - dy
        x1 = cx + dx
        y1 = cy + dy

        dx, dy = x1 - x0, y1 - y0
        px, py = -dy, dx

        r, g, b = wx.NamedColor(fill)
        if r + g + b > 384:
            line = 'black'
        else:
            line = 'white'

        dc.SetPen(wx.Pen(line))
        dc.DrawLine(x0, y0, x1, y1)
        dc.DrawLine(x1, y1, x1 + px / 6 - dx / 3, y1 + py / 6 - dy / 3)
        dc.DrawLine(x1, y1, x1 - px / 6 - dx / 3, y1 - py / 6 - dy / 3)
        dc.SetPen(wx.NullPen)

    def paint_rect(self, dc, x, y, w, h):
        c0 = (x + 0) / self.size
        c1 = (x + w + 1) / self.size
        r0 = (y + 0) / self.size
        r1 = (y + h + 1) / self.size
        for r in range(r0, r1 + 1):
            for c in range(c0, c1 + 1):
                self.paint_cell(dc, r, c)

    def OnPaint(self, ev):
        dc = wx.PaintDC(self)
        self.DoPrepareDC(dc)
        for r in range(self.app.rows):
            for c in range(self.app.cols):
                self.paint_cell(dc, r, c)

    def OnPaint2(self, ev):
        dc = wx.PaintDC(self)
        self.DoPrepareDC(dc)
        it = wx.RegionIterator(self.GetUpdateRegion())
        while it.HaveRects():
            x = it.GetX()
            y = it.GetY()
            w = it.GetW()
            h = it.GetH()
            (x, y) = self.CalcUnscrolledPosition(x, y)
            self.paint_rect(dc, x, y, w, h)
            it.Next()

    def cell_enter(self):
        if not self.row or not self.col:
            return
        self.app.update_status(self.row, self.col)

    def cell_leave(self):
        self.app.clear_status()

    def cell_get(self):
        self.app.brush = self.app.values[self.row][self.col]
        self.app.frame.brush_update()

    def cell_set(self):
        self.app.values[self.row][self.col] = self.app.brush
        self.app.changed[self.row][self.col] = True
        self.refresh_cell(self.row, self.col)

    def refresh_cell(self, row, col):
        x = col * self.size
        y = row * self.size
        (x, y) = self.CalcScrolledPosition(x, y)
        self.RefreshRect((x, y, self.size, self.size))


class ColorPanel(wx.Panel):

    def __init__(self, **kwargs):
        wx.Panel.__init__(self, **kwargs)
        self.SetBackgroundStyle(wx.BG_STYLE_COLOUR)
        self.stipple = wx.BitmapFromBits(gray12_bits, 16, 16)
        self.null_bg = True
        self.Bind(wx.EVT_ERASE_BACKGROUND, self.OnErase)

    def OnErase(self, ev):
        self.ClearBackground()

        if not self.null_bg:
            return

        dc = ev.GetDC()
        if not dc:
            dc = wx.ClientDC(self)

        brush = wx.Brush('black', style=wx.STIPPLE)
        brush.SetStipple(self.stipple)
        dc.SetBackground(brush)
        dc.Clear()
        dc.SetBackground(wx.NullBrush)

    def SetNullBackgroundColour(self):
        wx.Panel.SetBackgroundColour(self, 'gray')
        self.null_bg = True

    def SetBackgroundColour(self, color):
        wx.Panel.SetBackgroundColour(self, color)
        self.null_bg = False


class MainWindow(wx.Frame):

    def __init__(self, app):
        wx.Frame.__init__(self, None, title="d.rast.edit (%s)" % app.inmap)
        self.app = app

        self.Bind(wx.EVT_CLOSE, self.OnClose)

        filemenu = wx.Menu()
        filemenu.Append(wx.ID_SAVE, "&Save", "Save changes")
        filemenu.Append(wx.ID_EXIT, "E&xit", "Terminate the program")
        menubar = wx.MenuBar()
        menubar.Append(filemenu, "&File")
        self.SetMenuBar(menubar)

        wx.EVT_MENU(self, wx.ID_SAVE, self.OnSave)
        wx.EVT_MENU(self, wx.ID_EXIT, self.OnExit)

        sizer = wx.BoxSizer(orient=wx.VERTICAL)

        self.canvas = Canvas(app, parent=self)
        si = sizer.Add(self.canvas, proportion=1, flag=wx.EXPAND)

        tools = wx.BoxSizer(wx.HORIZONTAL)

        l = wx.StaticText(parent=self, label='New Value:')
        tools.Add(l, flag=wx.ALIGN_CENTER_VERTICAL)
        tools.AddSpacer(5)

        self.newval = wx.TextCtrl(parent=self, style=wx.TE_PROCESS_ENTER)
        tools.Add(self.newval, flag=wx.ALIGN_CENTER_VERTICAL)

        l = wx.StaticText(parent=self, label='Color:')
        tools.Add(l, flag=wx.ALIGN_CENTER_VERTICAL)
        tools.AddSpacer(5)

        self.color = ColorPanel(parent=self, size=(30, 5))
        tools.Add(self.color, flag=wx.ALIGN_CENTER_VERTICAL | wx.EXPAND)

        self.Bind(wx.EVT_TEXT_ENTER, self.OnReturn, source=self.newval)

        sizer.AddSizer(tools, proportion=0, flag=wx.EXPAND)

        self.SetSizerAndFit(sizer)
        self.SetSize((app.width, app.height))

        self.status = self.CreateStatusBar(6)

        self.brush_update()

    def OnSave(self, ev):
        self.app.save_map()

    def OnExit(self, ev):
        self.Close(True)

    def OnClose(self, ev):
        self.app.finalize()

    def OnReturn(self, ev):
        self.app.brush = self.newval.GetValue()
        if self.app.brush != '*' and self.app.brush.strip('0123456789') != '':
            self.app.brush = '*'
        self.brush_update()

    def update_status(self):
        for i, key in enumerate(['row', 'col', 'x', 'y', 'value', 'aspect']):
            s = "%s%s: %s" % (key[0].upper(), key[1:], self.app.status[key])
            self.status.SetStatusText(s, i)

    def clear_status(self):
        for key in self.status:
            self.status[key] = ''

    def brush_update(self):
        self.newval.ChangeValue(self.app.brush)
        if self.app.brush == '*':
            self.color.SetNullBackgroundColour()
        else:
            self.color.SetBackgroundColour(self.app.get_color(self.app.brush))
        self.color.Refresh()


class Application(wx.App):

    def __init__(self, options):
        self.options = options
        wx.App.__init__(self)

    def initialize(self):
        grass.use_temp_region()

        run('g.region', raster=self.inmap)

        reg = grass.region()
        for k, f in wind_keys.values():
            self.total[k] = (f)(reg[k])

        if self.cols > self.total['cols']:
            self.cols = self.total['cols']
        if self.rows > self.total['rows']:
            self.rows = self.total['rows']

        tempbase = grass.tempfile()
        grass.try_remove(tempbase)

        self.tempfile = tempbase + '.ppm'
        self.tempmap = 'tmp.d.rast.edit'

        atexit.register(self.cleanup)

        run('g.copy', raster=(self.inmap, self.outmap), overwrite=True)
        run('r.colors', map=self.outmap, rast=self.inmap)

    def cleanup(self):
        grass.try_remove(self.tempfile)
        run('g.remove', flags='f', type='raster', name=self.tempmap)

    def finalize(self):
        self.save_map()
        sys.exit(0)

    def save_map(self):
        p = grass.feed_command(
            'r.in.ascii',
            input='-',
            output=self.tempmap,
            quiet=True,
            overwrite=True)
        outf = p.stdin
        outf.write("north: %f\n" % self.wind['n'])
        outf.write("south: %f\n" % self.wind['s'])
        outf.write("east: %f\n" % self.wind['e'])
        outf.write("west: %f\n" % self.wind['w'])
        outf.write("rows: %d\n" % self.wind['rows'])
        outf.write("cols: %d\n" % self.wind['cols'])
        outf.write("null: *\n")

        for row in range(self.wind['rows']):
            for col in range(self.wind['cols']):
                if col > 0:
                    outf.write(" ")
                val = self.values[row][col]
                if val and self.changed[row][col]:
                    outf.write("%s" % val)
                else:
                    outf.write('*')
            outf.write("\n")

        outf.close()
        p.wait()

        run('g.region', raster=self.inmap)
        run('r.patch',
            input=(self.tempmap,
                   self.outmap),
            output=self.outmap,
            overwrite=True)
        run('r.colors', map=self.outmap, rast=self.inmap)
        run('g.remove', flags='f', type='raster', name=self.tempmap)

    def read_header(self, infile):
        wind = {}
        for i in range(6):
            line = infile.readline().rstrip('\r\n')
            f = line.split(':')
            key = f[0]
            val = f[1].strip()
            (k, f) = wind_keys[key]
            wind[k] = (f)(val)
        return wind

    def read_data(self, infile):
        values = []
        for row in range(self.wind['rows']):
            line = infile.readline().rstrip('\r\n')
            values.append(line.split())
        return values

    def load_map(self):
        run('g.region', **self.wind)

        p = grass.pipe_command('r.out.ascii', input=self.inmap, quiet=True)
        self.wind = self.read_header(p.stdout)
        self.values = self.read_data(p.stdout)
        self.changed = [[False for c in row] for row in self.values]
        p.wait()

        self.clear_changes()

        run('r.out.ppm', input=self.inmap, output=self.tempfile)
        colorimg = wx.Image(self.tempfile)
        grass.try_remove(self.tempfile)

        for row in range(self.wind['rows']):
            for col in range(self.wind['cols']):
                val = self.values[row][col]
                if val in self.colors:
                    continue
                r = colorimg.GetRed(col, row)
                g = colorimg.GetGreen(col, row)
                b = colorimg.GetBlue(col, row)
                color = "#%02x%02x%02x" % (r, g, b)
                self.colors[val] = color

        colorimg.Destroy()

    def load_aspect(self):
        if not self.aspect:
            return

        p = grass.pipe_command('r.out.ascii', input=self.aspect, quiet=True)
        self.read_header(p.stdout)
        self.angles = self.read_data(p.stdout)
        p.wait()

    def clear_changes(self):
        for row in range(self.wind['rows']):
            for col in range(self.wind['cols']):
                self.changed[row][col] = 0

    def update_window(self):
        x0 = self.origin_x
        y0 = self.origin_y
        x1 = x0 + self.cols
        y1 = y0 + self.rows

        self.wind['n'] = self.total['n'] - y0 * self.total['nsres']
        self.wind['s'] = self.total['n'] - y1 * self.total['nsres']
        self.wind['w'] = self.total['w'] + x0 * self.total['ewres']
        self.wind['e'] = self.total['w'] + x1 * self.total['ewres']
        self.wind['rows'] = self.rows
        self.wind['cols'] = self.cols

    def change_window(self):
        self.save_map()
        self.update_window()
        self.load_map()
        self.load_aspect()
        self.refresh_canvas()

    def force_window(self):
        if self.origin_x < 0:
            self.origin_x = 0
        if self.origin_x > self.total['cols'] - self.cols:
            self.origin_x = self.total['cols'] - self.cols
        if self.origin_y < 0:
            self.origin_y = 0
        if self.origin_y > self.total['rows'] - self.rows:
            self.origin_y = self.total['rows'] - self.rows

    def update_status(self, row, col):
        self.status['row'] = row
        self.status['col'] = col
        self.status['x'] = self.wind[
            'e'] + (col + 0.5) * (self.wind['e'] - self.wind['w']) / self.wind['cols']
        self.status['y'] = self.wind[
            'n'] - (row + 0.5) * (self.wind['n'] - self.wind['s']) / self.wind['rows']
        self.status['value'] = self.values[row][col]
        if self.angles:
            self.status['aspect'] = self.angles[row][col]

    def force_color(val):
        run('g.region', rows=1, cols=1)
        run('r.mapcalc', expression="%s = %d" % (self.tempmap, val))
        run('r.colors', map=self.tempmap, rast=self.inmap)
        run('r.out.ppm', input=self.tempmap, out=self.tempfile)
        run('g.remove', flags='f', type='raster', name=self.tempmap)

        tempimg = wx.Image(self.tempfile)
        grass.try_remove(self.tempfile)

        rgb = tempimg.get(0, 0)
        color = "#%02x%02x%02x" % rgb
        self.colors[val] = color
        tempimg.delete()

    def get_color(self, val):
        if val not in self.colors:
            try:
                self.force_color(val)
            except:
                self.colors[val] = "#ffffff"

        return self.colors[val]

    def refresh_canvas(self):
        self.frame.canvas.Refresh()

    def make_ui(self):
        self.frame = MainWindow(self)
        self.SetTopWindow(self.frame)
        self.frame.Show()
        self.overview = OverviewWindow(self)
        self.overview.Show()

    def OnInit(self):
        self.outmap = self.options['output']
        self.inmap = self.options['input']
        self.aspect = self.options['aspect']
        self.width = int(self.options['width'])
        self.height = int(self.options['height'])
        self.size = int(self.options['size'])
        self.rows = int(self.options['rows'])
        self.cols = int(self.options['cols'])

        self.status = {
            'row': '',
            'col': '',
            'x': '',
            'y': '',
            'value': '',
            'aspect': ''
        }

        self.values = None
        self.changed = None
        self.angles = None
        self.colors = {}
        self.brush = '*'
        self.origin_x = 0
        self.origin_y = 0
        self.wind = {}
        self.total = {}

        self.initialize()
        self.update_window()
        self.make_ui()
        self.load_map()
        self.load_aspect()
        self.refresh_canvas()

        return True

if __name__ == "__main__":
    options, flags = grass.parser()

    app = Application(options)
    app.MainLoop()
