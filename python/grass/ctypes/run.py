#!/usr/bin/env python3

import sys, os

THIS_DIR = os.path.dirname(__file__)
# ensure that we can load the ctypesgen library
sys.path.insert(0, THIS_DIR)

import ctypesgen.main

if __name__ == "__main__":
    ctypesgen.main.main()
