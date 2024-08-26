# Getting Started

This Project is build to be included via Cmake.\n The main way to interact with
the build system and the testing setup is handled via the
[Ayon automator](https://github.com/ynput/ayon-automator).\n This allows us to
implement extensive tests while still allowing for an easy Cli user interface.

## General Info's about the project and Setup.

After cloing the repo you first want to run `python AyonBuild.py setup` as with
all other Ayon automator projects this will setup the project.

After that we have a few stage Groups predefined that are worth running.

`python AyonBuild.py runStageGRP CleanBuild` \n executes a Clean build, Removes
the build and install dir before running the clean Cmake Commands

`python AyonBuild.py runStageGRP CleanBuildAndDocs`\n the same as CleanBuild but
also generates the documentation

`python AyonBuild.py runStageGRP BuildAndTest`\n this will build the cpp api
using the cmake cache and then runs all the tests

`python AyonBuild.py runStageGRP CleanBuildAndTest`\n the same as CleanBuild but
runs the tests

`python AyonBuild.py runStageGRP BuildAndBnech`\n this will build the cpp api
using the cmake cache and then runs all the Benchmark's

`python AyonBuild.py runStageGRP CleanBuildAndBnech`\n the same as CleanBuild
but runs the Benchmark's

`python AyonBuild.py execSingleStage DocumentationGen` only generates the
documentation using doxygen

## Usage

Most projects will include Ayon Cpp Api as a CMake sub project. we build the Api
as a static library for performance and ease of use.

```
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/ext/ayon-cpp-api")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/ext/ayon-cpp-api")
target_link_libraries(Your-Project AyonCppApi)
```

## Developing for this tool set

### General

When developing for this tool we try to follow the idea of Red Green development
by first building a test that fails and then implementing the system that solves
the test case.

We try to keep everything we work on in GH issues and create branches from that.
There are 3 Issue Options.

- Bug Report
- Feature Request
- Proposal

Feel free to use what every fits the scale of work you want to do but if you
work on a bigger feature that holds quite a bit of complexity a Proposal would
be important so that we can debate if the technical approach to the idea can be
implemented this way, it also allows us to go back to older proposals and see
what and why we decided for specific solutions.

### Testing

We try to have the maximum test coverage so if you find an edge case it might be
worth implementing a test case just for that even if you know that the edge case
is already solved.

Currently all tests live in the GTestMain.cpp file this is about to change in
the near future and we will build tests similar to how we do it in the
[AyonDevTools](https://github.com/ynput/ayon-cpp-dev-tools)
