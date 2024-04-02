import os
from io import StringIO 
import sys
from pprint import pprint
import shutil
import subprocess
import inspect
from . import pipfuncs

class Capturing(list):
    def __enter__(self):
        self._stdout = sys.stdout
        sys.stdout = self._stringio = StringIO()
        return self

    def __exit__(self, *args):
        self.extend(self._stringio.getvalue().splitlines())
        del self._stringio 
        sys.stdout = self._stdout


class Project():
    def __init__(self, projectName) -> None:
        self.Stages = []
        self.ProjectName = projectName
        self.baseOutputFoulderPath = os.path.abspath(projectName+"_CiCd")
        self.buildArtefactsPath = os.path.abspath(os.path.join(self.baseOutputFoulderPath, "buildArtefacts"))
        self.venvPath = os.path.abspath(os.path.join(self.baseOutputFoulderPath, (projectName+"_BuildVenv")))
        self.rootExecuterScript = inspect.stack()[1].filename

        if (self.check_venv(projectName+"_BuildVenv") != True):
            if os.path.exists(self.baseOutputFoulderPath):
                shutil.rmtree(self.baseOutputFoulderPath)
                print("Deleted Root foulder")
            os.mkdir(self.baseOutputFoulderPath)
            os.mkdir(self.buildArtefactsPath)

            self.create_venv(self.venvPath)
            self.activate_venv(self.venvPath, self.rootExecuterScript)
            self.stop_execution()
        print("running")

    def create_venv(self, venv_name):
        print(f"Creating Venv: {venv_name}")
        subprocess.run([sys.executable, "-m", "venv", venv_name], check=True)

    def activate_venv(self, venv_name, python_script_path):
        if sys.platform.startswith('win'):
            activate_script = os.path.join(venv_name, "Scripts", "activate")
        else:
            activate_script = os.path.join(venv_name, "bin", "activate")
        
        activate_cmd = f"source {activate_script} && python {python_script_path}"
        process = subprocess.Popen(activate_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
        stdout, stderr = process.communicate()
        print(stdout.decode())
        print(stderr.decode())
        process.wait()

    def addPipPackage(self, packageName:str) -> None:
        pipfuncs.check_and_install_package(packageName)

    def stop_execution(self):
        print("Stopping further execution of the script.")
        sys.exit()

    def check_venv(self, venv_name) -> bool:
        if hasattr(sys, 'real_prefix') or (hasattr(sys, 'base_prefix') and sys.base_prefix != sys.prefix):
            # Inside a virtual environment
            if os.path.basename(sys.prefix) != venv_name:
                print(f"Error: This script should be run within a virtual environment named '{venv_name}'.")
                return False
        else:
            print("Error: This script should be run within a virtual environment.")
            return False
        return True

    def addStage(self ,stageInstance) -> None:
        self.Stages.append(stageInstance)
        stageInstance.parentOutputFoulder = self.buildArtefactsPath

    def execStage(self, Stage) -> None:
        fileName = f"{Stage.StageName}.txt"
        filePos = os.path.join(self.buildArtefactsPath, fileName)
        print()
        print("-"*80)
        print("Running Stage: ", Stage.StageName)
        print("Output file path: ", filePos)
        print()
        with open(filePos, "w") as file:
            output = Stage.execStage()
            file.write('\n'.join(output))
            
            pprint(output)
        print("-"*80)

    def execAllStages(self) -> None:
        for index, stage in enumerate(self.Stages):
            self.execStage(stage)




class Stage: 
    def __init__(self, StageName:str) -> None:
        self.funcs = []
        self.artifacts = []
        self.StageName = StageName
        self.parentOutputFoulder = None

    def addFunc(self, funcInstance) -> None:
        self.funcs.append(funcInstance)

    def addFuncs(self, *args) -> None:
        for lambda_func in args:
            self.funcs.append(lambda_func)

    def addArtefactFoulder(self, fouderPath) -> None:
        self.artifacts.append(fouderPath)

    def execStage(self):
        with Capturing() as output:
            # exec stage funcs
            for func in self.funcs:
                func()
            # exec stage artefacts 
            for artefactPath in self.artifacts:
                shutil.copytree(os.path.abspath(artefactPath), os.path.join(self.parentOutputFoulder, (self.StageName + "_Artefact"))),
        return output
