import os
import json
import unittest
import tempfile
import shutil
from datetime import datetime

import wx

from grass.gui.wxpython.history.browser import HistoryInfoPanel, HistoryBrowser
from grass.gui.wxpython.history.tree import HistoryBrowserNode, HistoryTreeModel, HistoryBrowserTree
from grass.script import core as gcore

class TestHistoryFeatures(unittest.TestCase):
    def setUp(self):
        self.app = wx.App()
        self.frame = wx.Frame(None)
        self.temp_dir = tempfile.mkdtemp()
        self.windows_dir = os.path.join(self.temp_dir, 'windows')
        os.makedirs(self.windows_dir)
        
        # Create test region files
        self.region1 = {
            'n': 100,
            's': 0,
            'e': 100,
            'w': 0,
            'nsres': 1,
            'ewres': 1
        }
        self.region2 = {
            'n': 200,
            's': 0,
            'e': 200,
            'w': 0,
            'nsres': 2,
            'ewres': 2
        }
        
        with open(os.path.join(self.windows_dir, 'region1.json'), 'w') as f:
            json.dump(self.region1, f)
        with open(os.path.join(self.windows_dir, 'region2.json'), 'w') as f:
            json.dump(self.region2, f)
            
        # Set GISBASE for testing
        os.environ['GISBASE'] = self.temp_dir

    def tearDown(self):
        shutil.rmtree(self.temp_dir)
        self.frame.Destroy()
        self.app.Destroy()

    def test_region_name_display(self):
        """Test region name display functionality"""
        panel = HistoryInfoPanel(self.frame, giface=None)
        
        # Test with matching region
        panel.updateRegionSettings(self.region1)
        self.assertEqual(panel.region_name, 'region1')
        
        # Test with non-matching region
        non_matching_region = {
            'n': 300,
            's': 0,
            'e': 300,
            'w': 0,
            'nsres': 3,
            'ewres': 3
        }
        panel.updateRegionSettings(non_matching_region)
        self.assertIsNone(panel.region_name)
        
        # Test with None region
        panel.updateRegionSettings(None)
        self.assertIsNone(panel.region_name)

    def test_error_message_display(self):
        """Test error message display functionality"""
        panel = HistoryInfoPanel(self.frame, giface=None)
        
        # Test showing error
        error_msg = "Test error message"
        panel.showError(error_msg)
        self.assertEqual(panel.error_message, error_msg)
        self.assertTrue(panel.error_box.IsShown())
        
        # Test hiding error
        panel.hideError()
        self.assertIsNone(panel.error_message)
        self.assertFalse(panel.error_box.IsShown())

    def test_history_node_error(self):
        """Test error handling in history nodes"""
        node = HistoryBrowserNode()
        
        # Test setting error message
        error_msg = "Node error message"
        node.set_error_message(error_msg)
        self.assertEqual(node.get_error_message(), error_msg)
        
        # Test error button creation
        button = node.create_error_button(self.frame)
        self.assertIsNotNone(button)
        self.assertEqual(button.GetForegroundColour(), wx.RED)

    def test_history_tree_error_display(self):
        """Test error display in history tree"""
        tree = HistoryBrowserTree(self.frame, infoPanel=None, giface=None)
        
        # Create test data
        test_data = {
            'name': 'test_command',
            'timestamp': datetime.now().isoformat(),
            'status': 'failed',
            'error_message': 'Command failed'
        }
        
        # Test node creation with error
        node = HistoryBrowserNode(data=test_data)
        self.assertEqual(node.get_error_message(), 'Command failed')
        
        # Test tree model update
        model = HistoryTreeModel(data=[test_data])
        self.assertEqual(model.root.children[0].get_error_message(), 'Command failed')

    def test_command_info_update(self):
        """Test complete command info update"""
        panel = HistoryInfoPanel(self.frame, giface=None)
        
        # Test with all data
        command_data = {
            'timestamp': datetime.now().isoformat(),
            'runtime': 1.5,
            'status': 'success',
            'region': self.region1,
            'error_message': None
        }
        panel.updateCommandInfo(command_data)
        self.assertEqual(panel.region_name, 'region1')
        self.assertIsNone(panel.error_message)
        
        # Test with error
        command_data['error_message'] = 'Command failed'
        panel.updateCommandInfo(command_data)
        self.assertEqual(panel.error_message, 'Command failed')
        self.assertTrue(panel.error_box.IsShown())

if __name__ == '__main__':
    unittest.main() 