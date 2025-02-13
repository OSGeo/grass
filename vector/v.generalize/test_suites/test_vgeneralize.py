from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.script import core as grass
from grass.script import vector as gvector

class TestVGeneralize(TestCase):
	@classmethod
	def setUpClass(cls):
		"""Set up the test environment by importing coordinates and creating a vector map"""
		#Setting up the temporary computational region
		cls.runModule("g.region", n=3401500, s=3400800, e=5959100, w=5958800, res=10)
		
		#Import coordinates from test_coordinates.txt which is in ascii format using v.in.ascii"""
		cls.runModule("v.in.ascii", input="test_coordinates.txt", output="test_lines", format="standard", overwrite=True,)
		
		
	@classmethod
	def tearDownClass(cls):
		"""Cleaning up the test environment."""
        #Cleaning up the temporary computational region
		cls.runModule("g.remove", type="vector", name="test_lines,generalized_lines", flags="f",)
		
	def get_vertices(self,file_name):
		"""Getting vertices from the ascii based formatted file"""
        #This function counts the number of vertices in the ASCII Standard Format File of the Grass GIS. This will help us in verifying whether simplification or smoothing algorithm works correctly or not
		with open(file_name,"r") as file:
			lines=file.readlines()
		
		vertices=0
		in_verti_section=False
		
		for line in lines:
			if line.strip() == "VERTI:":
				in_verti_section=True
				continue
			
			if in_verti_section:
				if line.strip().startswith("B"):
					parts=line.strip().split()
					if len(parts)>=2:
						vertices=vertices+int(parts[1])
							
		return vertices
		
	def test_douglas_peuckar(self):
		"""Test Douglas-Peucker Simplification."""
        self.assertModule("v.generalize", input="test_lines", output="generalized_lines", method="douglas", threshold=10.0, overwrite=True,)
		self.assertVectorExists("generalized_lines")
		
	def test_snakes(self):
		"""Test Snakes Smoothing"""
		self.assertModule("v.generalize", input="test_lines", output="generalized_lines", method="snakes", alpha=1.0, beta=1.0, threshold=1, overwrite=True,)
		self.assertVectorExists("generalized_lines")
		
	def test_chaiken(self):
		"""Test Chaiken Smoothing"""
		self.assertModule("v.generalize", input="test_lines", output="generalized_lines", method="chaiken", threshold=1.0, overwrite=True,)
		self.assertVectorExists("generalized_lines")
		
	def test_hermite(self):
		"""Test Hermite Method"""
		self.assertModule("v.generalize", input="test_lines", output="generalized_lines", method="hermite", threshold=1.0, overwrite=True,)
		self.assertVectorExists("generalized_lines")
		
	def test_distance_weighting(self):
		"""Distance Weighting Method"""
		self.assertModule("v.generalize", input="test_lines", output="generalized_lines", method="distance_weighting", threshold=1.0, overwrite=True)
		self.assertVectorExists("generalized_lines")
		
	def test_invalid_method(self):
		"""Test invalid generalization method."""
		with self.assertRaises(ValueError):
			self.assertModule("v.generalize", input="test_lines", output="generalized_lines", method="invalid", overwrite=True)
			
	def test_simplification(self):
		"""Number of vertices decreases after simplification using Douglas-Peucker Simplification."""
		self.assertModule("v.generalize", input="test_lines", output="generalized_lines", method="douglas", threshold=10.0, overwrite=True)
		self.assertVectorExists("generalized_lines")

        #Exporting the output vector(generalized_lines) to ascii file for further analysis
		output_file=grass.read_command("v.out.ascii", input="generalized_lines", format="standard", layer=-1)
		
		
		input_vertices= self.get_vertices("test_coordinates.txt")
		output_vertices=0
		in_verti_section=False

        #Extracting output vertices        
		for line in output_file.splitlines():
			if line.strip() == "VERTI:":
				in_verti_section=True
				continue
			
			if in_verti_section:
				if line.strip().startswith("B"):
					parts=line.strip().split()
					if len(parts)>=2:
						output_vertices=output_vertices+int(parts[1])
		
		
		self.assertGreater(int(input_vertices),int(output_vertices))
		
	def test_smoothing(self):
		"""Number of vertices increases after smoothing using Chaiken Smoothing"""
		self.assertModule("v.generalize", input="test_lines", output="generalized_lines", method="chaiken", threshold=1.0, overwrite=True, verbose=True)
		self.assertVectorExists("generalized_lines")

        #Exporting the output vector(generalized_lines) to ascii file for further analysis
		output_file=grass.read_command("v.out.ascii", input="generalized_lines", format="standard", layer=-1)
		
		
		input_vertices= self.get_vertices("test_coordinates.txt")
		output_vertices=0
		in_verti_section=False

        #Extracting Output Vertices
		for line in output_file.splitlines():
			if line.strip() == "VERTI:":
				in_verti_section=True
				continue
			
			if in_verti_section:
				if line.strip().startswith("B"):
					parts=line.strip().split()
					if len(parts)>=2:
						output_vertices=output_vertices+int(parts[1])
		
		
		self.assertLess(int(input_vertices),int(output_vertices))
		
		
		
if __name__=="__main__":
	test()
		
		
