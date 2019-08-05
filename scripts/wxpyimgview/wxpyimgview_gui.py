#!/usr/bin/env python3

############################################################################
#
# MODULE:       wxpyimgview
# AUTHOR(S):    Glynn Clements <glynn@gclements.plus.com>
# COPYRIGHT:    (C) 2010 Glynn Clements
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
#% description: Views BMP images from the PNG driver.
#% keyword: display
#% keyword: raster
#%end
#%option G_OPT_F_INPUT
#% key: image
#% description: Name of input image file
#%end
#%option
#% key: percent
#% type: integer
#% required: no
#% multiple: no
#% description: Percentage of CPU time to use
#% answer: 10
#%end

import sys
import struct
import numpy
import time
import signal

import wxversion
wxversion.select('2.8')
import wx


class Frame(wx.Frame):
    title = "Image Viewer"

    def __init__(self, app, size):
        self.app = app
        wx.Frame.__init__(self, None, title=Frame.title, size=size)
        self.Create()

    def Create(self):
        self.Bind(wx.EVT_ERASE_BACKGROUND, self.erase)
        self.Bind(wx.EVT_PAINT, self.redraw)

        self.Bind(wx.EVT_TIMER, self.tick, id=1)
        self.timer = wx.Timer(self, 1)
        self.timer.Start(100, True)

        # Python doesn't receive signals while wx is waiting for an event
        self.Bind(wx.EVT_TIMER, self.dummy, id=2)
        self.ticker = wx.Timer(self, 2)
        self.ticker.Start(100, False)

    def erase(self, ev):
        ev.GetDC()

    def draw(self):
        app = self.app
        size = self.GetSize()
        x0 = (size.GetWidth() - app.i_width) / 2
        y0 = (size.GetHeight() - app.i_height) / 2
        dc = wx.PaintDC(self)
        data = app.imgbuf.reshape((app.i_height, app.i_width, 4))
        data = data[::, ::, 2::-1]
        fn = getattr(data, "tobytes", getattr(data, "tostring"))
        image = wx.ImageFromData(app.i_width, app.i_height, fn())
        dc.DrawBitmap(wx.BitmapFromImage(image), x0, y0, False)

    def redraw(self, ev):
        if self.app.fraction > 0.001:
            t0 = time.time()
            self.draw()
            t1 = time.time()

            last = t1 - t0
            delay = last / self.app.fraction
            self.timer.Start(int(delay * 1000), True)
        else:
            self.draw()

    def tick(self, ev):
        self.Refresh()

    def dummy(self, ev):
        pass


class Application(wx.App):

    def __init__(self):
        self.image = sys.argv[1]
        self.fraction = int(sys.argv[2]) / 100.0
        self.HEADER_SIZE = 64
        wx.App.__init__(self)

    def read_bmp_header(self, header):
        magic, bmfh, bmih = struct.unpack("2s12s40s10x", header)

        if magic != 'BM':
            raise SyntaxError("Invalid magic number")

        size, res1, res2, hsize = struct.unpack("<IHHI", bmfh)

        if hsize != self.HEADER_SIZE:
            raise SyntaxError("Invalid file header size")

        hsize, width, height, planes, bpp, compression, imsize, xppm, yppm, cused, cimp = \
            struct.unpack("<IiiHHIIiiII", bmih)

        if hsize != 40:
            raise SyntaxError("Invalid info header size")

        self.i_width = width
        self.i_height = -height

        if planes != 1:
            raise SyntaxError("Planar data not supported")
        if bpp != 32:
            raise SyntaxError("Only 32-BPP images supported")
        if compression != 0:
            raise SyntaxError("Compression not supported")
        if imsize != self.i_width * self.i_height * 4:
            raise SyntaxError("Invalid image data size")
        if size != self.HEADER_SIZE + self.i_width * self.i_height * 4:
            raise SyntaxError("Invalid image size")

    def map_file(self):
        f = open(self.image, 'r')

        header = f.read(self.HEADER_SIZE)
        self.read_bmp_header(header)

        self.imgbuf = numpy.memmap(f, mode='r', offset=self.HEADER_SIZE)

    def signal_handler(self, sig, frame):
        wx.CallAfter(self.mainwin.Refresh)

    def set_handler(self):
        if 'SIGUSR1' in dir(signal):
            signal.signal(signal.SIGUSR1, self.signal_handler)

    def OnInit(self):
        self.map_file()

        size = wx.Size(self.i_width, self.i_height)
        self.mainwin = Frame(self, size)
        self.mainwin.Show()
        self.SetTopWindow(self.mainwin)

        self.set_handler()

        return True

if __name__ == "__main__":
    app = Application()
    app.MainLoop()
