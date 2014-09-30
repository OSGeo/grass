Modules items file
==================

The file contains information obtained from modules' interface descriptions. The structure of one
``"module-item"``
is the same as in the
`Toolboxes file <toolboxes_file.html>`_
but some subelements are mandatory.

File contained in distribution is generated during compilation from available modules using the script
``gui/wxpython/tools/build_modules_xml.py``
.

Element
``"<module>"``
is the name of the executable, e.g. "r.info".

Element
``"<label>"``
is currently not present. It represents the short label in menu and it is added in toolboxes.

Element
``"<description>"``
is created from module's module->description (or if it exists, module->label concatenated with module->description).

Element
``"<keywords>"``
is created from module's module->keywords.
