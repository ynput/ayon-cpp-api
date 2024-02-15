#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BIN_DIR=$SCRIPT_DIR/../bin

# Function to generate random strings
generate_random_string() {
    echo "ayon://Usd_Base/UsdTesting?product=usdUsdTest_$((RANDOM % 124))&version=v001&representation=usd"
}

# Generate a list of random strings
num_strings=5  # Change this value as needed
strings=()
for ((i = 0; i < num_strings; i++)); do
    strings+=("$(generate_random_string)")
done

# Run the C++ program with the list of random strings as arguments
cd $BIN_DIR


MIN_PATHS=400
# MIN_PATHS=8300
# MIN_PATHS=10000
# MIN_PATHS=100000
TESTITERATIONS=1
MIN_VARIENT=2
MAX_VARIENT=2


./test_app "ayon://Usd_Base/trees?product=usdtree_" "&version=*&representation=usd" "$MIN_PATHS" "$TESTITERATIONS" "$MIN_VARIENT" "$MAX_VARIENT" 


# ./test_app "ayon://Usd_Base/UsdTesting?product=modelMain_" "&version=*&representation=obj" "$MIN_PATHS" "$TESTITERATIONS" "$MIN_VARIENT" "$MAX_VARIENT" 

echo "Ending Testing"
