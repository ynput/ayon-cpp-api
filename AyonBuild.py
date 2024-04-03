import shutil
from AyonCiCdTools.AyonCiCd import Cmake, Project, docGen
import os

# define a Project that you want to CiCd
AyonCppApiPrj = Project.Project("AyonCppApi")
# add packages to the project
AyonCppApiPrj.addPipPackage("pytest")


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
    lambda: Cmake.Command("-S", ".", "-B", "build", "-DBUILD_TEST=OFF", "-DJTRACE=0", "-DCMAKE_BUILD_TYPE=Release"),
    lambda: Cmake.Command("--build", "build", "--config", "Release"),
    lambda: Cmake.Command("--install", "build"),
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


TestStage = Project.Stage("Test")
TestStage.addFuncs(
    lambda: print("Start Testing")
)
AyonCppApiPrj.addStage(TestStage)

# make the CiCd class available to the Cli so we can interact with it
AyonCppApiPrj.makeClassCliAvailable()
