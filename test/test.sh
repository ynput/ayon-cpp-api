#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BIN_DIR=$SCRIPT_DIR/../bin
cd $BIN_DIR

export AYONLOGGERLOGLVL="INFO"
export AYONLOGGERFILELOGGING="ON"
export AYONLOGGERFILEPOS="LoggingFiles"

export AYON_SITE_ID=""
export AYON_API_KEY=""
export AYON_SERVER_URL=""

#export AYON_LOGGIN_LOGGIN_KEYS="AyonApi/AyonApiDebugEnvVars/" # the key needs the /	 at the end to work
#export AYON_LOGGIN_LOGGIN_KEYS="AyonApiDebugEnvVars/" # the key needs the /	 at the end to work
#export AYON_LOGGIN_LOGGIN_KEYS="AyonApi/"


./test_app

echo "Ending Testing"
