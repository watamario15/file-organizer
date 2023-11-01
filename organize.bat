@echo off

set /p extensions="Enter a file name pattern (*.txt means all files with the .txt extension): "
set /p sepunit="Enter a separation unit (ex. 100): "
echo "Processing..."

set /a dirnum=0
set /a i=0
set /a amari=0

setlocal enabledelayedexpansion

for %%s in ("%extensions%") do (
  set /a amari=!i!%%!sepunit!

  if /I !amari! equ 0 (
    set /a dirnum=!dirnum!+1
    mkdir !dirnum!
  )

  set /a i+=1

  echo !dirnum!

  move "%%s" "!dirnum!\%%s"
)

endlocal

echo "Successfully organized."
pause
