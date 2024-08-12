import shutil
import os
import sys
from AyonCiCdTools.AyonCiCd import Cmake, Project, docGen, GBench, GTest


# define a Project that you want to CiCd
AyonCppApiPrj = Project.Project("AyonCppApi")

# add packages to the project
AyonCppApiPrj.add_pip_package("pytest")
AyonCppApiPrj.add_pip_package("fastapi==0.109.1")
AyonCppApiPrj.add_pip_package("uvicorn[standard]==0.25.0")
AyonCppApiPrj.add_pip_package("requests")


# define stages for this project
CleanUpStage = Project.Stage("Cleanup")
# define what happens in this stage
binFoulder = os.path.join(os.getcwd(), "bin")
buildFoulder = os.path.join(os.getcwd(), "build")
CleanUpStage.add_funcs(
    lambda: print("build foulder path:", buildFoulder),
    lambda: (
        shutil.rmtree(buildFoulder, ignore_errors=True)
        if os.path.exists(buildFoulder)
        else print("no build foulder")
    ),
    lambda: print("build foulder path:", binFoulder),
    lambda: (
        shutil.rmtree(binFoulder, ignore_errors=True)
        if os.path.exists(binFoulder)
        else print("no bin foulder")
    ),
)
# add the stage to the project
AyonCppApiPrj.add_stage(CleanUpStage)


BuildStage = Project.Stage("Build")
BuildStage.add_funcs(
    lambda: Cmake.cmake_command(
        AyonCppApiPrj,
        "-S",
        ".",
        "-B",
        "build",
        f"-DBUILD_TEST={AyonCppApiPrj._project_internal_varialbes['BuildTest']}",
        f"-DJTRACE={AyonCppApiPrj._project_internal_varialbes['JTRACE']}",
        f"-DCMAKE_BUILD_TYPE={AyonCppApiPrj._project_internal_varialbes['ReleaseType']}",
    ),
    lambda: Cmake.cmake_command(
        AyonCppApiPrj,
        "--build",
        "build",
        "--config",
        f"{AyonCppApiPrj._project_internal_varialbes['ReleaseType']}",
    ),
    lambda: Cmake.cmake_command(AyonCppApiPrj, "--install", "build"),
)
# BuildStage.addArtefactFoulder("bin")
AyonCppApiPrj.add_stage(BuildStage)


DoxyGenStage = Project.Stage("DocumentationGen")
DoxyGenStage.add_funcs(
    lambda: (
        shutil.rmtree("Docs/html")
        if os.path.exists("Docs/html")
        else print("Docs dir clean")
    ),
    lambda: docGen.doxygen_run(
        os.path.join(os.getcwd(), "Docs/src/Doxyfile"), AyonCppApiPrj
    ),
)
DoxyGenStage.addArtefactFoulder("Docs/html")
AyonCppApiPrj.add_stage(DoxyGenStage)


def startTestApp():
    os.environ["AYON_API_KEY"] = "SuperSaveTestKey"
    os.environ["AYON_SERVER_URL"] = "http://localhost:8003"
    os.environ["AYON_SITE_ID"] = "TestId"
    os.environ["AYON_PROJECT_NAME"] = "TestPrjName"
    GTest.run_google_test(
        "bin/AyonCppApiGtestMain",
        f"{AyonCppApiPrj._build_artefacts_out_path}/GTest/Test.xml",
        AyonCppApiPrj,
    )


def setupBenchEnvVars():
    os.environ["AYON_API_KEY"] = "SuperSaveTestKey"
    os.environ["AYON_SERVER_URL"] = "http://localhost:8003"
    os.environ["AYON_SITE_ID"] = "TestId"
    os.environ["AYON_PROJECT_NAME"] = "TestPrjName"
    os.environ["AYONLOGGERLOGLVL"] = "CRITICAL"
    os.environ["AYONLOGGERFILELOGGING"] = "OFF"


def runSerialBench():
    setupBenchEnvVars()
    GBench.run_google_benchmark(
        "bin/AyonCppApiGBenchMain",
        f"{AyonCppApiPrj._build_artefacts_out_path}/GBench/SerialResolve.json",
        "--benchmark_filter=AyonCppApiSerialResolve",
    )


def runBatchBench():
    setupBenchEnvVars()
    GBench.run_google_benchmark(
        "bin/AyonCppApiGBenchMain",
        f"{AyonCppApiPrj._build_artefacts_out_path}/GBench/BatchResolve.json",
        "--benchmark_filter=AyonCppApiBatchResolve",
    )


def runAllBench():
    setupBenchEnvVars()
    GBench.run_google_benchmark(
        "bin/AyonCppApiGBenchMain",
        f"{AyonCppApiPrj._build_artefacts_out_path}/GBench/AllResolve.json",
    )


def startTestServer():
    from test import TestServer

    global ServerPocVar
    ServerPocVar = TestServer.start()
    print(ServerPocVar)


def CheckTestServer():
    import requests

    response = requests.get("http://localhost:8003/")
    print("Test Respone", response.text)


def stopTestServer():
    ServerPocVar.terminate()


TestStage = Project.Stage("Test")
TestStage.add_funcs(
    lambda: startTestServer(),
    lambda: CheckTestServer(),
    lambda: startTestApp(),
    lambda: runSerialBench(),
    lambda: runBatchBench(),
    lambda: runAllBench(),
    lambda: stopTestServer(),
)
TestStage.addArtefactFoulder("bin/profBatch.json")
TestStage.addArtefactFoulder("bin/profSerial.json")
AyonCppApiPrj.add_stage(TestStage)

SetBaseVars = Project.Stage("SetBaseVars")
SetBaseVars.add_funcs(
    lambda: AyonCppApiPrj.setVar("BuildTest", "OFF"),
    lambda: AyonCppApiPrj.setVar("JTRACE", "0"),
    lambda: AyonCppApiPrj.setVar("ReleaseType", "Release"),
)

SetTestVars = Project.Stage("SetTestVars")
SetTestVars.add_funcs(
    lambda: AyonCppApiPrj.setVar("BuildTest", "ON"),
    lambda: AyonCppApiPrj.setVar("JTRACE", "1"),
    lambda: AyonCppApiPrj.setVar("ReleaseType", "Release"),
)


AyonCppApiPrj.creat_stage_group(
    "CleanBuild", SetBaseVars, CleanUpStage, BuildStage
)  # SetBaseVars needs to replace SetTestVars
AyonCppApiPrj.creat_stage_group(
    "CleanBuildAndDocs", SetBaseVars, CleanUpStage, BuildStage, DoxyGenStage
)
AyonCppApiPrj.creat_stage_group("BuildAndTest", SetTestVars, BuildStage, TestStage)
AyonCppApiPrj.creat_stage_group(
    "CleanBuildAndTest", CleanUpStage, SetTestVars, BuildStage, TestStage
)

# make the CiCd class available to the Cli so we can interact with it
with AyonCppApiPrj as PRJ:
    PRJ.make_project_cli_available()
