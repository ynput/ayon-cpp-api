import os
from io import StringIO 
import sys
from functools import partial
from pprint import pprint
import shutil

class Capturing(list):
    def __enter__(self):
        self._stdout = sys.stdout
        sys.stdout = self._stringio = StringIO()
        return self
    def __exit__(self, *args):
        self.extend(self._stringio.getvalue().splitlines())
        del self._stringio    # free up some memory
        sys.stdout = self._stdout


class Project():
    def __init__(self, outputFoulderName) -> None:
        self.Stages = []
        self.baseOutputFoulderPath = os.path.abspath(outputFoulderName)
        if os.path.exists(self.baseOutputFoulderPath):
            shutil.rmtree(self.baseOutputFoulderPath)
        os.mkdir(self.baseOutputFoulderPath)

    def addStage(self ,stageInstance) -> None:
        self.Stages.append(stageInstance)

    def execStage(self, Stage) -> None:
        fileName = f"{Stage.StageName}.txt"
        filePos = os.path.join(self.baseOutputFoulderPath, fileName)
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
        for stage in self.Stages:
            self.execStage(stage)


class Stage: 
    def __init__(self, StageName:str) -> None:
        self.funcs = []
        self.StageName = StageName

    def addFunc(self, funcInstance) -> None:
        self.funcs.append(funcInstance)

    def addFuncs(self, *args) -> None:
        for lambda_func in args:
            self.funcs.append(lambda_func)

    def execStage(self):
        with Capturing() as output:
            for func in self.funcs:
                func()
        return output
