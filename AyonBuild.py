import shutil
import os
import sys
from ayon_automator.AyonCiCd import Cmake, Project, docGen, GBench, GTest


AyonCppApiPrj = Project.Project("AyonCppApi")

AyonCppApiPrj.add_pip_package("pytest")
AyonCppApiPrj.add_pip_package("fastapi==0.109.1")
AyonCppApiPrj.add_pip_package("uvicorn[standard]==0.25.0")
AyonCppApiPrj.add_pip_package("requests")

AyonCppApiPrj.setVar("AYON_CPP_API_ENALBE_GBENCH", "OFF")
AyonCppApiPrj.setVar("AYON_CPP_API_ENALBE_GTEST", "OFF")
AyonCppApiPrj.setVar("JTRACE", "0")
AyonCppApiPrj.setVar("ReleaseType", "Release")

AyonCppApiPrj.setup_prj()


SetTestVars = Project.Stage("SetTestVars")
SetTestVars.add_funcs(
    Project.Func("", AyonCppApiPrj.setVar, "AYON_CPP_API_ENALBE_GBENCH", "ON"),
    Project.Func("", AyonCppApiPrj.setVar, "AYON_CPP_API_ENALBE_GTEST", "ON"),
    Project.Func("", AyonCppApiPrj.setVar, "JTRACE", "1"),
    Project.Func("", AyonCppApiPrj.setVar, "ReleaseType", "Release"),
)
AyonCppApiPrj.add_stage(SetTestVars)

CleanUpStage = Project.Stage("Cleanup")
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
AyonCppApiPrj.add_stage(CleanUpStage)


BuildStage = Project.Stage("Build")
BuildStage.add_funcs(
    Project.Func(
        "Run Cmake Config Commmand",
        Cmake.cmake_command,
        AyonCppApiPrj,
        None,
        "-S",
        ".",
        "-B",
        "build",
        lambda: f"-DAYON_CPP_API_ENALBE_GBENCH={AyonCppApiPrj.getVar('AYON_CPP_API_ENALBE_GBENCH')}",
        lambda: f"-DAYON_CPP_API_ENALBE_GTEST={AyonCppApiPrj.getVar('AYON_CPP_API_ENALBE_GTEST')}",
        lambda: f"-DJTRACE={AyonCppApiPrj.getVar('JTRACE')}",
        lambda: f"-DCMAKE_BUILD_TYPE={AyonCppApiPrj.getVar('ReleaseType')}",
    ),
    Project.Func(
        "Run Cmake Build Command",
        Cmake.cmake_command,
        AyonCppApiPrj,
        None,
        "--build",
        "build",
        "--config",
        lambda: f"{AyonCppApiPrj.getVar('ReleaseType')}",
        f"-j{os.cpu_count()}",
    ),
    Project.Func(
        "Run Cmake Install Command",
        Cmake.cmake_command,
        AyonCppApiPrj,
        None,
        "--install",
        "build",
    ),
)
BuildStage.addArtefactFoulder("bin")
AyonCppApiPrj.add_stage(BuildStage)


DoxyGenStage = Project.Stage("DocumentationGen")
DoxyGenStage.add_funcs(
    Project.Func("Remove Htlm Docs", shutil.rmtree, "Docs/html", ignore_errors=True),
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

    GTest.run_google_test(
        "bin/AyonCppApiGtestMain",
        f"{AyonCppApiPrj._build_artefacts_out_path}/GTest/Test.xml",
        AyonCppApiPrj,
        None,
    )


def runSerialBench():
    GBench.run_google_benchmark(
        "bin/AyonCppApiGBenchMain",
        f"{AyonCppApiPrj._build_artefacts_out_path}/GBench/SerialResolve.json",
        None,
        None,
        "--benchmark_filter=AyonCppApiSerialResolve",
    )


def runBatchBench():
    GBench.run_google_benchmark(
        "bin/AyonCppApiGBenchMain",
        f"{AyonCppApiPrj._build_artefacts_out_path}/GBench/BatchResolve.json",
        None,
        None,
        "--benchmark_filter=AyonCppApiBatchResolve",
    )


def runAllBench():
    GBench.run_google_benchmark(
        "bin/AyonCppApiGBenchMain",
        f"{AyonCppApiPrj._build_artefacts_out_path}/GBench/AllResolve.json",
        None,
        None,
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
    ServerPocVar.kill()


SetupTestServer = Project.Stage("SetupTestServer")
SetupTestServer.add_funcs(
    Project.Func("Start Test Server", startTestServer),
    Project.Func("Wait For Test Server to be Avaialbe", CheckTestServer),
)
AyonCppApiPrj.add_stage(SetupTestServer)

StopTestServer = Project.Stage("StopTestServer")
StopTestServer.add_funcs(
    Project.Func("Stop Test Server", stopTestServer),
)
AyonCppApiPrj.add_stage(StopTestServer)

TestStage = Project.Stage("Test")
TestStage.add_funcs(
    Project.Func("Start Testing", print),
    Project.Func("Run GTest", startTestApp),
)
AyonCppApiPrj.add_stage(TestStage)


BenchStage = Project.Stage("Bench")
BenchStage.add_funcs(
    Project.Func("Run GBench Serial Bench", runSerialBench),
    Project.Func("Run GBench Batch Bench", runBatchBench),
    Project.Func("Run GBench All Benchmark's", runAllBench),
)
BenchStage.addArtefactFoulder("bin/profBatch.json")
BenchStage.addArtefactFoulder("bin/profSerial.json")
AyonCppApiPrj.add_stage(BenchStage)


AyonCppApiPrj.creat_stage_group(
    "CleanBuild",
    CleanUpStage,
    BuildStage,
)
AyonCppApiPrj.creat_stage_group(
    "CleanBuildAndDocs",
    CleanUpStage,
    BuildStage,
    DoxyGenStage,
)
AyonCppApiPrj.creat_stage_group(
    "BuildAndTest",
    SetTestVars,
    BuildStage,
    SetupTestServer,
    TestStage,
    StopTestServer,
)
AyonCppApiPrj.creat_stage_group(
    "CleanBuildAndTest",
    CleanUpStage,
    SetTestVars,
    BuildStage,
    SetupTestServer,
    TestStage,
    StopTestServer,
)

AyonCppApiPrj.creat_stage_group(
    "BuildAndBnech",
    SetTestVars,
    BuildStage,
    SetupTestServer,
    BenchStage,
    StopTestServer,
)
AyonCppApiPrj.creat_stage_group(
    "CleanBuildAndBnech",
    CleanUpStage,
    SetTestVars,
    BuildStage,
    SetupTestServer,
    BenchStage,
    StopTestServer,
)
AyonCppApiPrj.creat_stage_group(
    "CleanBuildAndBnechPlusTest",
    CleanUpStage,
    SetTestVars,
    BuildStage,
    SetupTestServer,
    TestStage,
    BenchStage,
    StopTestServer,
)


with AyonCppApiPrj as PRJ:
    PRJ.make_project_cli_available()
