# ayon-cpp-api

An API Wrapper for AYON server written in cpp

requiered: 
- C++ Compiler
- Cmake
- GitHub public key setup ( this is because the submodules are linked via git@ )

Tested Platforms: 
- AlmaLinux9
- Ubunto 22.04.3LTS
- 


git clone --recurse-submodules git@github.com:ynput/ayon-cpp-api.git

git submodule update --init --recursive

linux build 
./build.sh 
or 
./build.sh Debug ( for well debug build, also builds the testing applycatoin ) 


## Environment Varibles. 
The AyonLogger can be controlled via env variables. 

`AYONLOGGERLOGLVL` = 
`INFO,ERROR,WARN,CRITICAL,OFF`

`AYONLOGGERFILELOGGING` =
`OFF,ON`

`AYONLOGGERFILEPOS` =
`/path/to or relPath`
