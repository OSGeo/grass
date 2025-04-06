#!/usr/bin/env python3

import os
import sys
import unittest

# Add the GRASS package to Python path
current_dir = os.path.dirname(os.path.abspath(__file__))
grass_dir = os.path.abspath(os.path.join(current_dir, "../../../../../"))
sys.path.insert(0, grass_dir)

# Import and run tests
from test_history_features import TestHistoryFeatures

if __name__ == "__main__":
    unittest.main() 