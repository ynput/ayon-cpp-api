from pprint import pprint
import subprocess
import os


def DoxygenRun(doxyFile):
    DoxyFileName = os.path.basename(doxyFile)
    filePath = os.path.dirname(doxyFile)
    print(DoxyFileName, filePath)
    result = subprocess.run(["doxygen", f"{DoxyFileName}"],cwd=filePath, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    output = result.stdout
    errors = result.stderr
    print("doxygen output:")
    pprint(output)
    print("doxygen errors:")
    pprint(errors)


