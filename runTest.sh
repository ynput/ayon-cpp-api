#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
TESTDIR=$SCRIPT_DIR/test



#-------- Seting Default Vars
# export AYON_SITE_ID="optimal-boisterous-jaybird"

export AYON_API_KEY="7d2b717473e48a8b219103096472dc07c1103c575cb1dd24eccbece73e87c721"
export AYON_SERVER_URL="http://192.168.178.42:5000"

export AYONLOGGERLOGLVL="INFO"
export AYONLOGGERFILELOGGING="ON"
export AYONLOGGERFILEPOS="LoggingFiles"

# export AYON_API_KEY="7cefc33b3e47ef9804034dc6adaca9e92a9ceed03d341bbec46a64b09caaae7d"
# export AYON_SERVER_URL="https://usd.ayon.app"

cd $TESTDIR

./test.sh
