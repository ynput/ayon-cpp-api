@echo off
set "AYON_SITE_ID=groovy-amiable-reindeer"
set "AYON_API_KEY=4ca78d976737ec6588131c5fe95e5f841b65c1e625b5ce723d107942b03eeec4"
set "AYON_SERVER_URL=http://192.168.178.42:5000"
REM ./test_app.exe "ayon://Usd_Base/trees?product=usdtree_" "&version=*&representation=usd" "10" "1" "1" "1" 
"%~dp0test_app.exe" "ayon://Usd_Base/trees?product=usdtree_" "&version=*&representation=usd" "10" "1" "1" "1"
pause