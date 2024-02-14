#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
TESTDIR=$SCRIPT_DIR/test



#-------- Seting Default Vars
# export AYON_SITE_ID="optimal-boisterous-jaybird"

export AYON_API_KEY="68e60ad07956ca8c5632c2f11fd4b860c50aa439b313297a4f6e5ed064a2893e"
export AYON_SERVER_URL="http://192.168.178.42:5000"


# export AYON_API_KEY="6eb4fdf0ed97b215a397377aa5e56862766eb1339a16a873f206ff93caee4cf7"
# export AYON_SERVER_URL="http://usd.ayon.app"

cd $TESTDIR

./test.sh
