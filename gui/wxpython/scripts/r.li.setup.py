"""
MODULE:    r_li_setup_GUI.py

AUTHOR(S): Anne Ghisla <a.ghisla AT gmail.com>

PURPOSE:   Dedicated GUI for r.li.setup, translated and reshaped from original TclTk.

DEPENDS:   []

COPYRIGHT: (C) 2010 by the GRASS Development Team

This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.
"""
# bulk import from location_wizard.py. 
import os
import shutil
import re
import string
import sys
import locale
import platform

GUIModulesPath = os.path.join(os.getenv("GISBASE"), "etc", "gui", "wxpython", "gui_modules")
sys.path.append(GUIModulesPath)

import globalvar

import wx
import wx.lib.mixins.listctrl as listmix
import wx.wizard as wiz
import wx.lib.scrolledpanel as scrolled
import time

import gcmd
import utils
from grass.script import core as grass

#@TODO create wizard instead of progressively increasing window

print >> sys.stderr, "TODO: implement r.li.setup wizard..."

#@NOTE: r.li.setup writes in the settings file with 
## r.li.windows.tcl:
#exec echo "SAMPLINGFRAME $per_x|$per_y|$per_rl|$per_cl" >> $env(TMP).set

class BaseClass(wx.Object):
    """!Base class providing basic methods"""
    def __init__(self):
        pass

    def MakeLabel(self, text="", style=wx.ALIGN_LEFT, parent=None):
        """!Make aligned label"""
        if not parent:
            parent = self
        return wx.StaticText(parent=parent, id=wx.ID_ANY, label=text,
                             style=style)

    def MakeTextCtrl(self, text='', size=(100,-1), style=0, parent=None):
        """!Generic text control"""
        if not parent:
            parent = self
        return wx.TextCtrl(parent=parent, id=wx.ID_ANY, value=text,
                           size=size, style=style)

    def MakeButton(self, text, id=wx.ID_ANY, size=(-1,-1), parent=None):
        """!Generic button"""
        if not parent:
            parent = self
        return wx.Button(parent=parent, id=id, label=text,
                         size=size)

class TitledPage(BaseClass, wiz.WizardPageSimple):
    """
    Class to make wizard pages. Generic methods to make
    labels, text entries, and buttons.
    """
    def __init__(self, parent, title):

        self.page = wiz.WizardPageSimple.__init__(self, parent)

        # page title
        self.title = wx.StaticText(parent=self, id=wx.ID_ANY, label=title)
        self.title.SetFont(wx.Font(13, wx.SWISS, wx.NORMAL, wx.BOLD))

        # main sizers
        self.pagesizer = wx.BoxSizer(wx.VERTICAL)
        self.sizer = wx.GridBagSizer(vgap=0, hgap=0)
        
    def DoLayout(self):
        """!Do page layout"""
      

        self.pagesizer.Add(item=self.title, proportion=0,
                     flag=wx.ALIGN_CENTRE | wx.ALL,
                     border=5)
        self.pagesizer.Add(item=wx.StaticLine(self, -1), proportion=0,
                     flag=wx.EXPAND | wx.ALL,
                     border=0)
        self.pagesizer.Add(item=self.sizer)

        self.SetAutoLayout(True)
        self.SetSizer(self.pagesizer)
        # tmpsizer.Fit(self)
        self.Layout()
        
        
class LocationWizard(wx.Object):
    """
    Start wizard here and finish wizard here
    """
    def __init__(self, parent): 
#        global coordsys
        self.parent = parent

        #
        # define wizard image
        #
        # file = "loc_wizard.png"
        file = "loc_wizard_qgis.png"
        imagePath = os.path.join(globalvar.ETCIMGDIR, file)
        wizbmp = wx.Image(imagePath, wx.BITMAP_TYPE_PNG)
        # wizbmp.Rescale(250,600)
        wizbmp = wizbmp.ConvertToBitmap()

        #
        # get georeferencing information from tables in $GISBASE/etc
        #
#        self.__readData()
#        
#        #
#        # datum transform number and list of datum transforms
#        #
#        self.datumtrans = 0
#        self.proj4string = ''
#
#        #
#        # define wizard pages
#        #
        self.wizard = wiz.Wizard(parent, id=wx.ID_ANY, title=_("I m!"),
                                 bitmap=wizbmp)
        self.startpage = SummaryPage(self.wizard, self)
#        self.csystemspage = CoordinateSystemPage(self.wizard, self)
#        self.projpage = ProjectionsPage(self.wizard, self)
#        self.datumpage = DatumPage(self.wizard, self)
#        self.paramspage = ProjParamsPage(self.wizard,self)
#        self.epsgpage = EPSGPage(self.wizard, self)
#        self.filepage = GeoreferencedFilePage(self.wizard, self)
#        self.wktpage = WKTPage(self.wizard, self)
#        self.ellipsepage = EllipsePage(self.wizard, self)
#        self.custompage = CustomPage(self.wizard, self)
#        self.sumpage = SummaryPage(self.wizard, self)
#
#        #
#        # set the initial order of the pages
#        # (should follow the epsg line)
#        #
#        self.startpage.SetNext(self.csystemspage)
#
#        self.csystemspage.SetPrev(self.startpage)
#        self.csystemspage.SetNext(self.sumpage)
#
#        self.projpage.SetPrev(self.csystemspage)
#        self.projpage.SetNext(self.paramspage)
#
#        self.paramspage.SetPrev(self.projpage)
#        self.paramspage.SetNext(self.datumpage)
#
#        self.datumpage.SetPrev(self.paramspage)
#        self.datumpage.SetNext(self.sumpage)
#
#        self.ellipsepage.SetPrev(self.paramspage)
#        self.ellipsepage.SetNext(self.sumpage)
#
#        self.epsgpage.SetPrev(self.csystemspage)
#        self.epsgpage.SetNext(self.sumpage)
#
#        self.filepage.SetPrev(self.csystemspage)
#        self.filepage.SetNext(self.sumpage)
#
#        self.wktpage.SetPrev(self.csystemspage)
#        self.wktpage.SetNext(self.sumpage)
#
#        self.custompage.SetPrev(self.csystemspage)
#        self.custompage.SetNext(self.sumpage)
#
#        self.sumpage.SetPrev(self.csystemspage)
#
#        #
#        # do pages layout
#        #
#        self.startpage.DoLayout()
#        self.csystemspage.DoLayout()
#        self.projpage.DoLayout()
#        self.datumpage.DoLayout()
#        self.paramspage.DoLayout()
#        self.epsgpage.DoLayout()
#        self.filepage.DoLayout()
#        self.wktpage.DoLayout()
#        self.ellipsepage.DoLayout()
#        self.custompage.DoLayout()
#        self.sumpage.DoLayout()
#        self.wizard.FitToPage(self.datumpage)
#
#        # new location created?
#        self.location = None 
#        success = False
#        
#        # location created in different GIS database?
#        self.altdb = False

        #
        # run wizard...
        #
        if self.wizard.RunWizard(self.startpage):
            pass
#            msg = self.OnWizFinished()
#            if len(msg) < 1:
#                self.wizard.Destroy()
#                self.location = self.startpage.location
#                
#                if self.altdb == False: 
#                    dlg = wx.MessageDialog(parent=self.parent,
#                                           message=_("Do you want to set the default "
#                                                     "region extents and resolution now?"),
#                                           caption=_("Location <%s> created") % self.location,
#                                           style=wx.YES_NO | wx.YES_DEFAULT | wx.ICON_QUESTION)
#                    dlg.CenterOnScreen()
#                    if dlg.ShowModal() == wx.ID_YES:
#                        dlg.Destroy()
#                        defineRegion = RegionDef(self.parent, location=self.location)
#                        defineRegion.CenterOnScreen()
#                        defineRegion.Show()
#                    else:
#                        dlg.Destroy()
#            else: # -> error
#                self.wizard.Destroy()
#                wx.MessageBox(parent=self.parent,
#                              message="%s" % _("Unable to create new location. "
#                                               "Location <%(loc)s> not created.\n\n"
#                                               "Details: %(err)s") % \
#                                  { 'loc' : self.startpage.location,
#                                    'err' : msg },
#                              caption=_("Location wizard"),
#                              style=wx.OK | wx.ICON_ERROR | wx.CENTRE)
#        else:
#            win = wx.MessageBox(parent=self.parent,
#                                message=_("Location wizard canceled. "
#                                          "Location not created."),
#                                caption=_("Location wizard"))
    
    def __readData(self):
        """!Get georeferencing information from tables in $GISBASE/etc"""

        # read projection and parameters
        f = open(os.path.join(globalvar.ETCDIR, "proj-parms.table"), "r")
        self.projections = {}
        self.projdesc = {}
        for line in f.readlines():
            line = line.strip()
            try:
                proj, projdesc, params = line.split(':')
                paramslist = params.split(';')
                plist = []
                for p in paramslist:
                    if p == '': continue
                    p1, pdefault = p.split(',')
                    pterm, pask = p1.split('=')
                    p = [pterm.strip(), pask.strip(), pdefault.strip()]
                    plist.append(p)
                self.projections[proj.lower().strip()] = (projdesc.strip(), plist)
                self.projdesc[proj.lower().strip()] = projdesc.strip()
            except:
                continue
        f.close()

        # read datum definitions
        f = open(os.path.join(globalvar.ETCDIR, "datum.table"), "r")
        self.datums = {}
        paramslist = []
        for line in f.readlines():
            line = line.expandtabs(1)
            line = line.strip()
            if line == '' or line[0] == "#":
                continue
            datum, info = line.split(" ", 1)
            info = info.strip()
            datumdesc, params = info.split(" ", 1)
            datumdesc = datumdesc.strip('"')
            paramlist = params.split()
            ellipsoid = paramlist.pop(0)
            self.datums[datum] = (ellipsoid, datumdesc.replace('_', ' '), paramlist)
        f.close()

        # read ellipsiod definitions
        f = open(os.path.join(globalvar.ETCDIR, "ellipse.table"), "r")
        self.ellipsoids = {}
        for line in f.readlines():
            line = line.expandtabs(1)
            line = line.strip()
            if line == '' or line[0] == "#":
                continue
            ellipse, rest = line.split(" ", 1)
            rest = rest.strip('" ')
            desc, params = rest.split('"', 1)
            desc = desc.strip('" ')
            paramslist = params.split()
            self.ellipsoids[ellipse] = (desc, paramslist)
        f.close()
        
        # read projection parameter description and parsing table
        f = open(os.path.join(globalvar.ETCDIR, "proj-desc.table"), "r")
        self.paramdesc = {}
        for line in f.readlines():
            line = line.strip()
            try:
                pparam, datatype, proj4term, desc = line.split(':')
                self.paramdesc[pparam] = (datatype, proj4term, desc)
            except:
                continue
        f.close()

    def OnWizFinished(self):
        database = self.startpage.grassdatabase
        location = self.startpage.location
        global coordsys
        msg = '' # error message (empty on success)
        
        # location already exists?
        if os.path.isdir(os.path.join(database,location)):
            dlg = wx.MessageDialog(parent=self.wizard,
                                   message="%s <%s>: %s" % \
                                       (_("Unable to create new location"),
                                        os.path.join(database, location),
                                        _("Location already exists in GRASS Database.")),
                                   caption=_("Error"),
                                   style=wx.OK | wx.ICON_ERROR)
            dlg.ShowModal()
            dlg.Destroy()
            return False
        
        # current GISDbase or a new one?
        current_gdb = grass.gisenv()['GISDBASE']
        if current_gdb != database:
            # change to new GISDbase or create new one
            if os.path.isdir(database) != True:
                # create new directory
                os.mkdir(database)
                
            # change to new GISDbase directory
            gcmd.RunCommand('g.gisenv',
                            parent = self.wizard,
                            set='GISDBASE=%s' % database)
            
            wx.MessageBox(parent=self.wizard,
                          message=_("Location <%(loc)s> will be created "
                                    "in GIS data directory <%(dir)s>."
                                    "You will need to change the default GIS "
                                    "data directory in the GRASS startup screen.") % \
                              { 'loc' : location, 'dir' : database},
                          caption=_("New GIS data directory"), 
                          style=wx.OK | wx.ICON_INFORMATION | wx.CENTRE)
            # location created in alternate GISDbase
            self.altdb = True
            
        if coordsys == "xy":
            msg = self.XYCreate()
        elif coordsys == "proj":
            proj4string = self.CreateProj4String()
            msg = self.Proj4Create(proj4string)
        elif coordsys == 'custom':
            msg = self.CustomCreate()
        elif coordsys == "epsg":
            msg = self.EPSGCreate()
        elif coordsys == "file":
            msg = self.FileCreate()
        elif coordsys == "wkt":
            msg = self.WKTCreate()

        return msg

    def XYCreate(self):
        """!Create an XY location

        @return error message (empty string on success)
        """        
        database = self.startpage.grassdatabase
        location = self.startpage.location
        
        # create location directory and PERMANENT mapset
        try:
            os.mkdir(os.path.join(database, location))
            os.mkdir(os.path.join(database, location, 'PERMANENT'))
            # create DEFAULT_WIND and WIND files
            regioninfo =   ['proj:       0',
                            'zone:       0',
                            'north:      1',
                            'south:      0',
                            'east:       1',
                            'west:       0',
                            'cols:       1',
                            'rows:       1',
                            'e-w resol:  1',
                            'n-s resol:  1',
                            'top:        1',
                            'bottom:     0',
                            'cols3:      1',
                            'rows3:      1',
                            'depths:     1',
                            'e-w resol3: 1',
                            'n-s resol3: 1',
                            't-b resol:  1']
            
            defwind = open(os.path.join(database, location, 
                                        "PERMANENT", "DEFAULT_WIND"), 'w')
            for param in regioninfo:
                defwind.write(param + '%s' % os.linesep)
            defwind.close()
            
            shutil.copy(os.path.join(database, location, "PERMANENT", "DEFAULT_WIND"),
                        os.path.join(database, location, "PERMANENT", "WIND"))
            
            # create MYNAME file
            myname = open(os.path.join(database, location, "PERMANENT",
                                       "MYNAME"), 'w')
            myname.write('%s' % os.linesep)
            myname.close()
        except OSError, e:
            return e
        
        return ''

    def CreateProj4String(self):
        """!Constract PROJ.4 string"""
        location = self.startpage.location
        proj = self.projpage.p4proj
        projdesc = self.projpage.projdesc
        proj4params = self.paramspage.p4projparams
                
        datum = self.datumpage.datum
        if self.datumpage.datumdesc:
            datumdesc = self.datumpage.datumdesc +' - ' + self.datumpage.ellipse
        else:
            datumdesc = ''
        datumparams = self.datumpage.datumparams        
        ellipse = self.ellipsepage.ellipse
        ellipsedesc = self.ellipsepage.ellipsedesc
        ellipseparams = self.ellipsepage.ellipseparams
                
        #
        # creating PROJ.4 string
        #
        proj4string = '%s %s' % (proj, proj4params)
                            
        # set ellipsoid parameters
        if ellipse != '': proj4string = '%s +ellps=%s' % (proj4string, ellipse)
        for item in ellipseparams:
            if item[:4] == 'f=1/':
                item = ' +rf='+item[4:]
            else:
                item = ' +'+item
            proj4string = '%s %s' % (proj4string, item)
            
        # set datum and transform parameters if relevant
        if datum != '': proj4string = '%s +datum=%s' % (proj4string, datum)
        if datumparams:
            for item in datumparams:
                proj4string = '%s +%s' % (proj4string,item)

        proj4string = '%s +no_defs' % proj4string
        
        return proj4string
        
    def Proj4Create(self, proj4string):
        """!Create a new location for selected projection
        
        @return error message (empty string on success)
        """
        ret, msg = gcmd.RunCommand('g.proj',
                                   flags = 'c',
                                   proj4 = proj4string,
                                   location = self.startpage.location,
                                   datumtrans = self.datumtrans,
                                   getErrorMsg = True)
        
        if ret == 0:
            return ''

        return msg
        

    def CustomCreate(self):
        """!Create a new location based on given proj4 string

        @return error message (empty string on success)
        """
        proj4string = self.custompage.customstring
        location = self.startpage.location
        
        ret, msg = gcmd.RunCommand('g.proj',
                                   flags = 'c',
                                   proj4 = proj4string,
                                   location = location,
                                   getErrorMsg = True)
        
        if ret == 0:
            return ''
        
        return msg

    def EPSGCreate(self):
        """!Create a new location from an EPSG code.

        @return error message (empty string on success)
        """
        epsgcode = self.epsgpage.epsgcode
        epsgdesc = self.epsgpage.epsgdesc
        location = self.startpage.location
        
        # should not happend
        if epsgcode == '':
            return _('EPSG code missing.')
        
        ret, msg = gcmd.RunCommand('g.proj',
                                   flags = 'c',
                                   epsg = epsgcode,
                                   location = location,
                                   datumtrans = self.datumtrans,
                                   getErrorMsg = True)
        
        if ret == 0:
            return ''

        return msg

    def FileCreate(self):
        """!Create a new location from a georeferenced file

        @return error message (empty string on success)
        """
        georeffile = self.filepage.georeffile
        location = self.startpage.location
        
        # this should not happen
        if not georeffile or not os.path.isfile(georeffile):
            return _("File not found.")
        
        # creating location
        ret, msg = gcmd.RunCommand('g.proj',
                                   flags = 'c',
                                   georef = georeffile,
                                   location = location,
                                   getErrorMsg = True)
        
        if ret == 0:
            return ''
        
        return msg

    def WKTCreate(self):
        """!Create a new location from a WKT file
        
        @return error message (empty string on success)
        """
        wktfile = self.wktpage.wktfile
        location = self.startpage.location
        
        # this should not happen
        if not wktfile or not os.path.isfile(wktfile):
            return _("File not found.")
        
        # creating location
        ret, msg = gcmd.RunCommand('g.proj',
                                   flags = 'c',
                                   wkt = wktfile,
                                   location = location,
                                   getErrorMsg = True)
        
        if ret == 0:
            return ''
        
        return msg
    
class SummaryPage(TitledPage):
    """
    Shows summary result of choosing coordinate system parameters
    prior to creating location
    """
    def __init__(self, wizard, parent):
        TitledPage.__init__(self, wizard, _("Summary"))

        self.parent = parent

        # labels
        self.ldatabase  = self.MakeLabel("")
        self.llocation  = self.MakeLabel("")
        self.lprojection = self.MakeLabel("")
        self.lproj4string = self.MakeLabel("")
        self.lproj4stringLabel = self.MakeLabel("")
        
        self.lprojection.Wrap(400)
        
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGED, self.OnEnterPage)
        # self.Bind(wx.EVT_BUTTON, self.OnFinish, wx.ID_FINISH)

        # do sub-page layout
        self.__DoLayout()
        
    def __DoLayout(self):
        """!Do page layout"""
        self.sizer.AddGrowableCol(1)
        self.sizer.Add(item=self.MakeLabel(_("GRASS Database:")),
                       flag=wx.ALIGN_LEFT | wx.ALL,
                       border=5, pos=(1, 0))
        self.sizer.Add(item=self.ldatabase, 
                       flag=wx.ALIGN_LEFT | wx.ALL,
                       border=5, pos=(1, 1))
        self.sizer.Add(item=self.MakeLabel(_("Location Name:")),
                       flag=wx.ALIGN_LEFT | wx.ALL,
                       border=5, pos=(2, 0))
        self.sizer.Add(item=self.llocation,
                       flag=wx.ALIGN_LEFT | wx.ALL,
                       border=5, pos=(2, 1))
        self.sizer.Add(item=self.MakeLabel(_("Projection:")),
                       flag=wx.ALIGN_LEFT | wx.ALL,
                       border=5, pos=(3, 0))
        self.sizer.Add(item=self.lprojection,
                       flag=wx.ALIGN_LEFT | wx.ALL,
                       border=5, pos=(3, 1))
        self.sizer.Add(item=self.lproj4stringLabel,
                       flag=wx.ALIGN_LEFT | wx.ALL,
                       border=5, pos=(4, 0))
        self.sizer.Add(item=self.lproj4string,
                       flag=wx.ALIGN_LEFT | wx.ALL,
                       border=5, pos=(4, 1))
        self.sizer.Add(item=(10,20),
                       flag=wx.ALIGN_CENTER_HORIZONTAL | wx.ALL,
                       border=5, pos=(5, 0), span=(1, 2))

    def OnEnterPage(self,event):
        """
        Insert values into text controls for summary of location creation options
        """

#        database = self.parent.startpage.grassdatabase
#        location = self.parent.startpage.location
        proj4string = self.parent.CreateProj4String()
        epsgcode = self.parent.epsgpage.epsgcode
        dtrans = self.parent.datumtrans
        
        global coordsys
        if coordsys not in ['proj', 'epsg']:
            self.lproj4stringLabel.Hide()
            self.lproj4string.Hide()
            self.lproj4stringLabel.SetLabel('')
            self.lproj4string.SetLabel('')
        else:
            self.lproj4string.Show()
            self.lproj4stringLabel.SetLabel(_("PROJ.4 definition:"))
            if coordsys == 'proj':
                ret, msg, err = gcmd.RunCommand('g.proj',
                                       flags = 'j',
                                       proj4 = proj4string,
                                       datumtrans = dtrans,
                                       location = location,
                                       getErrorMsg = True,
                                       read = True)
            elif coordsys == 'epsg':
                ret, msg, err = gcmd.RunCommand('g.proj',
                                       flags = 'j',
                                       epsg = epsgcode,
                                       datumtrans = dtrans,
                                       location = location,
                                       getErrorMsg = True,
                                       read = True)
            
            if ret == 0:
                projlabel = ''
                for line in msg.splitlines():
                    projlabel = projlabel + '%s ' % line
                self.lproj4string.SetLabel(projlabel)
            else:
                wx.MessageBox(err, 'Error', wx.ICON_ERROR)

            self.lproj4string.Wrap(400)
            
        projdesc = self.parent.projpage.projdesc
        ellipsedesc = self.parent.ellipsepage.ellipsedesc
        datumdesc = self.parent.datumpage.datumdesc
#        self.ldatabase.SetLabel(database)
#        self.llocation.SetLabel(location)
        label = ''
        
        if coordsys == 'epsg':
            label = 'EPSG code %s (%s)' % (self.parent.epsgpage.epsgcode, self.parent.epsgpage.epsgdesc)
            self.lprojection.SetLabel(label)
        elif coordsys == 'file':
            label = 'matches file %s' % self.parent.filepage.georeffile
            self.lprojection.SetLabel(label)
        elif coordsys == 'wkt':
            label = 'matches file %s' % self.parent.wktpage.wktfile
            self.lprojection.SetLabel(label)
        elif coordsys == 'proj':
            label = ('%s, %s %s' % (projdesc, datumdesc, ellipsedesc))
            self.lprojection.SetLabel(label)
        elif coordsys == 'xy':
            label = ('XY coordinate system (not projected).')
            self.lprojection.SetLabel(label)
        elif coordsys == 'custom':
            label = ('%s' % self.parent.custompage.customstring)
            self.lprojection.SetLabel(label)

    def OnFinish(self, event):
        dlg = wx.MessageDialog(parent=self.wizard,
                               message=_("Do you want to create GRASS location <%s>?") % location,
                               caption=_("Create new location?"),
                               style=wx.YES_NO | wx.YES_DEFAULT | wx.ICON_QUESTION)

        if dlg.ShowModal() == wx.ID_NO:
            dlg.Destroy()
            event.Veto()
        else:
            dlg.Destroy()
            event.Skip()
      
if __name__ == "__main__":
    app = wx.App()
    gWizard = LocationWizard(None)
#    gWizard.Show()
    app.MainLoop()
