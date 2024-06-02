import unittest
import grass.script as gs
from grass.exceptions import CalledModuleError, OpenError


class TestPansharpeningAlgorithms(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        try:
            # Ensure the pan raster exists and set the region accordingly
            pan_raster = 'input_pan@PERMANENT'
            if not gs.find_file(pan_raster)['name']:
                raise FileNotFoundError(f"Raster map <{pan_raster}> not found.")
            gs.run_command('g.region', raster=pan_raster, flags='p')

            cls.inputs = {'red': 'input_red', 'green': 'input_green', 'blue': 'input_blue', 'pan': pan_raster}
            cls.outputs = {'brovey': 'output_brovey', 'ihs': 'output_ihs', 'pca': 'output_pca'}

        except CalledModuleError as e:
            raise unittest.SkipTest(f"Required raster data not available or GRASS setup issue: {str(e)}.")


    def test_brovey_algorithm(self):
        # Verifies that the Brovey algorithm processes correctly and meets expected outputs
        output = self.outputs['brovey']
        self.assertTrue(self.run_algorithm('brovey', output))
        self.validate_output(output, self.expected_stats)

    def test_ihs_algorithm(self):
        # Ensures the IHS algorithm functions correctly and validates the output integrity
        output = self.outputs['ihs']
        self.assertTrue(self.run_algorithm('ihs', output))
        self.validate_output(output, self.expected_stats)

    def test_pca_algorithm(self):
        # Tests the PCA algorithm for correct execution and validates the output's accuracy
        output = self.outputs['pca']
        self.assertTrue(self.run_algorithm('pca', output))
        self.validate_output(output, self.expected_stats)

    def test_algorithm_execution(self):
        # Test each algorithm to ensure it executes correctly and validates output integrity. can be replaced with 
        # above three algos
        for method, output in self.outputs.items():
            with self.subTest(method=method):
                self.assertTrue(self.run_algorithm(method, output))
                self.validate_output(output, self.expected_stats)

    def run_algorithm(self, method, output):
        # Run the pansharpening algorithm and simulate the processing.
        try:
            gs.run_command('i.pansharpen', method=method, **self.inputs, output=self.outputs[method])
            return True
        except CalledModuleError:
            return False

    def validate_output(self, output, expected_stats):
        # Check the existence and integrity of each output raster component.
        for suffix in ['red', 'green', 'blue']:
            raster_name = f"{output}_{suffix}"
            if not gs.find_file(raster_name)['name']:
                self.fail(f"Output raster {raster_name} not found.")
            stats = gs.parse_command('r.univar', map=raster_name, flags='g')
            self.assertTrue(float(stats['min']) >= expected_stats['min'], f"{raster_name} min value is below expected.")
            self.assertTrue(float(stats['max']) <= expected_stats['max'], f"{raster_name} max value exceeds expected.")

    @classmethod
    def tearDownClass(cls):
        # Cleans up all generated data to maintain a clean state.
        gs.run_command('g.remove', type='raster', pattern='output_*', flags='f')

if __name__ == '__main__':
    unittest.main()
