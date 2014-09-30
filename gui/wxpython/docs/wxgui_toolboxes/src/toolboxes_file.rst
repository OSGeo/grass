Toolboxes file
==============

This file contains definition of toolboxes. A toolbox contains references (links) to other items, namely
``"<module-items>"``
,
``"<wxgui-items>"``
and other toolboxes using tag
``"<subtoolbox>"``
. Tag
``"<separator>"``
is used when the view supports some kind of visual separators to group parts of the toolbox (or menu).

Items are referenced using
``name``
attribute. In case of
``"<module-items>"``
,
``"<wxgui-items>"``
also subelements can be added to create new items or to replace subelements values from item definition.

|toolboxes_filestructure.png|


.. |toolboxes_filestructure.png| image:: toolboxes_filestructure.png
    :width: 6.9252in
    :height: 6.2957in

