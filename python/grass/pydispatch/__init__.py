"""Multi-consumer multi-producer dispatching mechanism

This Python library is used e.g. in wxGUI. The purpose of the library
is to provide mechanism for communication between objects in wxGUI.
The library consists of two parts:

* the Python Signal API which will be used in wxGUI, and
* this 3rd party package PyDispatcher does the hard work.

In short, simple function calls are not sufficient in the GUI, event
driven and large environment with many persistent objects because
using simple function calls would lead to tightly coupled code. Thus,
some better mechanism is needed such as Observer design pattern. In
GRASS GIS, we use the Signal system which is similar to Signals used in
PyQt and other frameworks. As the underlying library, we have chosen
PyDispatcher because it provides very general API which enables to
implement Signal API, wide and robust functionality which makes
implementation and use of Signals easier.

PyDispatcher metadata:

:version: 2.0.3
:author: Patrick K. O'Brien
:license: BSD-style, see license.txt for details
"""
