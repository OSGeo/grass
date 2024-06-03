import unittest
from unittest.mock import patch
import grass.script as gs
from grass.exceptions import CalledModuleError

class TestPansharpeningAlgorithms(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.mock_find_file = patch('grass.script.find_file', return_value={'name': 'dummy'})
        cls.mock_find_file.start()

        cls.mock_run_command = patch('grass.script.run_command', return_value=0)
        cls.mock_run_command.start()

        cls.mock_parse_command = patch('grass.script.parse_command', return_value={'min': '0', 'max': '255'})
        cls.mock_parse_command.start()

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

    @classmethod
    def tearDownClass(cls):
        cls.mock_find_file.stop()
        cls.mock_run_command.stop()
        cls.mock_parse_command.stop()

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
            stats = gs.parse_command('r.univar', map=raster_name, flags="g")
            self.assertTrue(float(stats['min']) >= expected_stats['min'], f"{raster_name} min value is below expected.")
            self.assertTrue(float(stats['max']) <= expected_stats['max'], f"{raster_name} max value exceeds expected.")

if __name__ == "__main__":
    unittest.main()
