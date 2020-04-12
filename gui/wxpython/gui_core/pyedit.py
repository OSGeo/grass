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
try:
    from StringIO import StringIO
except ImportError:
    from io import StringIO
import time

import wx

import grass.script as gscript
from grass.script.utils import try_remove

# needed just for testing
if __name__ == '__main__':
    from grass.script.setup import set_gui_path
    set_gui_path()

from core.gcmd import GError
from gui_core.pystc import PyStc
from core import globalvar
from core.menutree import MenuTreeModelBuilder
from gui_core.menu import Menu
from gui_core.toolbars import BaseToolbar, BaseIcons
from icons.icon import MetaIcon
from core.debug import Debug

# TODO: add validation: call/import pep8 (error message if not available)
# TODO: run with parameters (alternatively, just use console or GUI)
# TODO: add more examples (better separate file)
# TODO: add test for templates and examples
# TODO: add pep8 test for templates and examples
# TODO: add snippets?


def script_template():
    """The most simple script which runs and gives something"""
    return r"""#!/usr/bin/env python3

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
        r"""#!/usr/bin/env python3
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
            properties['name'],
            properties['author'],
            '\n# '.join(properties['description'].splitlines()),
            time.asctime(),
            '#' * 72))

    # UI
    output.write(
        r"""
#%%module
#%% description: %s
#%%end
""" % (' '.join(properties['description'].splitlines())))

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
    return r"""#!/usr/bin/env python3

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
    return r"""#!/usr/bin/env python3

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


def module_error_handling_example():
    """Example of a GRASS module"""
    return r"""#!/usr/bin/env python3

#%module
#% description: Selects values from raster above value of mean plus standard deviation
#% keyword: raster
#% keyword: select
#% keyword: standard deviation
#%end
#%option G_OPT_R_INPUT
#%end
#%option G_OPT_R_OUTPUT
#%end


import sys

import grass.script as gscript
from grass.exceptions import CalledModuleError


def main():
    options, flags = gscript.parser()
    input_raster = options['input']
    output_raster = options['output']

    try:
        stats = gscript.parse_command('r.univar', map=input_raster, flags='g')
    except CalledModuleError as e:
        gscript.fatal('{0}'.format(e))
    raster_mean = float(stats['mean'])
    raster_stddev = float(stats['stddev'])
    raster_high = raster_mean + raster_stddev
    gscript.mapcalc('{r} = {i} > {v}'.format(r=output_raster, i=input_raster,
                                             v=raster_high))
    return 0


if __name__ == "__main__":
    sys.exit(main())
"""


def open_url(url):
    import webbrowser
    webbrowser.open(url)


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
        self.overwrite = False
        self.parameters = None

    def OnRun(self, event):
        """Run Python script"""
        if not self.filename:
            self.filename = gscript.tempfile() + '.py'
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
            # always save automatically before running
            fd = open(self.filename, "w")
            try:
                fd.write(self.body.GetText())
            finally:
                fd.close()
            # set executable file
            # (not sure if needed every time but useful for opened files)
            os.chmod(self.filename, stat.S_IRWXU | stat.S_IWUSR)

        # run in console as other modules, avoid Python shell which
        # carries variables over to the next execution
        env = os.environ.copy()
        if self.overwrite:
            env['GRASS_OVERWRITE'] = '1'
        cmd = [fd.name]
        if self.parameters:
            cmd.extend(self.parameters)
        self.giface.RunCmd(cmd, env=env)

    def SaveAs(self):
        """Save python script to file"""
        if self.tempfile:
            try_remove(self.filename)
            self.tempfile = False

        filename = None
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
        if self.filename and not self.tempfile:
            self.Save()
        else:
            self.SaveAs()

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
                            style=wx.FD_OPEN)

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
        """Handle open event but ask about replacing content first"""
        if self.CanReplaceContent('file'):
            self.Open()

    def IsEmpty(self):
        """Check if python script is empty"""
        return len(self.body.GetText()) == 0

    def IsContentValuable(self):
        """Check if content of the editor is valuable to user

        Used for example to check if content should be saved before closing.
        The content is not valuable for example if it already saved in a file.
        """
        Debug.msg(2, "pyedit IsContentValuable? empty=%s, modified=%s" % (
                  self.IsEmpty(), self.IsModified()))
        return not self.IsEmpty() and self.IsModified()

    def SetScriptTemplate(self, event):
        if self.CanReplaceContent('template'):
            self.body.SetText(script_template())

    def SetModuleTemplate(self, event):
        if self.CanReplaceContent('template'):
            self.body.SetText(module_template())

    def SetScriptExample(self, event):
        if self.CanReplaceContent('example'):
            self.body.SetText(script_example())

    def SetModuleExample(self, event):
        if self.CanReplaceContent('example'):
            self.body.SetText(module_example())

    def SetModuleErrorHandlingExample(self, event):
        if self.CanReplaceContent('example'):
            self.body.SetText(module_error_handling_example())

    def CanReplaceContent(self, by_message):
        """Check with user if we can replace content by something else

        Asks user if replacement is OK depending on the state of the editor.
        Use before replacing all editor content by some other text.
        
        :param by_message: message used to ask user if it is OK to replace
            the content with something else; special values are 'template',
            'example' and 'file' which will use predefined messages, otherwise
            a translatable, user visible string should be used.
        """
        if by_message == 'template':
            message = _("Replace the content by the template?")
        elif by_message == 'example':
            message = _("Replace the content by the example?")
        elif by_message == 'file':
            message = _("Replace the current content by the file content?")
        else:
            message = by_message
        if self.IsContentValuable():
            dlg = wx.MessageDialog(
                parent=self.guiparent, message=message,
                caption=_("Replace content"),
                style=wx.YES_NO | wx.YES_DEFAULT | wx.ICON_QUESTION)
            if dlg.ShowModal() == wx.ID_NO:
                dlg.Destroy()
                return False
            dlg.Destroy()
        return True

    def OnSetParameters(self, event):
        """Handle setting CLI parameters for the script (asks for input)"""
        dlg = wx.TextEntryDialog(
            parent=self.guiparent,
            caption=_("Set parameters for the script"),
            message=_("Specify command line parameters for the script separated by spaces:"),
            )
        if self.parameters:
            dlg.SetValue(" ".join(self.parameters))
        # TODO: modality might not be needed here if we bind the events
        if dlg.ShowModal() == wx.ID_OK:
            text = dlg.GetValue().strip()
            # TODO: split in the same way as in console
            if text:
                self.parameters = text.split()
            else:
                self.parameters = None

    def OnHelp(self, event):
        # inspired by g.manual but simple not using GRASS_HTML_BROWSER
        # not using g.manual because it does not show
        entry = 'libpython/script_intro.html'
        major, minor, patch = gscript.version()['version'].split('.')
        url = 'http://grass.osgeo.org/grass%s%s/manuals/%s' % (
            major, minor, entry)
        open_url(url)

    def OnPythonHelp(self, event):
        url = 'https://docs.python.org/%s/tutorial/' % sys.version_info[0]
        open_url(url)

    def OnModulesHelp(self, event):
        self.giface.Help('full_index')

    def OnSubmittingHelp(self, event):
        open_url('https://trac.osgeo.org/grass/wiki/Submitting/Python')

    def OnAddonsHelp(self, event):
        open_url('https://grass.osgeo.org/development/code-submission/')

    def OnSupport(self, event):
        open_url('https://grass.osgeo.org/support/')


class PyEditToolbar(BaseToolbar):
    # GUI class
    # pylint: disable=too-many-ancestors
    # pylint: disable=too-many-public-methods
    """PyEdit toolbar"""

    def __init__(self, parent):
        BaseToolbar.__init__(self, parent)

        self.icons = {
            'open': MetaIcon(img='open',
                             label=_('Open (Ctrl+O)')),
            'save': MetaIcon(img='save',
                             label=_('Save (Ctrl+S)')),
            'run': MetaIcon(img='execute',
                            label=_('Run (Ctrl+R)')),
            # TODO: better icons for overwrite modes
            'overwriteTrue': MetaIcon(img='locked',
                                      label=_('Activate overwrite')),
            'overwriteFalse': MetaIcon(img='unlocked',
                                       label=_('Deactive overwrite')),
        }

        # workaround for http://trac.wxwidgets.org/ticket/13888
        if sys.platform == 'darwin':
            parent.SetToolBar(self)

        self.InitToolbar(self._toolbarData())

        # realize the toolbar
        self.Realize()

    def _toolbarData(self):
        """Toolbar data"""
        return self._getToolbarData((('open', self.icons['open'],
                                      self.parent.OnOpen),
                                     ('save', self.icons['save'],
                                      self.parent.OnSave),
                                     (None, ),
                                     ('run', self.icons['run'],
                                      self.parent.OnRun),
                                     ('overwrite', self.icons['overwriteTrue'],
                                      self.OnSetOverwrite, wx.ITEM_CHECK),
                                     (None, ),
                                     ("help", BaseIcons['help'],
                                      self.parent.OnHelp),
                                     ))

    # TODO: add overwrite also to the menu and sync with toolbar
    def OnSetOverwrite(self, event):
        if self.GetToolState(self.overwrite):
            self.SetToolNormalBitmap(self.overwrite,
                                     self.icons['overwriteFalse'].GetBitmap())
            self.SetToolShortHelp(self.overwrite,
                                  self.icons['overwriteFalse'].GetLabel())
            self.parent.overwrite = True
        else:
            self.SetToolNormalBitmap(self.overwrite,
                                     self.icons['overwriteTrue'].GetBitmap())
            self.SetToolShortHelp(self.overwrite,
                                  self.icons['overwriteTrue'].GetLabel())
            self.parent.overwrite = False


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
        sizer.Add(self.panel, proportion=1,
                  flag=wx.EXPAND)
        sizer.Fit(self)
        sizer.SetSizeHints(self)
        self.SetSizer(sizer)
        self.Fit()
        self.SetAutoLayout(True)
        self.Layout()
        self.Bind(wx.EVT_CLOSE, self.OnClose)

    # TODO: it would be nice if we can pass the controller to the menu
    # might not be possible on the side of menu
    # here we use self and self.controller which might make it harder
    def OnOpen(self, *args, **kwargs):
        self.controller.OnOpen(*args, **kwargs)

    def OnSave(self, *args, **kwargs):
        self.controller.OnSave(*args, **kwargs)

    def OnClose(self, *args, **kwargs):
        # this will be often true because PyStc is using EVT_KEY_DOWN
        # to say if it was modified, not actual user change in text
        if self.controller.IsContentValuable():
            self.controller.OnSave(*args, **kwargs)
        self.Destroy()

    def OnRun(self, *args, **kwargs):
        # save without asking
        self.controller.OnRun(*args, **kwargs)

    def OnHelp(self, *args, **kwargs):
        self.controller.OnHelp(*args, **kwargs)

    def OnSimpleScriptTemplate(self, *args, **kwargs):
        self.controller.SetScriptTemplate(*args, **kwargs)

    def OnGrassModuleTemplate(self, *args, **kwargs):
        self.controller.SetModuleTemplate(*args, **kwargs)

    def OnSimpleScriptExample(self, *args, **kwargs):
        self.controller.SetScriptExample(*args, **kwargs)

    def OnGrassModuleExample(self, *args, **kwargs):
        self.controller.SetModuleExample(*args, **kwargs)

    def OnGrassModuleErrorHandlingExample(self, *args, **kwargs):
        self.controller.SetModuleErrorHandlingExample(*args, **kwargs)

    def OnPythonHelp(self, *args, **kwargs):
        self.controller.OnPythonHelp(*args, **kwargs)

    def OnModulesHelp(self, *args, **kwargs):
        self.controller.OnModulesHelp(*args, **kwargs)

    def OnSubmittingHelp(self, *args, **kwargs):
        self.controller.OnSubmittingHelp(*args, **kwargs)

    def OnAddonsHelp(self, *args, **kwargs):
        self.controller.OnAddonsHelp(*args, **kwargs)

    def OnSupport(self, *args, **kwargs):
        self.controller.OnSupport(*args, **kwargs)

    def _get_overwrite(self):
        return self.controller.overwrite

    def _set_overwrite(self, overwrite):
        self.controller.overwrite = overwrite

    overwrite = property(_get_overwrite, _set_overwrite,
                         doc="Tells if overwrite should be used")

    def OnSetParameters(self, *args, **kwargs):
        self.controller.OnSetParameters(*args, **kwargs)


def main():
    """Test application (potentially useful as g.gui.pyedit)"""
    from core.giface import StandaloneGrassInterface

    app = wx.App()
    giface = StandaloneGrassInterface()
    simple_editor = PyEditFrame(parent=None, giface=giface)
    simple_editor.SetSize((600, 800))
    simple_editor.Show()
    app.MainLoop()


if __name__ == '__main__':
    main()
