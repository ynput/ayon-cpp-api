SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR=$SCRIPT_DIR/build








# Set the desired value for BUILD_SHARED_LIBS
export BUILD_SHARED_LIBS=ON

# Run CMake with the desired options
cmake -D BUILD_SHARED_LIBS=$BUILD_SHARED_LIBS <path_to_dependency_source_directory>





DEBUG=0
CLEAN_BUILD=0

#Checks for statup commands 
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



if ["$CLEAN_BUILD" -eq 1]; then
  echo "Clean build is activated"
  rm -rf bin
  rm -rf build
  mkdir bin
  mkdir build
fi

if [ "$DEBUG" -eq 1 ]; then

  BUILD_TEST="ON"
else
  BUILD_TEST="OFF"
fi


cmake . -B build -DBUILD_TEST=$BUILD_TEST

if [ "$CLEAN_BUILD" -eq 1 ]; then 
  cmake --build build --clean-first
else
  cmake --build build
fi


cmake --install build    



if [ "$DEBUG" -eq 1 ]; then
  echo "Debbug is on"
  cd $SCRIPT_DIR/bin 
  ./test_app
fi


