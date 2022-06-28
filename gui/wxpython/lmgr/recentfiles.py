"""
@package lmgr.recentfiles

@brief wxGUI Layer Manager - recent files submenu

Classes:
 - recentfiles::RecentFilesMixin

(C) 2022 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Tomas Zigo <tomas.zigo slovanet.sk>
"""

from core.gcmd import GError
from gui_core.menu import RecentFilesMenu


class RecentFilesMixin:
    """Recent files mixin class for single and multiple window mode"""

    def _createWorkspaceRecentFilesMenu(self):
        """Create workspace recent files menu"""
        file_menu = self.menubar.GetMenu(
            menuIndex=self.menubar.FindMenu(title="File"),
        )
        workspace_item = file_menu.FindItem(
            id=file_menu.FindItem(itemString="Workspace"),
        )[0]
        self.recent_files = RecentFilesMenu(
            app_name="main",
            parent_menu=workspace_item.GetSubMenu(),
            pos=0,
        )
        self.recent_files.file_requested.connect(self.OpenRecentFile)

    def AddFileToHistory(self, file_path):
        """Add file to history (recent files)

        :param str file_path: file path
        """
        self.recent_files.AddFileToHistory(filename=file_path)

    def OpenRecentFile(self, path, file_exists, file_history):
        """Try open recent file and read content

        :param str path: file path
        :param bool file_exists: file path exists
        :param bool file_history: file history obj instance

        :return: None
        """
        if not file_exists:
            GError(
                _(
                    "File <{}> doesn't exist."
                    " It was probably moved or deleted.".format(path)
                ),
                parent=self,
            )
        else:
            self.workspace_manager.Close()
            self.workspace_manager.loadingWorkspace = True
            self.workspace_manager.Load(path)
            self.workspace_manager.loadingWorkspace = False
            self._setTitle()
            file_history.AddFileToHistory(filename=path)  # move up the list
