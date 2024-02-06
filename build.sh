SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR=$SCRIPT_DIR/build



#-------- Seting Default Vars 
DEBUG=0
CLEAN_BUILD=0
BUILD_TEST="OFF"
JTRACE=0

  

#----------- Checking for build mode
if [ "$1" == "Debug" ]; then
  echo "Build running in debug mode"
  DEBUG=1
fi
if [[ " $@ " =~ " Clean " ]]; then
  echo "Running Clean Build"
  CLEAN_BUILD=1
fi
if [[ " $@ " =~ " Debug " && " $@ " =~ " Clean " ]]; then
  echo "Running Debug and Clean Build"
  DEBUG=1
  CLEAN_BUILD=1
fi




#------------ Setting up project depending on build setup
if [ "$CLEAN_BUILD" -eq 1 ]; then
  echo "Clean build is activated"
  rm -rf bin
  rm -rf build
  mkdir bin
  mkdir build
fi

if [ "$DEBUG" -eq 1 ]; then
  BUILD_TEST="ON"
  JTRACE=1
fi


#-------------- Cmake / Build Commands 
cmake . -B build -DBUILD_TEST=$BUILD_TEST -DJTRACE=$JTRACE

if [ "$CLEAN_BUILD" -eq 1 ]; then 
  cmake --build build --clean-first
else
  cmake --build build
fi

cmake --install build    


if [ "$DEBUG" -eq 0 ]; then
  doxygen Doxyfile
fi
 


#------------- Optional testing 
#TODO update this to be laucnhed via AYON 
# if [ "$DEBUG" -eq 1 ]; then
#   echo "Debbug is on"
#   cd $SCRIPT_DIR/bin 
#   ./test_app
# fi
#
#
