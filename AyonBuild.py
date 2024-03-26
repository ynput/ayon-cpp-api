from os.path import abspath, exists
import subprocess
import shutil
from sys import path
from AyonCiCd import pipfuncs, Cmake, Project 
import os

# foulderfuncs.check_and_install_package("cmake")
# foulderfuncs.check_and_install_package("importlib-metadata")

# shutil.rmtree("build", ignore_errors=True)
# shutil.rmtree("bin", ignore_errors=True)
#
#
# with Cmake.CMakeCommandsContext("BuildManifest") as cmake:
#     cmake.add_command(["cmake", "-S", ".", "-B", "build", "-DBUILD_TEST=OFF", "-DJTRACE=0", "-DCMAKE_BUILD_TYPE=Release"])
#     cmake.add_command(["cmake", "--build", "build", "--config", "Release"])
#     cmake.add_command(["cmake", "--install", "build"])

AyonCppApiPrj = Project.Project("BuildArtefacts")


CleanUpStage = Project.Stage("Cleanup")

binFoulder = os.path.join(os.getcwd(), "bin")
buildFoulder = os.path.join(os.getcwd(), "build")
CleanUpStage.addFuncs(
    lambda: print("build foulder path:", buildFoulder),
    lambda: shutil.rmtree(buildFoulder, ignore_errors=True) if os.path.exists(buildFoulder) else print("no build foulder"),

    lambda: print("build foulder path:", binFoulder),
    lambda: shutil.rmtree(binFoulder, ignore_errors=True) if os.path.exists(binFoulder) else print("no bin foulder"),
)

AyonCppApiPrj.addStage(CleanUpStage)


BuildStage = Project.Stage("Build")
BuildStage.addFuncs(
    lambda: Cmake.Command("-S", ".", "-B", "build", "-DBUILD_TEST=OFF", "-DJTRACE=0", "-DCMAKE_BUILD_TYPE=Release"),
    lambda: Cmake.Command("--build", "build", "--config", "Release"),
    lambda: Cmake.Command("--install", "build"),
)
AyonCppApiPrj.addStage(BuildStage)

AyonCppApiPrj.execAllStages()

