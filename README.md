# ayon-cpp-api

An API Wrapper for AYON server written in cpp

required: 
- C++ Compiler
- Cmake
- GitHub public key setup ( this is because the submodules are linked via git@ )

Tested Platforms: 
- AlmaLinux9
- Ubunto 22.04.3LTS
- Windows 10


git clone --recurse-submodules git@github.com:ynput/ayon-cpp-api.git

git submodule update --init --recursive

Linux build 
./build.sh 
or 
./build.sh Debug ( for well debug build, also builds the testing application ) 


## Environment Varibles. 
The AyonLogger can be controlled via env variables. 

`AYONLOGGERLOGLVL` = 
`INFO,ERROR,WARN,CRITICAL,OFF`

`AYONLOGGERFILELOGGING` =
`OFF,ON`

`AYONLOGGERFILEPOS` =
`/path/to or relPath`

`AYON_LOGGIN_LOGGIN_KEYS`=
`AyonApi/AyonApiDebugEnvVars/`
