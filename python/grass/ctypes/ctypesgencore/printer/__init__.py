#!/usr/bin/env python3

"""
This module is the backend to ctypesgen; it contains classes to
produce the final .py output files.
"""

from .printer import WrapperPrinter

__all__ = ["WrapperPrinter"]
