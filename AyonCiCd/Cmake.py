from pprint import pprint
import subprocess


def Command(*args):
    result = subprocess.run(["cmake", *args], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    output = result.stdout
    errors = result.stderr
    print("CMake output:")
    pprint(output)
    print("CMake errors:")
    pprint(errors)


