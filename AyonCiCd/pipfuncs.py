import importlib
import subprocess

def check_and_install_package(package_name):
    try:
        importlib.import_module(package_name)
        print(f"{package_name} is already installed.")
    except ImportError:
        print(f"{package_name} is not installed. Installing...")
        subprocess.check_call(["pip", "install", package_name])
        print(f"{package_name} has been successfully installed.")
