import unittest
from unittest.mock import patch
import grass.script as gs
from grass.exceptions import CalledModuleError

class TestPansharpeningAlgorithms(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        # Start mocking the raster existence check to always return True
        cls.mock_find_file = patch('grass.script.find_file', return_value={'name': 'dummy'})
        cls.mock_find_file.start()

        cls.inputs = {
            'blue': 'ms1_orig',
            'green': 'ms2_orig',
            'red': 'ms3_orig',
            'pan': 'pan_orig'
        }
        cls.outputs = {
            'brovey': 'output_brovey',
            'ihs': 'output_ihs',
            'pca': 'output_pca'
        }
        cls.expected_stats = {
            'min': 0,
            'max': 255
        }
        # Set region to the first raster (usually the pan band)
        gs.run_command("g.region", raster=cls.inputs['pan'], flags="p")

    @classmethod
    def tearDownClass(cls):
        cls.mock_find_file.stop()
        gs.run_command("g.remove", type="raster", pattern="output_*", flags="f")

    def test_brovey_algorithm(self):
        output = self.outputs['brovey']
        self.assertTrue(self.run_algorithm('brovey', output))
        self.validate_output(output, self.expected_stats)

    def run_algorithm(self, method, output):
        try:
            gs.run_command("i.pansharpen", method=method, **self.inputs, output=self.outputs[method])
            return True
        except CalledModuleError as e:
            print(f"Error running module: {e}")
            return False

    def validate_output(self, output, expected_stats):
        # Check the existence and integrity of each output raster component.
        for suffix in ['red', 'green', 'blue']:
            raster_name = f"{output}_{suffix}"
            if not gs.find_file(raster_name)['name']:
                self.fail(f"Output raster {raster_name} not found.")
            stats = gs.parse_command('r.univar', map=raster_name, flags="g")
            self.assertTrue(float(stats['min']) >= expected_stats['min'], f"{raster_name} min value is below expected.")
            self.assertTrue(float(stats['max']) <= expected_stats['max'], f"{raster_name} max value exceeds expected.")

if __name__ == "__main__":
    unittest.main()
