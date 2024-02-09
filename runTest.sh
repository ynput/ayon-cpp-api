#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
TESTDIR=$SCRIPT_DIR/test



#-------- Seting Default Vars
# export AYON_SITE_ID="optimal-boisterous-jaybird"

# export AYON_API_KEY="dca3d942a68887ab275cb3ca66b04d8a99367a8298c8e9f45b30fe9a11acba02"
# export AYON_SERVER_URL="http://192.168.178.42:5000"


export AYON_API_KEY="6eb4fdf0ed97b215a397377aa5e56862766eb1339a16a873f206ff93caee4cf7"
export AYON_SERVER_URL="http://usd.ayon.app"

cd $TESTDIR

./test.sh
