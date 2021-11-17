# MODULE:    grass
#
# AUTHOR(S): Vaclav Petras <wenzeslaus gmail com>
#
# PURPOSE:   Top level file of the grass package and its initialization
#
# COPYRIGHT: (C) 2021 Vaclav Petras, and by the GRASS Development Team
#
#           This program is free software under the GNU General Public
#           License (>=v2). Read the file COPYING that comes with GRASS
#           for details.

"""Top-level GRASS GIS Python package

Importing the package (or any subpackage) initializes translation functions
so that the function ``_`` appears in the global namespace (as an additional build-in).
"""

import builtins as _builtins
import os


# Setup translations
#
# The translations in the GRASS GIS Python package grass, GRASS Python modules
# (scripts), and the wxPython GUI (wxGUI) are handled as application translations,
# rather than Python module or package translations. This means that that translation
# function called `_` (underscore) is added to buildins namespace. When the grass
# package or any subpackage (or an object from there) is imported, the `_` function
# becomes available globally (i.e. for all Python code in the same process). This is
# the good part.

# Unfortunately, creating `_` in the global namespace has unitended consequences such
# as issues in interactive shells, with doctests, and it is conflicting with a common
# practice of using `_` for unused variables. This is the same behavior as with the
# common function `gettext.install()` which also adds `_` into the global namespace.
# The solution to this is to use the module translations approach instead of
# application translation approach. Without changing the overall approach, the current
# code can be modified to allow for using imports of translation functions instead of
# relying on the buildins when desired as a transitional state before removing the
# modification of buildins.
#
# The current code mitigates two other issues associated with `gettext.install()`
# approach: First, it delays initialization of translations to the time when they are
# needed instead of doing it during import time (and possibly failing when
# environmental variables pointing to the source file are not set properly). Second,
# it adds multiple domains as fallback unlike `gettext.install()` which simply uses
# the last used domain.
#
# For more info, please check the following links:
# - https://docs.python.org/3/library/gettext.html#gettext.translation
# - https://github.com/python/cpython/blob/main/Lib/gettext.py (esp. install function)
# - https://pymotw.com/3//gettext/index.html#application-vs-module-localization
# - https://www.wefearchange.org/2012/06/the-right-way-to-internationalize-your.html


def _import_user_settings():
    """Import UserSettings

    Required for getting user defined language.
    """
    try:
        import gettext
        from grass.script.setup import set_gui_path

        set_gui_path()
        # Initialize translatation func before import UserSettings
        _builtins.__dict__["_"] = gettext.gettext
        global _UserSettings
        from core.settings import UserSettings as _UserSettings
    except ModuleNotFoundError:  # during compilation, and tests (without wxGUI)
        pass


def _translate(text):
    """Get translated version of text

    The first call to this function initializes translations, i.e., simply importing
    the package does not require the translations to be availabe. However, a first
    call to translate a message will do the initialization first before translating
    the message.
    """
    if _translate.translation is None:
        # Initialize translations if needed. This should happen (only) during the
        # the first call of the function.
        try:
            import gettext  # pylint: disable=import-outside-toplevel

            gisbase = os.environ["GISBASE"]
            locale_dir = os.path.join(gisbase, "locale")
            # With fallback set to True, not finding the translations files for
            # a language or domain results in a use of null translation, so this
            # does not raise an exception even if the locale settings is broken
            # or the translation files were not installed.
            fallback = True
            translation = None
            domains = ("grasslibs", "grassmods", "grasswxpy")

            def apply_trans(kwargs):
                nonlocal translation
                if kwargs["domain"] == "grasslibs":
                    translation = gettext.translation(**kwargs)
                else:
                    translation.add_fallback(gettext.translation(**kwargs))

            try:
                # Get user defined lang
                lang = _UserSettings.Get(
                    group="language",
                    key="locale",
                    subkey="lc_all",
                )
            except NameError:
                # If _UserSettings isn't imported during tests (without wxGUI)
                lang = None

            if lang and lang not in ("en", "system"):
                for domain in domains:
                    apply_trans(
                        {
                            "domain": domain,
                            "localedir": locale_dir,
                            "fallback": fallback,
                            "languages": (lang,),
                        }
                    )
            elif lang == "en":
                translation = gettext.NullTranslations()
            else:
                for domain in domains:
                    apply_trans(
                        {
                            "domain": domain,
                            "localedir": locale_dir,
                            "fallback": fallback,
                        }
                    )
            # Store the resulting translation object.
            _translate.translation = translation
        except (KeyError, ImportError):
            # If the environmental variable is not set or there is no gettext,
            # use null translation as an ultimate fallback.
            _translate.translation = gettext.NullTranslations()
    return _translate.translation.gettext(text)


# Initialize the translation attribute of the translate function to indicate
# that the translations are not initialized.
_translate.translation = None
_import_user_settings()
_builtins.__dict__["_"] = _translate


__all__ = ["script", "temporal"]
if os.path.exists(os.path.join(os.path.dirname(__file__), "lib", "__init__.py")):
    __all__.append("lib")
