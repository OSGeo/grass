# -*- coding: utf-8 -*-


class DBError(Exception):
    pass


class FatalError(Exception):
    pass


class FlagError(Exception):
    pass


class GrassError(Exception):
    pass


class ImplementationError(Exception):
    pass


class OpenError(Exception):
    pass


class ParameterError(Exception):
    pass


class ScriptError(Exception):
    pass


class Usage(Exception):
    pass
