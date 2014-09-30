Generation of files and menu
============================

As noted in the section
`Files overview <index.htmlsOverview>`_
, there are files in the GRASS distribution and in the user home directory (particularly in
``".grass7/toolboxes"``
subdirectory).

When user doesn't have any
``toolboxes.xml``
or
``main_menu.xml``
files in the home directory, file
``menudata.xml``
included in the distribution is used to build a menu.

When
``toolboxes.xml``
or
``main_menu.xml``
file (in user home directory) is newer than
``menudata.xml``
in user home directory or
``menudata.xml``
does not exists in user home directory, the
``menudata.xml``
is generated when GUI starts.

When
``menudata.xml``
in user home directory is fresh enough, it is used to create a menu.

When
``toolboxes.xml``
or
``main_menu.xml``
file is not in user home directory but
``menudata.xml``
is, the file is re-generated (each time the GUI starts). So, if you just have your own
``main_menu.xml``
, it is better to create also a
``toolboxes.xml``
file with no toolboxes (note that you still need to create a valid XML toolbox file). Similarly, if you have only the
``toolboxes.xml``
file, it is better to copy the
``main_menu.xml``
file from distribution into your home directory.

When reading the main_menu file, user toolboxes are expanded first and then toolboxes from distribution are expanded.
