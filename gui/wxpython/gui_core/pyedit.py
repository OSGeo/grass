"""GRASS GIS Simple Python Editor

Copyright (C) 2016 by the GRASS Development Team

This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS GIS
for details.

:authors: Vaclav Petras
:authors: Martin Landa
"""

import sys
import os
import stat
from StringIO import StringIO
import time

import wx

import grass.script as gscript
from grass.script.utils import try_remove

# just for testing
if __name__ == '__main__':
    from grass.script.setup import set_gui_path
    set_gui_path()

from core.utils import _
from core.gcmd import EncodeString, GError
from core.giface import StandaloneGrassInterface
from gui_core.pystc import PyStc
from core import globalvar
from core.menutree import MenuTreeModelBuilder
from gui_core.menu import Menu
from gui_core.toolbars import BaseToolbar, BaseIcons
from icons.icon import MetaIcon

# TODO: add validation: call/import pep8 (error message if not available)
# TODO: run with parameters
# TODO: run with overwrite (in process env, not os.environ)
# TODO: add more examples (separate file)
# TODO: add test for templates and examples
# TODO: add pep8 test for templates and examples
# TODO: add snippets?


def script_template():
    """The most simple script which runs and gives something"""
    return r"""#!/usr/bin/env python

import grass.script as gscript


def main():
    gscript.run_command('g.region', flags='p')


if __name__ == '__main__':
    main()
"""


def module_template():
    """Template from which to start writing GRASS module"""
    import getpass
    author = getpass.getuser()

    properties = {}
    properties['name'] = 'module name'
    properties['author'] = author
    properties['description'] = 'Module description'

    output = StringIO()
    # header
    output.write(
        r"""#!/usr/bin/env python
#
#%s
#
# MODULE:       %s
#
# AUTHOR(S):    %s
#
# PURPOSE:      %s
#
# DATE:         %s
#
#%s
""" % ('#' * 72,
       EncodeString(properties['name']),
       EncodeString(properties['author']),
       EncodeString('\n# '.join(properties['description'].splitlines())),
       time.asctime(),
       '#' * 72))

    # UI
    output.write(
        r"""
#%%module
#%% description: %s
#%%end
""" % (EncodeString(' '.join(properties['description'].splitlines()))))

    # import modules
    output.write(
        r"""
import sys
import os
import atexit

import grass.script as gscript
""")

    # cleanup()
    output.write(
        r"""
RAST_REMOVE = []

def cleanup():
""")
    output.write(
        r"""    gscript.run_command('g.remove', flags='f', type='raster',
                          name=RAST_REMOVE)
""")
    output.write("\ndef main():\n")
    output.write(
        r"""    options, flags = gscript.parser()
    gscript.run_command('g.remove', flags='f', type='raster',
                        name=RAST_REMOVE)
""")

    output.write("\n    return 0\n")

    output.write(
        r"""
if __name__ == "__main__":
    atexit.register(cleanup)
    sys.exit(main())
""")
    return output.getvalue()


def script_example():
    """Example of a simple script"""
    return r"""#!/usr/bin/env python

import grass.script as gscript

def main():
    input_raster = 'elevation'
    output_raster = 'high_areas'
    stats = gscript.parse_command('r.univar', map='elevation', flags='g')
    raster_mean = float(stats['mean'])
    raster_stddev = float(stats['stddev'])
    raster_high = raster_mean + raster_stddev
    gscript.mapcalc('{r} = {a} > {m}'.format(r=output_raster, a=input_raster,
                                             m=raster_high))

if __name__ == "__main__":
    main()
"""


def module_example():
    """Example of a GRASS module"""
    return r"""#!/usr/bin/env python

#%module
#% description: Adds the values of two rasters (A + B)
#% keyword: raster
#% keyword: algebra
#% keyword: sum
#%end
#%option G_OPT_R_INPUT
#% key: araster
#% description: Name of input raster A in an expression A + B
#%end
#%option G_OPT_R_INPUT
#% key: braster
#% description: Name of input raster B in an expression A + B
#%end
#%option G_OPT_R_OUTPUT
#%end


import sys

import grass.script as gscript


def main():
    options, flags = gscript.parser()
    araster = options['araster']
    braster = options['braster']
    output = options['output']

    gscript.mapcalc('{r} = {a} + {b}'.format(r=output, a=araster, b=braster))

    return 0


if __name__ == "__main__":
    sys.exit(main())
"""


class PyEditController(object):
    # using the naming GUI convention, change for controller?
    # pylint: disable=invalid-name

    def __init__(self, panel, guiparent, giface):
        """Simple editor, this class could be a pure controller"""
        self.guiparent = guiparent
        self.giface = giface
        self.body = panel
        self.filename = None
        self.tempfile = None  # bool, make them strings for better code
        self.running = False

    def OnRun(self, event):
        """Run Python script"""
        if self.running:
            # ignore when already running
            return

        if not self.filename:
            self.filename = gscript.tempfile()
            self.tempfile = True
            try:
                fd = open(self.filename, "w")
                fd.write(self.body.GetText())
            except IOError as e:
                GError(_("Unable to launch Python script. %s") % e,
                       parent=self.guiparent)
                return
            finally:
                fd.close()
                mode = stat.S_IMODE(os.lstat(self.filename)[stat.ST_MODE])
                os.chmod(self.filename, mode | stat.S_IXUSR)
        else:
            fd = open(self.filename, "w")
            try:
                fd.write(self.body.GetText())
            finally:
                fd.close()
            # set executable file
            # (not sure if needed every time but useful for opened files)
            os.chmod(self.filename, stat.S_IRWXU | stat.S_IWUSR)

        # TODO: add overwrite to toolbar, needs env in GConsole
        # run in console as other modules, avoid Python shell which
        # carries variables over to the next execution
        self.giface.RunCmd([fd.name], skipInterface=True, onDone=self.OnDone)
        self.running = True

    def OnDone(self, event):
        """Python script finished"""
        if self.tempfile:
            try_remove(self.filename)
            self.filename = None
        self.running = False

    def SaveAs(self):
        """Save python script to file"""
        filename = ''
        dlg = wx.FileDialog(parent=self.guiparent,
                            message=_("Choose file to save"),
                            defaultDir=os.getcwd(),
                            wildcard=_("Python script (*.py)|*.py"),
                            style=wx.FD_SAVE)

        if dlg.ShowModal() == wx.ID_OK:
            filename = dlg.GetPath()

        if not filename:
            return

        # check for extension
        if filename[-3:] != ".py":
            filename += ".py"

        if os.path.exists(filename):
            dlg = wx.MessageDialog(
                parent=self.guiparent,
                message=_("File <%s> already exists. "
                          "Do you want to overwrite this file?") % filename,
                caption=_("Save file"),
                style=wx.YES_NO | wx.YES_DEFAULT | wx.ICON_QUESTION)
            if dlg.ShowModal() == wx.ID_NO:
                dlg.Destroy()
                return

            dlg.Destroy()

        self.filename = filename
        self.tempfile = False
        self.Save()

    def Save(self):
        """Save current content to a file and set executable permissions"""
        assert self.filename
        fd = open(self.filename, "w")
        try:
            fd.write(self.body.GetText())
        finally:
            fd.close()

        # executable file
        os.chmod(self.filename, stat.S_IRWXU | stat.S_IWUSR)

    def OnSave(self, event):
        """Save python script to file

        Just save if file already specified, save as action otherwise.
        """
        if self.filename:
            self.Save()
        else:
            self.SaveAs()

    # TODO: it should be probably used with replacing, when this gives what we want?
    def IsModified(self):
        """Check if python script has been modified"""
        return self.body.modified

    def Open(self):
        """Ask for a filename and load its content"""
        filename = ''
        dlg = wx.FileDialog(parent=self.guiparent,
                            message=_("Open file"),
                            defaultDir=os.getcwd(),
                            wildcard=_("Python script (*.py)|*.py"),
                            style=wx.OPEN)

        if dlg.ShowModal() == wx.ID_OK:
            filename = dlg.GetPath()

        if not filename:
            return

        fd = open(filename, "r")
        try:
            self.body.SetText(fd.read())
        finally:
            fd.close()

        self.filename = filename
        self.tempfile = False

    def OnOpen(self, event):
        if self.CanReplaceContent('file'):
            self.Open()

    def IsEmpty(self):
        """Check if python script is empty"""
        return len(self.body.GetText()) == 0

    def SetScriptTemplate(self, event):
        if self.CanReplaceContent('template'):
            self.body.SetText(script_template())

    def SetModuleTemplate(self, event):
        if self.CanReplaceContent('template'):
            self.body.SetText(module_template())

    def SetScriptExample(self, event):
        if self.CanReplaceContent('template'):
            self.body.SetText(script_example())

    def SetModuleExample(self, event):
        if self.CanReplaceContent('template'):
            self.body.SetText(module_example())

    def CanReplaceContent(self, by_message):
        if by_message == 'template':
            message = _("Replace the content by the template?")
        elif by_message == 'file':
            message = _("Replace the current content by the file content?")
        else:
            message = by_message
        if not self.IsEmpty():
            dlg = wx.MessageDialog(
                parent=self.guiparent, message=message,
                caption=_("Replace content"),
                style=wx.YES_NO | wx.YES_DEFAULT | wx.ICON_QUESTION)
            if dlg.ShowModal() == wx.ID_NO:
                dlg.Destroy()
                return False
            dlg.Destroy()
        return True

    def OnHelp(self, event):
        import webbrowser

        # inspired by g.manual but simple not using GRASS_HTML_BROWSER
        # not using g.manual because it does not show
        entry = 'libpython/script_intro.html'
        major, minor, patch = gscript.version()['version'].split('.')
        url = 'http://grass.osgeo.org/grass%s%s/manuals/%s' % (
            major, minor, entry)
        webbrowser.open(url)

    def OnPythonHelp(self, event):
        import webbrowser

        url = 'https://docs.python.org/%s/tutorial/' % sys.version_info[0]
        webbrowser.open(url)

    def OnModulesHelp(self, event):
        self.giface.Help('full_index')

    def OnAddonsHelp(self, event):
        import webbrowser

        url = 'https://grass.osgeo.org/development/code-submission/'
        webbrowser.open(url)

    def OnSupport(self, event):
        import webbrowser

        url = 'https://grass.osgeo.org/support/'
        webbrowser.open(url)


class PyEditToolbar(BaseToolbar):
    # GUI class
    # pylint: disable=too-many-ancestors
    # pylint: disable=too-many-public-methods
    """PyEdit toolbar"""
    def __init__(self, parent):
        BaseToolbar.__init__(self, parent)

        # workaround for http://trac.wxwidgets.org/ticket/13888
        if sys.platform == 'darwin':
            parent.SetToolBar(self)

        self.InitToolbar(self._toolbarData())

        # realize the toolbar
        self.Realize()

    def _toolbarData(self):
        """Toolbar data"""
        icons = {
            'open': MetaIcon(img='open',
                             label=_('Open (Ctrl+O)')),
            'save': MetaIcon(img='save',
                             label=_('Save (Ctrl+S)')),
            'run': MetaIcon(img='execute',
                            label=_('Run (Ctrl+R)')),
        }

        return self._getToolbarData((('open', icons['open'],
                                      self.parent.OnOpen),
                                     ('save', icons['save'],
                                      self.parent.OnSave),
                                     (None, ),
                                     ('run', icons['run'],
                                      self.parent.OnRun),
                                     (None, ),
                                     ("help", BaseIcons['help'],
                                      self.parent.OnHelp),
                                    ))


class PyEditFrame(wx.Frame):
    # GUI class and a lot of trampoline methods
    # pylint: disable=missing-docstring
    # pylint: disable=too-many-public-methods
    # pylint: disable=invalid-name
    def __init__(self, parent, giface, id=wx.ID_ANY,
                 title=_("GRASS GIS Simple Python Editor"),
                 **kwargs):
        wx.Frame.__init__(self, parent=parent, id=id, title=title, **kwargs)
        self.parent = parent

        filename = os.path.join(
            globalvar.WXGUIDIR, 'xml', 'menudata_pyedit.xml')
        self.menubar = Menu(
            parent=self,
            model=MenuTreeModelBuilder(filename).GetModel(separators=True))
        self.SetMenuBar(self.menubar)

        self.toolbar = PyEditToolbar(parent=self)
        # workaround for http://trac.wxwidgets.org/ticket/13888
        # TODO: toolbar is set in toolbar and here
        if sys.platform != 'darwin':
            self.SetToolBar(self.toolbar)

        self.panel = PyStc(parent=self)
        self.controller = PyEditController(
            panel=self.panel, guiparent=self, giface=giface)

        # don't start with an empty page
        self.panel.SetText(script_template())

        sizer = wx.BoxSizer(wx.VERTICAL)
        sizer.Add(item=self.panel, proportion=1,
                  flag=wx.EXPAND)
        sizer.Fit(self)
        sizer.SetSizeHints(self)
        self.SetSizer(sizer)
        self.Fit()
        self.SetAutoLayout(True)
        self.Layout()

    # TODO: it would be nice if we can pass the controller to the menu
    # might not be possible on the side of menu
    # here we use self and self.controller which might make it harder
    def OnOpen(self, *args, **kwargs):
        self.controller.OnOpen(*args, **kwargs)

    def OnSave(self, *args, **kwargs):
        self.controller.OnSave(*args, **kwargs)

    def OnClose(self, *args, **kwargs):
        # saves without asking if we have an open file
        self.controller.OnSave(*args, **kwargs)
        self.Destroy()

    def OnRun(self, *args, **kwargs):
        # save without asking
        self.controller.OnRun(*args, **kwargs)

    def OnHelp(self, *args, **kwargs):
        # save without asking
        self.controller.OnHelp(*args, **kwargs)

    def OnSimpleScriptTemplate(self, *args, **kwargs):
        self.controller.SetScriptTemplate(*args, **kwargs)

    def OnGrassModuleTemplate(self, *args, **kwargs):
        self.controller.SetModuleTemplate(*args, **kwargs)

    def OnSimpleScriptExample(self, *args, **kwargs):
        self.controller.SetScriptExample(*args, **kwargs)

    def OnGrassModuleExample(self, *args, **kwargs):
        self.controller.SetModuleExample(*args, **kwargs)

    def OnPythonHelp(self, *args, **kwargs):
        self.controller.OnPythonHelp(*args, **kwargs)

    def OnModulesHelp(self, *args, **kwargs):
        self.controller.OnModulesHelp(*args, **kwargs)

    def OnAddonsHelp(self, *args, **kwargs):
        self.controller.OnAddonsHelp(*args, **kwargs)

    def OnSupport(self, *args, **kwargs):
        self.controller.OnSupport(*args, **kwargs)


def main():
    """Test application (potentially useful as g.gui.pyedit)"""
    app = wx.App()
    giface = StandaloneGrassInterface()
    simple_editor = PyEditFrame(parent=None, giface=giface)
    simple_editor.SetSize((600, 800))
    simple_editor.Show()
    app.MainLoop()


if __name__ == '__main__':
    main()
