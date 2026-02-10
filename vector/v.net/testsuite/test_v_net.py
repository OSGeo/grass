from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.script.core import read_command
from grass.tools import Tools
import json

class TestVNet(TestCase):
    network = "test_vnet"

    def tearDown(self):
        """Remove viewshed map after each test method"""
        self.runModule("g.remove", flags="f", type="vector", name=self.network)

    def test_nreport_json(self):
        self.runModule("v.net", input="streets", points="schools", 
                       output=self.network, operation="connect", 
                       threshold=400, flags="c")
        
        tools = Tools(quiet=True)
        result = tools.v_net(
            input=self.network,
            operation="nreport",
            node_layer=2,
            format="json"
        )

        try:
            data = json.loads(result.text)
        except json.JSONDecodeError:
            self.fail(f"nreport produced invalid JSON: {result.text}")

        self.assertIsInstance(data, list)
        self.assertGreater(len(data), 0, "nreport output is empty")
        
        first_row = data[0]
        self.assertIn("node_cat", first_row)
        self.assertIn("lines", first_row)
        
        self.assertIsInstance(first_row["node_cat"], int)
        self.assertIsInstance(first_row["lines"], list)
        self.assertGreater(len(first_row["lines"]), 0)

    def test_report_json(self):
        tools = Tools(quiet=True)
        result = tools.v_net(
            input="streets",
            operation="report",
            arc_layer=1,
            node_layer=2,
            format="json"
        )
        
        try:
            data = json.loads(result.text)
        except json.JSONDecodeError:
            self.fail(f"report produced invalid JSON: {result.text}")

        self.assertIsInstance(data, list)
        self.assertGreater(len(data), 0, "report output is empty")
        
        self.assertIn("line_cat", data[0])
        self.assertIsInstance(data[0]["line_cat"], int)

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
