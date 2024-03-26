import subprocess
import sys
from pathlib import Path

# class CMakeCommandsContext:
#     def __init__(self, outputFoulder):
#         self.commands = []
#         self.outputFoulder = outputFoulder
#         Path(self.outputFoulder).mkdir(exist_ok=True)
#
#     def __enter__(self):
#         return self
#
#     def __exit__(self, exc_type, exc_value, traceback):
#         self.execute_commands()
#
#     def add_command(self, command):
#         self.commands.append(command)
#
#     def execute_commands(self):
#         for command in self.commands:
#             file = self.outputFoulder + command[0]
#             with open(file, 'w') as f:
#                 result = subprocess.run(command, check=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
#                 output = result.stdout
#                 f.write(output)
#                 sys.stdout.write(output)


def Command(*args):
    result = subprocess.run(["cmake", *args], check=True)
    return result.stdout
