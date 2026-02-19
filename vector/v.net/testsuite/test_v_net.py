from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.script.core import read_command
from grass.gunittest.gmodules import SimpleModule
import json


class TestVNet(TestCase):
    network = "test_vnet"

    def tearDown(self):
        """Remove viewshed map after each test method"""
        self.runModule("g.remove", flags="f", type="vector", name=self.network)

    def test_nreport_json_output(self):
        """Verify that v.net nreport produces valid and accurate JSON output"""
        self.runModule(
            "v.net",
            input="streets",
            points="schools",
            output=self.network,
            operation="connect",
            threshold=400,
            flags="c",
        )

        vnet_module = SimpleModule(
            "v.net",
            input=self.network,
            operation="nreport",
            node_layer=2,
            format="json",
        )
        self.assertModule(vnet_module)

        actual_output = json.loads(vnet_module.outputs.stdout)

        self.assertIsInstance(actual_output, list)
        self.assertGreater(len(actual_output), 0, "The JSON output list is empty")

        expected_keys = {"node_cat", "lines"}
        self.assertEqual(set(actual_output[0].keys()), expected_keys)
        self.assertIsInstance(actual_output[0]["node_cat"], int)
        self.assertIsInstance(actual_output[0]["lines"], list)

    def test_report_json_output(self):
        """Verify that v.net report produces valid and accurate JSON output"""
        vnet_module = SimpleModule(
            "v.net",
            input="streets_net",
            operation="report",
            arc_layer=1,
            node_layer=2,
            format="json",
        )
        self.assertModule(vnet_module)

        actual_output = json.loads(vnet_module.outputs.stdout)

        self.assertIsInstance(actual_output, list)
        self.assertGreater(len(actual_output), 0, "The JSON output list is empty")

        expected_keys = {"line_cat", "start_node_cat", "end_node_cat"}
        self.assertEqual(set(actual_output[0].keys()), expected_keys)

        for entry in actual_output[:10]:
            for key in expected_keys:
                self.assertIsInstance(entry[key], int)

    def test_nodes(self):
        """Test"""
        self.assertModule(
            "v.net", input="streets", output=self.network, operation="nodes"
        )
        topology = {"points": 41813, "nodes": 41813, "lines": 49746}
        self.assertVectorFitsTopoInfo(vector=self.network, reference=topology)
        layers = read_command("v.category", input=self.network, option="layers").strip()
        self.assertEqual(first="1", second=layers, msg="Layers do not match")

    def test_nodes_layers(self):
        """Test"""
        self.assertModule(
            "v.net", input="streets", output=self.network, operation="nodes", flags="c"
        )
        topology = {"points": 41813, "nodes": 41813, "lines": 49746}
        self.assertVectorFitsTopoInfo(vector=self.network, reference=topology)
        layers = read_command("v.category", input=self.network, option="layers").strip()
        self.assertEqual(first="1\n2", second=layers, msg="Layers do not match")

    def test_connect(self):
        """Test"""
        self.assertModule(
            "v.net",
            input="streets",
            points="schools",
            output=self.network,
            operation="connect",
            threshold=1000,
        )
        topology = {"points": 167, "nodes": 42136, "lines": 50069}
        self.assertVectorFitsTopoInfo(vector=self.network, reference=topology)
        layers = read_command("v.category", input=self.network, option="layers").strip()
        self.assertEqual(first="1\n2", second=layers, msg="Layers do not match")

    def test_connect_snap(self):
        """Test"""
        self.assertModule(
            "v.net",
            input="streets",
            points="schools",
            flags="s",
            output=self.network,
            operation="connect",
            threshold=1000,
        )
        topology = {"points": 167, "nodes": 41969, "lines": 49902}
        self.assertVectorFitsTopoInfo(vector=self.network, reference=topology)


if __name__ == "__main__":
    test()
