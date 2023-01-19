import os
import filecmp

def test_generate_reflection_headers():
    current_file = os.path.realpath(__file__)
    current_path = os.path.dirname(current_file)
    parent_directory = os.path.dirname(current_path)

    script = os.path.join(parent_directory, 'generate_reflection_headers.py')
    header = os.path.join(current_path, 'reflectible.hpp')
    os.system(f'python {script} {header} --clang-args -std=c++20')

    reflection_header = os.path.join(current_path, 'reflectible.rpp')
    reference_header = os.path.join(current_path, 'expected.rpp')
    assert filecmp.cmp(reflection_header, reference_header)
