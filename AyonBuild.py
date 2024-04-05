import shutil
from time import process_time
from AyonCiCdTools.AyonCiCd import Cmake, Project, docGen
import os
import subprocess

# define a Project that you want to CiCd
AyonCppApiPrj = Project.Project("AyonCppApi")
AyonCppApiPrj.setVar("BuildTest", "ON")
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
    lambda: Cmake.Command("-S", ".", "-B", "build", f"-DBUILD_TEST={AyonCppApiPrj.Variables['BuildTest']}", f"-DJTRACE={AyonCppApiPrj.Variables['JTRACE']}", f"-DCMAKE_BUILD_TYPE={AyonCppApiPrj.Variables['ReleaseType']}"),
    lambda: Cmake.Command("--build", "build", "--config", f"{AyonCppApiPrj.Variables['ReleaseType']}"),
    lambda: Cmake.Command("--install", "build"),
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


def startMockingServer():
    from AyonCiCdTools.AyonCiCd import VenvFuncs
    venvCommand = VenvFuncs.getVenvActivateCommand(AyonCppApiPrj.venvPath)
    ServerProcessPID = subprocess.Popen([f"{venvCommand} && uvicorn test.Mockingbird:app --host 0.0.0.0 --port 8000"],
                                      stdout=subprocess.PIPE,
                                      shell=True,
                                      preexec_fn=os.setsid)
    AyonCppApiPrj.setVar("ServerProcessPID", f"{ServerProcessPID.pid}")

def stopMockingServer():
    import signal
    os.killpg(os.getpgid(int(AyonCppApiPrj.getVar("ServerProcessPID"))), signal.SIGTERM)

def getCheckMockingServer():
    import requests
    response = requests.get("http://localhost:8000/")
    print("Response from /:", response.json())

def startTestApp():
    from AyonCiCdTools.AyonCiCd import SysCon
    SysCon.open_software("bin/AyonCppApiBenchMain")

TestStage = Project.Stage("Test")
TestStage.addFuncs( 
    # lambda: startMockingServer(),
    # lambda: getCheckMockingServer(),
    # lambda: stopMockingServer(),
    lambda: startTestApp(),
)
AyonCppApiPrj.addStage(TestStage)

# make the CiCd class available to the Cli so we can interact with it
with AyonCppApiPrj as PRJ:
    PRJ.makeClassCliAvailable()
