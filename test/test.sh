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

MIN_PATHS=3000
TESTITERATIONS=2
MIN_VARIENT=0
MAX_VARIENT=200


./test_app "ayon://Usd_Base/UsdTesting?product=usdUsdTest_" "&version=v001&representation=usd" "$MIN_PATHS" "$TESTITERATIONS" "$MIN_VARIENT" "$MAX_VARIENT" 

echo "Ending Testing"
