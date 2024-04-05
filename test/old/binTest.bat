@echo off
set "AYON_SITE_ID={site id}"
set "AYON_API_KEY={token}"
set "AYON_SERVER_URL={server adress}"
REM ./test_app.exe "ayon://Usd_Base/trees?product=usdtree_" "&version=*&representation=usd" "10" "1" "1" "1" 
"%~dp0test_app.exe" "ayon://Usd_Base/trees?product=usdtree_" "&version=*&representation=usd" "10" "1" "1" "1"
pause
