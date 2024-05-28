
# AYON CPP API

An API Wrapper for [AYON server](https://ayon.ynput.io/) written in cpp

> [!NOTE]\
> we use [git-Submodules](https://git-scm.com/book/en/v2/Git-Tools-Submodules) to pull in external dependencys so you might want to use
> these commands
> `git clone --recurse-submodules git@github.com:ynput/ayon-cpp-api.git` to
> clone the repo with all its submodules.   
> `git submodule update --init --recursive` to can then be used to initialize [Submodule](https://git-scm.com/book/en/v2/Git-Tools-Submodules) when you change the branch or update other [Submodule](https://git-scm.com/book/en/v2/Git-Tools-Submodules) related parts

> [!NOTE]\
> we use tags in our [.gitmodules](https://git-scm.com/docs/gitmodules) in order to give a better overview towards the used [Submodules](https://git-scm.com/book/en/v2/Git-Tools-Submodules) used in this repo. 
> you might need those commands if you change [Submodule](https://git-scm.com/book/en/v2/Git-Tools-Submodules) tags. 
>
> ```sh
> git submodule sync
> git submodule update --remote
> ```

> [!IMPORTANT]\
> we allways run tests for both linux and windows on the compiled libarys but the developemnt setup is build on linux with the intention to be as cross platform as posible. but it can allways happend that something is not working on windows so if you have a wish or a bug report it. 


## Requirements:

- C++ Compiler
- Cmake
- Python
- pip

## Tested Platforms:
- Alma Linux9
- win 10

## Build Setps

the build setup recently moved from a .sh/.bat file into python in order to
allow for easy Usage. The python setup also includes optional tests.

The python setup exposes functions to the CLI and it can be run with every
python executable. But you will need pip for the Automatic package Installation.

Set up the build Env

```sh
{python} AyonBuild.py setup # Sets up the Venv and artifact's folder
```

Run one off the following build Setups

```sh
{python} AyonBuild.py runStageGRP CleanBuild # Only build the Api

{python} AyonBuild.py runStageGRP CleanBuildAndDocs # Build the Api and generate the Docs
```

## Usage / Config

## Using environment variables to Controle the API

The `AyonLogger` can be controlled with these environment variables:

| variable                  | value                                  |
| ------------------------- | -------------------------------------- |
| `AYONLOGGERLOGLVL`        | `INFO` `ERROR` `WARN` `CRITICAL` `OFF` |
| `AYONLOGGERFILELOGGING`   | `OFF` / `ON`                           |
| `AYONLOGGERFILEPOS`       | `/path/to or ./relative/path`          |
| `AYON_LOGGIN_LOGGIN_KEYS` | `AyonApi/AyonApiDebugEnvVars/`         |

The `AyonCppApi` needs these Env Variables to function (They will typically be
supplied by your Ayon Launch Env)

| variable            | value                                                    |
| ------------------- | -------------------------------------------------------- |
| `AYON_API_KEY`      | `ApiKey / BearerToken for your Ayon server`              |
| `AYON_SERVER_URL`   | `http / https addres off your Ayon server`               |
| `AYON_SITE_ID`      | `the side Id that Ayon launcher asinged to your machine` |
| `AYON_PROJECT_NAME` | `the Project name you want to work agains`               |

## Useage (Dev)

Here will be The documentation on how to Develop for the AyonCppApi

## How dose it work

Here will be a High Lvl overview on how the AyonCppAPi functions

