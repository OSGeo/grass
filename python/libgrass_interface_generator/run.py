#!/usr/bin/env python3

import sys
import os

THIS_DIR = os.path.dirname(__file__)
# ensure that we can load the ctypesgen library
sys.path.insert(0, THIS_DIR)

import ctypesgen.main  # noqa: E402

if __name__ == "__main__":
    ctypesgen.main.main()
