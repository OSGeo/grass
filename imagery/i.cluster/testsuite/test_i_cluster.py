import os
import re
import math
from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from pathlib import Path
from grass.script import find_file
from itertools import combinations


class TestIClusterWithSyntheticData(TestCase):
    group_name = "test_cluster_group"
    subgroup_name = "test_cluster_subgroup"
    input_maps = ["synth_map1", "synth_map2", "synth_map3"]
    signature_file = "sig"
    num_classes = 3

    @classmethod
    def setUpClass(cls):
        """Set up test environment"""
        cls.use_temp_region()
        cls.runModule("g.region", n=50, s=0, e=50, w=0, rows=100, cols=100)
        cls.runModule(
            "r.mapcalc",
            expression=f"{cls.input_maps[0]} = if(row() < 25, 10, 5)",
            overwrite=True,
        )
        cls.runModule(
            "r.mapcalc",
            expression=f"{cls.input_maps[1]} = if(col() < 25, 20, 10)",
            overwrite=True,
        )
        cls.runModule(
            "r.mapcalc",
            expression=f"{cls.input_maps[2]} = row() + col()",
            overwrite=True,
        )
        cls.runModule(
            "i.group",
            group=cls.group_name,
            subgroup=cls.subgroup_name,
            input=",".join(cls.input_maps),
        )

    @classmethod
    def tearDownClass(cls):
        """Cleanup test environment"""
        cls.runModule("g.remove", flags="f", type="group", name=cls.group_name)
        cls.runModule("g.remove", flags="f", type="raster", name=cls.input_maps)

    def setUp(self):
        """Ensure clean signature file state before each test"""
        if self._signature_exists():
            self.runModule(
                "g.remove", flags="f", type="raster", name=self.signature_file
            )

    def _signature_exists(self):
        """Check if the signature file exists in the nested directory"""
        parent_dir_info = find_file("sig", element="signatures")
        sig_file_path = os.path.join(
            parent_dir_info["file"],
            "sig",
            "sig",
        )
        return os.path.isfile(sig_file_path)

    def test_cluster_creation(self):
        """Test the creation of a signature file and validate the number of classes."""
        self.assertModule(
            "i.cluster",
            group=self.group_name,
            subgroup=self.subgroup_name,
            signaturefile=self.signature_file,
            classes=self.num_classes,
            overwrite=True,
        )
        self.assertTrue(
            self._signature_exists(),
            f"Signature file '{self.signature_file}' was not created",
        )
        parent_dir_info = find_file("sig", element="signatures")
        sig_path = os.path.join(
            parent_dir_info["file"],
            "sig",
            "sig",
        )
        content = Path(sig_path).read_text()
        class_count = content.count("#Class ")
        self.assertEqual(
            class_count,
            self.num_classes,
            f"Expected {self.num_classes} classes, but found {class_count}",
        )

    def test_sample_interval(self):
        """Test that sample interval affects the number of cells sampled"""
        sample_interval = (2, 2)
        report_file = "sample_report.txt"
        self.addCleanup(os.remove, report_file) if os.path.exists(report_file) else None

        self.assertModule(
            "i.cluster",
            group=self.group_name,
            subgroup=self.subgroup_name,
            signaturefile=self.signature_file,
            classes=self.num_classes,
            reportfile=report_file,
            sample=sample_interval,
            overwrite=True,
        )
        self.assertTrue(os.path.isfile(report_file))
        content = Path(report_file).read_text()
        expected_cells = (100 // sample_interval[0]) * (100 // sample_interval[1])
        match = re.search(r"Sample size:\s+(\d+)", content)
        self.assertIsNotNone(match, "Total cells line not found in report")
        actual_cells = int(match.group(1))
        self.assertEqual(actual_cells, expected_cells)

    def test_convergence_parameters(self):
        """Test that higher convergence target results in more iterations and higher convergence"""
        low_percent = 90.0
        high_percent = 99.5
        low_report = "low_percent_report.txt"
        high_report = "high_percent_report.txt"
        self.addCleanup(os.remove, low_report) if os.path.exists(low_report) else None
        self.addCleanup(os.remove, high_report) if os.path.exists(high_report) else None
        self.assertModule(
            "i.cluster",
            group=self.group_name,
            subgroup=self.subgroup_name,
            signaturefile=self.signature_file + "_low",
            classes=self.num_classes,
            reportfile=low_report,
            convergence=low_percent,
            overwrite=True,
        )
        self.assertModule(
            "i.cluster",
            group=self.group_name,
            subgroup=self.subgroup_name,
            signaturefile=self.signature_file + "_high",
            classes=self.num_classes,
            reportfile=high_report,
            convergence=high_percent,
            overwrite=True,
        )
        low_content = Path(low_report).read_text()
        high_content = Path(high_report).read_text()
        low_iterations = len(
            re.findall(r"######## iteration \d+ ###########", low_content)
        )
        high_iterations = len(
            re.findall(r"######## iteration \d+ ###########", high_content)
        )
        self.assertGreater(
            high_iterations,
            low_iterations,
            f"Higher convergence should require more iterations. Found {high_iterations} for high vs {low_iterations} for low.",
        )
        low_final_conv = float(
            re.search(r"(\d+) classes \(convergence=(\d+\.\d+)%\)", low_content).group(
                2
            )
        )
        high_final_conv = float(
            re.search(r"(\d+) classes \(convergence=(\d+\.\d+)%\)", high_content).group(
                2
            )
        )
        self.assertGreater(
            high_final_conv,
            low_final_conv,
            f"Higher target should yield higher convergence. Found {high_final_conv}% for high vs {low_final_conv}% for low.",
        )

    def parse_signature_means(self, sig_path):
        """Parse a signature file and return a list of cluster means."""
        means = []
        with open(sig_path) as f:
            current_class = None
            for line in f:
                line = line.strip()
                if line.startswith("#Class "):
                    current_class = int(line.split()[1])
                    means.append([])
                elif line == "#   Mean":
                    # The next line contains the mean values
                    mean_line = next(f).strip()
                    mean_values = list(map(float, mean_line.split()))
                    if current_class is not None:
                        # Ensure we're appending to the correct class
                        means[current_class - 1] = mean_values
                        current_class = None  # Reset to avoid reading other means
        return means

    def minimum_pairwise_distance(self, means):
        """Calculate the minimum pairwise Euclidean distance between cluster means."""
        min_dist = float("inf")
        for a, b in combinations(means, 2):
            dist = math.sqrt(sum((x - y) ** 2 for x, y in zip(a, b)))
            min_dist = min(min_dist, dist)
        return min_dist

    def test_separation_parameter1(self):
        """Test the effect of the separation parameter on cluster separation quality."""
        report_file = "separation_report.txt"
        self.addCleanup(os.remove, report_file) if os.path.exists(report_file) else None
        sig_file_default = self.signature_file + "_sep0"
        self.assertModule(
            "i.cluster",
            group=self.group_name,
            subgroup=self.subgroup_name,
            signaturefile=sig_file_default,
            separation=0.0,
            classes=self.num_classes,
            reportfile=report_file,
            overwrite=True,
        )
        self.addCleanup(
            self.runModule, "g.remove", flags="f", type="raster", name=sig_file_default
        )
        sig_file_high = self.signature_file + "_sep2"
        self.assertModule(
            "i.cluster",
            group=self.group_name,
            subgroup=self.subgroup_name,
            signaturefile=sig_file_high,
            separation=2.0,
            classes=self.num_classes,
            reportfile=report_file,
            overwrite=True,
        )
        self.addCleanup(
            self.runModule, "g.remove", flags="f", type="raster", name=sig_file_high
        )
        parent_dir_info = find_file("sig", element="signatures")
        default_sig_path = os.path.join(
            parent_dir_info["file"], sig_file_default, "sig"
        )
        high_sig_path = os.path.join(parent_dir_info["file"], sig_file_high, "sig")
        default_means = self.parse_signature_means(default_sig_path)
        high_means = self.parse_signature_means(high_sig_path)
        default_min_dist = self.minimum_pairwise_distance(default_means)
        high_min_dist = self.minimum_pairwise_distance(high_means)
        self.assertGreater(
            high_min_dist,
            default_min_dist,
            f"Minimum cluster distance should be higher with separation=2.0 (got {high_min_dist}) than with separation=0.0 (got {default_min_dist})",
        )

    def parse_signature_npoints(self, sig_path):
        """Parse a signature file and return a list of pixel counts per class."""
        npoints = []
        with open(sig_path) as f:
            for line in f:
                line = line.strip()
                if line.startswith("#Class "):
                    npoints_line = next(f).strip()
                    npoints.append(int(npoints_line))
        return npoints

    def test_min_size_parameter(self):
        """Test that classes have at least min_size pixels."""
        min_size = 2000
        sig_file = self.signature_file + "_min_size"
        self.assertModule(
            "i.cluster",
            group=self.group_name,
            subgroup=self.subgroup_name,
            signaturefile=sig_file,
            classes=self.num_classes,
            min_size=min_size,
            overwrite=True,
        )
        self.addCleanup(
            self.runModule, "g.remove", flags="f", type="raster", name=sig_file
        )
        parent_dir_info = find_file("sig", element="signatures")
        sig_path = os.path.join(parent_dir_info["file"], sig_file, "sig")
        npoints = self.parse_signature_npoints(sig_path)

        for count in npoints:
            self.assertGreaterEqual(
                count,
                min_size,
                f"Class has {count} pixels, less than min_size={min_size}",
            )


if __name__ == "__main__":
    test()
