#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
TESTDIR=$SCRIPT_DIR/test



#-------- Seting Default Vars
# export AYON_SITE_ID="optimal-boisterous-jaybird"

export AYON_API_KEY="ecff123e0fa1b5f4c9d5341494158f0c722d95ee1d6648626a60ec29f168b8ee"
export AYON_SERVER_URL="http://192.168.178.42:5000"

export AYONLOGGERLOGLVL="WARN"
export AYONLOGGERFILELOGGING="ON"
export AYONLOGGERFILEPOS="testFoulder"

# export AYON_API_KEY="6eb4fdf0ed97b215a397377aa5e56862766eb1339a16a873f206ff93caee4cf7"
# export AYON_SERVER_URL="http://usd.ayon.app"

cd $TESTDIR

./test.sh
