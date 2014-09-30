Menudata file
=============

Historically, menudata.xml file was in the source codes and was partially maintained by the script
``gui/wxpython/tools/update_menudata.py``
which updated the description and keywords (based on module's module->label or module->description, module->keywords). Other items (menu structure, menu item labels and non-module only items) were edited in the menudata.xml file directly.

Currently, the file is generated during compilation or at startup. It serves as an intermediate layer between all toolboxes XMLs and GUI menu tree generation.
