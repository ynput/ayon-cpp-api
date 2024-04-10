import shutil
from time import process_time
from typing import Set
from AyonCiCdTools.AyonCiCd import Cmake, Project, docGen
import os
import subprocess
from multiprocessing import Process
import requests

# define a Project that you want to CiCd
AyonCppApiPrj = Project.Project("AyonCppApi")
AyonCppApiPrj.setVar("BuildTest", "OFF")
AyonCppApiPrj.setVar("JTRACE", "0")
AyonCppApiPrj.setVar("ReleaseType", "Release")
# add packages to the project
AyonCppApiPrj.addPipPackage("pytest")
AyonCppApiPrj.addPipPackage("fastapi")
AyonCppApiPrj.addPipPackage("uvicorn[standard]")
AyonCppApiPrj.addPipPackage("requests")
AyonCppApiPrj.addPipPackage("universal-startfile")


# define stages for this project
CleanUpStage = Project.Stage("Cleanup")
# define what happens in this stage
binFoulder = os.path.join(os.getcwd(), "bin")
buildFoulder = os.path.join(os.getcwd(), "build")
print("bin foulder:" , binFoulder)
CleanUpStage.addFuncs(
    lambda: print("build foulder path:", buildFoulder),
    lambda: shutil.rmtree(buildFoulder, ignore_errors=True) if os.path.exists(buildFoulder) else print("no build foulder"),

    lambda: print("build foulder path:", binFoulder),
    lambda: shutil.rmtree(binFoulder, ignore_errors=True) if os.path.exists(binFoulder) else print("no bin foulder"),
)
# add the stage to the project
AyonCppApiPrj.addStage(CleanUpStage)


BuildStage = Project.Stage("Build")
BuildStage.addFuncs(
    lambda: Cmake.Command(AyonCppApiPrj, "-S", ".", "-B", "build", f"-DBUILD_TEST={AyonCppApiPrj.Variables['BuildTest']}", f"-DJTRACE={AyonCppApiPrj.Variables['JTRACE']}", f"-DCMAKE_BUILD_TYPE={AyonCppApiPrj.Variables['ReleaseType']}"),
    lambda: Cmake.Command(AyonCppApiPrj, "--build", "build", "--config", f"{AyonCppApiPrj.Variables['ReleaseType']}"),
    lambda: Cmake.Command(AyonCppApiPrj, "--install", "build"),
)
# BuildStage.addArtefactFoulder("bin")
AyonCppApiPrj.addStage(BuildStage)


DoxyGenStage = Project.Stage("DocumentationGen")
DoxyGenStage.addFuncs(
    lambda: shutil.rmtree("Docs/html") if os.path.exists("Docs/html") else print("Docs dir clean"),
    lambda: docGen.DoxygenRun(os.path.join(os.getcwd(), "Docs/src/Doxyfile"), AyonCppApiPrj),
)
DoxyGenStage.addArtefactFoulder("Docs/html")
AyonCppApiPrj.addStage(DoxyGenStage)


# TODO google benchmark writes system info into sed::error there needs to be an function for that 
def startTestApp():
    from AyonCiCdTools.AyonCiCd import GTest
# "--gtest_filter=AyonCppApi.AyonCppApiCreaion"
    os.environ["AYON_API_KEY"] = "SuperSaveTestKey"
    os.environ["AYON_SERVER_URL"] = "http://localhost:8003"
    os.environ["AYON_SITE_ID"] = "TestId"
    GTest.GTestRun("bin/AyonCppApiBenchMain", f"{AyonCppApiPrj.buildArtefactsPath}/GTest/TestA.xml", AyonCppApiPrj , )

def startTestServer():
    from test import TestServer
    import time
    global ServerPocVar
    ServerPocVar = TestServer.start()
    time.sleep(2)

def CheckTestServer():

    response = requests.get("http://localhost:8003/")
    print("Test Respone", response.text)

def stopTestServer():
    ServerPocVar.terminate()

TestStage = Project.Stage("Test")
TestStage.addFuncs( 
    lambda: startTestServer(),
    lambda: CheckTestServer(),
    #
    lambda: startTestApp(),
    lambda: stopTestServer(),

)
TestStage.addArtefactFoulder("bin/prof.json")
AyonCppApiPrj.addStage(TestStage)


SetTestVars = Project.Stage("SetTestVars")  
SetTestVars.addFuncs(
    lambda: AyonCppApiPrj.setVar("BuildTest", "ON"),
    lambda: AyonCppApiPrj.setVar("JTRACE", "1"),
    lambda: AyonCppApiPrj.setVar("ReleaseType", "Release"),
)


AyonCppApiPrj.CreateStageGRP("BuildAndTest", SetTestVars, BuildStage, TestStage)
AyonCppApiPrj.CreateStageGRP("CleanBuildAndTest", CleanUpStage, SetTestVars, BuildStage, TestStage)
# make the CiCd class available to the Cli so we can interact with it
with AyonCppApiPrj as PRJ:
    PRJ.makeClassCliAvailable()
