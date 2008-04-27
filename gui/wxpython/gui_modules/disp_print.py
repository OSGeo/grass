"""
MODULE:     disp_print.py

CLASSES:
    * MapPrint
    * PrintOptions

PURPOSE:    Print context and utility functions for printing
            contents of map display window.

AUTHORS:    The GRASS Development Team
            Michael Barton (Arizona State University)
            Based on generic example code from wxPython
            demo program by Robin Dunn

COPYRIGHT:  (C) 2007 by the GRASS Development Team
            This program is free software under the GNU General Public
            License (>=v2). Read the file COPYING that comes with GRASS
            for details.

"""

import  wx


class MapPrint(wx.Printout):
    def __init__(self, canvas):
        wx.Printout.__init__(self)
        self.canvas = canvas

    def OnBeginDocument(self, start, end):
        return super(MapPrint, self).OnBeginDocument(start, end)

    def OnEndDocument(self):
        super(MapPrint, self).OnEndDocument()

    def OnBeginPrinting(self):
        super(MapPrint, self).OnBeginPrinting()

    def OnEndPrinting(self):
        super(MapPrint, self).OnEndPrinting()

    def OnPreparePrinting(self):
        super(MapPrint, self).OnPreparePrinting()

    def HasPage(self, page):
        if page <= 2:
            return True
        else:
            return False

    def GetPageInfo(self):
        return (1, 2, 1, 2)

    def OnPrintPage(self, page):
        dc = self.GetDC()

        #-------------------------------------------
        # One possible method of setting scaling factors...
        maxX, maxY = self.canvas.GetSize()

        # Let's have at least 50 device units margin
        marginX = 10
        marginY = 10

        # Add the margin to the graphic size
        maxX = maxX + (2 * marginX)
        maxY = maxY + (2 * marginY)

        # Get the size of the DC in pixels
        (w, h) = dc.GetSizeTuple()

        # Calculate a suitable scaling factor
        scaleX = float(w) / maxX
        scaleY = float(h) / maxY

        # Use x or y scaling factor, whichever fits on the DC
        actualScale = min(scaleX, scaleY)

        # Calculate the position on the DC for centering the graphic
        posX = (w - (self.canvas.GetSize()[0] * actualScale)) / 2.0
        posY = (h - (self.canvas.GetSize()[1] * actualScale)) / 2.0

        # Set the scale and origin
        dc.SetUserScale(actualScale, actualScale)
        dc.SetDeviceOrigin(int(posX), int(posY))

        #-------------------------------------------

        self.canvas.pdc.DrawToDC(dc)

        # prints a page number on the page
#        dc.DrawText("Page: %d" % page, marginX/2, maxY-marginY)

        return True

class PrintOptions:
    def __init__(self, parent, mapwin):
        self.mapframe = parent
        self.mapwin = mapwin
#        self.frame = frame

        self.printData = wx.PrintData()
        self.printData.SetPaperId(wx.PAPER_LETTER)
        self.printData.SetPrintMode(wx.PRINT_MODE_PRINTER)

#        self.canvas = ScrolledWindow.MyCanvas(self)


    def OnPageSetup(self, event):
        psdd = wx.PageSetupDialogData(self.printData)
        psdd.CalculatePaperSizeFromId()
        dlg = wx.PageSetupDialog(self.mapwin, psdd)
        dlg.ShowModal()

        # this makes a copy of the wx.PrintData instead of just saving
        # a reference to the one inside the PrintDialogData that will
        # be destroyed when the dialog is destroyed
        self.printData = wx.PrintData( dlg.GetPageSetupData().GetPrintData() )

        dlg.Destroy()

    def OnPrintPreview(self, event):
        data = wx.PrintDialogData(self.printData)
        printout = MapPrint(self.mapwin)
        printout2 = MapPrint(self.mapwin)
        self.preview = wx.PrintPreview(printout, printout2, data)

        if not self.preview.Ok():
            wx.MessageBox("There was a problem printing this display\n", wx.OK)
            return

        pfrm = wx.PreviewFrame(self.preview, self.mapframe, "Print preview")

        pfrm.Initialize()
        pfrm.SetPosition(self.mapframe.GetPosition())
        pfrm.SetSize(self.mapframe.GetClientSize())
        pfrm.Show(True)

    def OnDoPrint(self, event):
        pdd = wx.PrintDialogData(self.printData)
        # set number of pages/copies
        pdd.SetToPage(1)
        printer = wx.Printer(pdd)
        printout = MapPrint(self.mapwin)

        if not printer.Print(self.mapframe, printout, True):
            wx.MessageBox("There was a problem printing.\nPerhaps your current printer is not set correctly?", "Printing", wx.OK)
        else:
            self.printData = wx.PrintData( printer.GetPrintDialogData().GetPrintData() )
        printout.Destroy()
