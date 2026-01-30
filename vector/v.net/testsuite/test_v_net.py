from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.script.core import read_command
import json
import grass.script as gs


class TestVNet(TestCase):
    network = "test_vnet"

    def tearDown(self):
        """Remove viewshed map after each test method"""
        # TODO: eventually, removing maps should be handled through testing framework functions
        self.runModule("g.remove", flags="f", type="vector", name=self.network)

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

    def test_nodes_json(self):
        output = gs.read_command(
            "v.net",
            input="streets",
            points="schools",
            output=self.network,
            operation="nodes",
            format="json",
        )

        self.assertTrue(output, "Module produced no output on stdout")

        try:
            start = output.find("{")
            end = output.rfind("}") + 1
            json_str = output[start:end]
            data = json.loads(json_str)
        except Exception as e:
            self.fail(f"Failed to parse JSON. Error: {e}. Raw output: {output}")

        self.assertIn("new_nodes", data)
        self.assertEqual(
            data["new_nodes"],
            41813,
            "The number of new nodes does not match the expected value",
        )

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
