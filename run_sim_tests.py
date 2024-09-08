import os
import subprocess
import filecmp
import shutil

import h5py
import numpy as np

# Define the base directory
TEST_DIR = './test/sim_tests/'
TOLERANCE = 0  # Use 0 for exact comparison or small value (e.g., 1e-6) for flexibility


def run_simulation(test_path: str) -> bool:
    """
    Run the FERS simulation for a given test case

    :param test_path: path to the test directory
    :return: True if the simulation runs successfully, False otherwise
    """
    # Change to the test directory
    original_dir = os.getcwd()
    os.chdir(test_path)

    # Run FERS simulation
    input_file = 'input.fersxml'
    result = subprocess.run(['../../../build/src/fers', input_file], capture_output=True, text=True)

    # Return to the original directory
    os.chdir(original_dir)

    if result.returncode != 0:
        print(f"Test {test_path} failed during execution: {result.stderr}")
        return False
    return True


def compare_hdf5_files(expected_file: str, generated_file: str) -> bool:
    """
    Compare two HDF5 files' datasets using a numerical tolerance.

    :param expected_file: The path to the expected HDF5 file.
    :param generated_file: The path to the generated HDF5 file.
    :return: True if the files match within the tolerance, False otherwise.
    """
    # Compare two HDF5 files' datasets using a numerical tolerance
    with h5py.File(expected_file, 'r') as expected, h5py.File(generated_file, 'r') as generated:
        for key in expected.keys():
            if key not in generated:
                print(f"Dataset {key} missing in generated file.")
                return False

            expected_data = expected[key][:]
            generated_data = generated[key][:]

            if not np.allclose(expected_data, generated_data, atol=TOLERANCE):
                print(f"Data mismatch in dataset {key}")
                return False
    return True


def compare_output(test_path: str) -> bool:
    """
    Compare the expected output with the generated output for a given test case

    :param test_path: The path to the test directory
    :return: True if the output matches the expected output, False otherwise
    """
    # Compare expected output with generated output
    expected_dir = os.path.join(test_path, 'expected_output')

    for root, dirs, files in os.walk(expected_dir):
        for file in files:
            expected_file = str(os.path.join(root, file))
            generated_file = os.path.join(test_path, file)

            if file.endswith('.h5'):
                if not compare_hdf5_files(expected_file, generated_file):
                    print(f"Test {test_path} failed: {file} output mismatch")
                    return False
            else:
                if not filecmp.cmp(expected_file, generated_file, shallow=False):
                    print(f"Test {test_path} failed: {file} output mismatch")
                    return False
    return True


def clean_up(test_path: str) -> None:
    """
    Clean up any generated files that are not needed in the test directory.

    :param test_path: The path to the test directory
    :return: None
    """
    # Clean up any generated files that are not needed
    for item in os.listdir(test_path):
        if item not in ['input.fersxml', 'waveform.csv', 'expected_output']:
            path = os.path.join(test_path, item)
            if os.path.isdir(path):
                shutil.rmtree(path)
            else:
                os.remove(path)


def run_tests() -> None:
    """
    Run all simulation tests in the test directory.

    :return: None
    """
    passed_tests = 0

    for test_name in os.listdir(TEST_DIR):
        test_path = os.path.join(TEST_DIR, test_name)
        if os.path.isdir(test_path):
            if run_simulation(test_path) and compare_output(test_path):
                print(f"Test {test_name} passed.")
                passed_tests += 1
            else:
                print(f"Test {test_name} failed.")
            clean_up(test_path)

    print(f"Passed {passed_tests} out of {len(os.listdir(TEST_DIR))} tests.")

    if passed_tests == len(os.listdir(TEST_DIR)):
        exit(0)
    else:
        exit(1)


if __name__ == '__main__':
    run_tests()