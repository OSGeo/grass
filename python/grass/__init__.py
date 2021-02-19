import gettext
import os

import six

# Setup i18N
#
# Calling `gettext.install()` injects `_()` in the builtins namespace and
# thus it becomes available globally (i.e. in the same process). Any python
# code that needs to use _() should make sure that it firsts imports this
# library.
#
# Note: we need to do this at the beginning of this module in order to
# ensure that the injection happens before anything else gets imported.
#
# For more info please check the following links:
# - https://docs.python.org/2/library/gettext.html#gettext.install
# - https://pymotw.com/2//gettext/index.html#application-vs-module-localization
# - https://www.wefearchange.org/2012/06/the-right-way-to-internationalize-your.html
#
_LOCALE_DIR = os.path.join(os.getenv("GISBASE"), 'locale')
if six.PY2:
    gettext.install('grasslibs', _LOCALE_DIR, unicode=True)
    gettext.install('grassmods', _LOCALE_DIR, unicode=True)
    gettext.install('grasswxpy', _LOCALE_DIR, unicode=True)
else:
    gettext.install('grasslibs', _LOCALE_DIR)
    gettext.install('grassmods', _LOCALE_DIR)
    gettext.install('grasswxpy', _LOCALE_DIR)


__all__ = ["script", "temporal"]
if os.path.exists(os.path.join(os.path.dirname(__file__), "lib", "__init__.py")):
    __all__.append("lib")
