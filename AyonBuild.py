import shutil
from AyonCiCdTools.AyonCiCd import Cmake, Project, docGen
import os


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
BuildStage.addArtefactFoulder("bin")
AyonCppApiPrj.addStage(BuildStage)


DoxyGenStage = Project.Stage("DocumentationGen")
DoxyGenStage.addFuncs(
    lambda: shutil.rmtree("Docs/html") if os.path.exists("Docs/html") else print("Docs dir clean"),
    lambda: docGen.DoxygenRun(os.path.join(os.getcwd(), "Docs/src/Doxyfile"), AyonCppApiPrj),
)
DoxyGenStage.addArtefactFoulder("Docs/html")
AyonCppApiPrj.addStage(DoxyGenStage)


def startTestApp():
    from AyonCiCdTools.AyonCiCd import GTest
    os.environ["AYON_API_KEY"] = "SuperSaveTestKey"
    os.environ["AYON_SERVER_URL"] = "http://localhost:8003"
    os.environ["AYON_SITE_ID"] = "TestId"
    os.environ["AYON_PROJECT_NAME"] = "TestPrjName"
    GTest.GTestRun("bin/AyonCppApiGtestMain", f"{AyonCppApiPrj.buildArtefactsPath}/GTest/Test.xml", AyonCppApiPrj , )

def startBnechApp():
    from AyonCiCdTools.AyonCiCd import GBench
    os.environ["AYON_API_KEY"] = "SuperSaveTestKey"
    os.environ["AYON_SERVER_URL"] = "http://localhost:8003"
    os.environ["AYON_SITE_ID"] = "TestId"
    os.environ["AYON_PROJECT_NAME"] = "TestPrjName"
    os.environ["AYONLOGGERLOGLVL"] = "CRITICAL"
    os.environ["AYONLOGGERFILELOGGING"] = "OFF"
    GBench.GBnechRun("bin/AyonCppApiGBenchMain",f"{AyonCppApiPrj.buildArtefactsPath}/GBench/Test.json", AyonCppApiPrj)


def startTestServer():
    from test import TestServer
    import time
    global ServerPocVar
    ServerPocVar = TestServer.start()
    time.sleep(2)

def CheckTestServer():
    import requests
    response = requests.get("http://localhost:8003/")
    print("Test Respone", response.text)

def stopTestServer():
    ServerPocVar.terminate()

TestStage = Project.Stage("Test")
TestStage.addFuncs( 
    lambda: startTestServer(),
    lambda: CheckTestServer(),

    lambda: startTestApp(),
    lambda: startBnechApp(),
    lambda: stopTestServer(),

)
TestStage.addArtefactFoulder("bin/profBatch.json")
TestStage.addArtefactFoulder("bin/profSerial.json")
AyonCppApiPrj.addStage(TestStage)


SetTestVars = Project.Stage("SetTestVars")  
SetTestVars.addFuncs(
    lambda: AyonCppApiPrj.setVar("BuildTest", "ON"),
    lambda: AyonCppApiPrj.setVar("JTRACE", "1"),
    lambda: AyonCppApiPrj.setVar("ReleaseType", "Release"),
)


AyonCppApiPrj.CreateStageGRP("CleanBuild", CleanUpStage, BuildStage)
AyonCppApiPrj.CreateStageGRP("CleanBuildAndDocs", CleanUpStage, BuildStage, DoxyGenStage)
AyonCppApiPrj.CreateStageGRP("BuildAndTest", SetTestVars, BuildStage, TestStage)
AyonCppApiPrj.CreateStageGRP("CleanBuildAndTest", CleanUpStage, SetTestVars, BuildStage, TestStage)


# make the CiCd class available to the Cli so we can interact with it
with AyonCppApiPrj as PRJ:
    PRJ.makeClassCliAvailable()
