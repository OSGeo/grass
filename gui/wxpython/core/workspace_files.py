#!/usr/bin/env python3

"""Print a list of wxGUI recent workspace files"""

from grass.script.setup import set_gui_path

set_gui_path()

from gui_core.menu import RecentFilesMenu  # noqa: E402


def main():
    RecentFilesMenu.PrintListOfRecentWorkspaceFiles()


if __name__ == "__main__":
    main()
