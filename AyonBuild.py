import shutil
import os
import sys
from ayon_automator.AyonCiCd import Cmake, Project, docGen, GBench, GTest


# define a Project that you want to CiCd
AyonCppApiPrj = Project.Project("AyonCppApi")

# add packages to the project
AyonCppApiPrj.add_pip_package("pytest")
AyonCppApiPrj.add_pip_package("fastapi==0.109.1")
AyonCppApiPrj.add_pip_package("uvicorn[standard]==0.25.0")
AyonCppApiPrj.add_pip_package("requests")
AyonCppApiPrj.setup_prj()

# define stages for this project
CleanUpStage = Project.Stage("Cleanup")
# define what happens in this stage
binFoulder = os.path.join(os.getcwd(), "bin")
buildFoulder = os.path.join(os.getcwd(), "build")
CleanUpStage.add_funcs(
    Project.Func(
        f"Remove Build Foulder: {buildFoulder}",
        shutil.rmtree,
        buildFoulder,
        ignore_errors=True,
    ),
    Project.Func(
        f"Remove Bin Foulder: {binFoulder}",
        shutil.rmtree,
        binFoulder,
        ignore_errors=True,
    ),
)
# add the stage to the project
AyonCppApiPrj.add_stage(CleanUpStage)


BuildStage = Project.Stage("Build")
BuildStage.add_funcs(
    Project.Func(
        "Run Cmake Config Commmand",
        Cmake.cmake_command,
        AyonCppApiPrj,
        "-S",
        ".",
        "-B",
        "build",
        f"-DBUILD_TEST={AyonCppApiPrj.getVar('BuildTest')}",
        f"-DJTRACE={AyonCppApiPrj.getVar('JTRACE')}",
        f"-DCMAKE_BUILD_TYPE={AyonCppApiPrj.getVar('ReleaseType')}",
    ),
    Project.Func(
        "Run Cmake Build Command",
        Cmake.cmake_command,
        AyonCppApiPrj,
        "--build",
        "build",
        "--config",
        f"{AyonCppApiPrj.getVar('ReleaseType')}",
    ),
    Project.Func(
        "Run Cmake Install Command",
        Cmake.cmake_command,
        AyonCppApiPrj,
        "--install",
        "build",
    ),
)
# BuildStage.addArtefactFoulder("bin")
AyonCppApiPrj.add_stage(BuildStage)


DoxyGenStage = Project.Stage("DocumentationGen")
DoxyGenStage.add_funcs(
    Project.Func("Remove Htlm Docs", shutil.rmtree, "Docs/html"),
    Project.Func(
        "Generate Html Docs",
        docGen.doxygen_run,
        os.path.join(os.getcwd(), "Docs/src/Doxyfile"),
        AyonCppApiPrj,
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
    Project.Func("Start Test Server", startTestServer),
    Project.Func("Wait For Test Server to be Avaialbe", CheckTestServer),
    Project.Func("Run GTest", startTestApp),
    Project.Func("Run GBench Serial Bench", runSerialBench),
    Project.Func("Run GBench Batch Bench", runBatchBench),
    Project.Func("Run GBench All Benchmark's", runAllBench),
    Project.Func("Stop Test Server", stopTestServer),
)
TestStage.addArtefactFoulder("bin/profBatch.json")
TestStage.addArtefactFoulder("bin/profSerial.json")
AyonCppApiPrj.add_stage(TestStage)

SetBaseVars = Project.Stage("SetBaseVars")
SetBaseVars.add_funcs(
    Project.Func("", AyonCppApiPrj.setVar, "BuildTest", "OFF"),
    Project.Func("", AyonCppApiPrj.setVar, "JTRACE", "0"),
    Project.Func("", AyonCppApiPrj.setVar, "ReleaseType", "Release"),
)

SetTestVars = Project.Stage("SetTestVars")
SetTestVars.add_funcs(
    Project.Func("", AyonCppApiPrj.setVar, "BuildTest", "ON"),
    Project.Func("", AyonCppApiPrj.setVar, "JTRACE", "1"),
    Project.Func("", AyonCppApiPrj.setVar, "ReleaseType", "Release"),
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
