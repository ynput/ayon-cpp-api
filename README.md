# ayon-cpp-api
An API Wrapper for AYON server written in cpp




Test runs tests on the api ( the test app needs to be run from inside AYON (Because off environment variables))
- The tests are formatted for Chrome://trcaing ( the output file is defined in Test.cpp file ) 

Start it from Admin Console 

import os
import subprocess

def start_cpp_application(executable_path):
    try:
        result = subprocess.run([executable_path], capture_output=True, text=True, check=True)
        print("Output:", result.stdout)
    except subprocess.CalledProcessError as e:
        print(f"Error: {e}")
        print("Stdout:", e.stdout)
        print("Stderr:", e.stderr)

executable_path = "/home/workh/Ynput/dev/ayon-cpp-api/bin/test_app"
start_cpp_application(executable_path)

