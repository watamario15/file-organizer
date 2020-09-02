@echo off

set /p extensions="ファイル名を入力して下さい。(例: *.txtでtxtファイル全て)"
set /p sepunit="分割単位を指定してください｡(例: 100)"
echo "処理中..."

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

echo 処理が終了しました｡
pause