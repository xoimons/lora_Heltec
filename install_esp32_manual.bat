@echo off
REM ============================================
REM Instal·lacio manual ESP32 Arduino Core 3.3.11
REM Tanca Arduino IDE abans d'executar!
REM ============================================
setlocal enabledelayedexpansion

set ARDUINO_PKG=%LOCALAPPDATA%\Arduino15\packages\esp32
set DL=%TEMP%\esp32_downloads
set HARDWARE=%ARDUINO_PKG%\hardware\esp32\3.3.11
set TOOLS=%ARDUINO_PKG%\tools

echo ============================================
echo  Instal-lacio manual ESP32 3.3.11
echo  Directori: %ARDUINO_PKG%
echo ============================================
echo.
echo IMPORTANT: Tanca Arduino IDE abans de continuar!
echo.
pause

REM Crear directoris
mkdir "%DL%" 2>nul
mkdir "%HARDWARE%" 2>nul
mkdir "%TOOLS%" 2>nul

REM ------ PLATFORM CORE ------
echo.
echo [1/18] Descarregant esp32-core-3.3.11.zip ...
curl -L --ssl-no-revoke -o "%DL%\esp32-core-3.3.11.zip" "https://github.com/espressif/arduino-esp32/releases/download/3.3.11/esp32-core-3.3.11.zip"
if errorlevel 1 (echo ERROR descarregant core! & pause & exit /b 1)
echo      Extraient a %HARDWARE% ...
powershell -Command "Expand-Archive -Path '%DL%\esp32-core-3.3.11.zip' -DestinationPath '%DL%\core_tmp' -Force"
REM El zip pot tenir un subdirectori, copiar contingut
xcopy /E /Y /Q "%DL%\core_tmp\*" "%HARDWARE%\" >nul 2>nul
REM Si hi ha subdirectori esp32-core-3.3.11
if exist "%DL%\core_tmp\esp32-core-3.3.11" (
    xcopy /E /Y /Q "%DL%\core_tmp\esp32-core-3.3.11\*" "%HARDWARE%\" >nul 2>nul
)
echo      OK!

REM ------ TOOLS ------
REM Funcio per descarregar i extreure eines
REM Format: nom_tool, versio, URL, nom_dins_zip

echo.
echo [2/18] Descarregant esp-x32 (xtensa compiler) ...
curl -L --ssl-no-revoke -o "%DL%\esp-x32.zip" "https://github.com/espressif/crosstool-NG/releases/download/esp-14.2.0_20260121/xtensa-esp-elf-14.2.0_20260121-i686-w64-mingw32.zip"
if errorlevel 1 (echo ERROR! Continuant...)
mkdir "%TOOLS%\esp-x32\2601" 2>nul
powershell -Command "Expand-Archive -Path '%DL%\esp-x32.zip' -DestinationPath '%TOOLS%\esp-x32\2601' -Force"
echo      OK!

echo.
echo [3/18] Descarregant xtensa-esp-elf-gdb ...
curl -L --ssl-no-revoke -o "%DL%\xtensa-gdb.zip" "https://github.com/espressif/binutils-gdb/releases/download/esp-gdb-v17.1_20260402/xtensa-esp-elf-gdb-17.1_20260402-i686-w64-mingw32.zip"
if errorlevel 1 (echo ERROR! Continuant...)
mkdir "%TOOLS%\xtensa-esp-elf-gdb\17.1_20260402" 2>nul
powershell -Command "Expand-Archive -Path '%DL%\xtensa-gdb.zip' -DestinationPath '%TOOLS%\xtensa-esp-elf-gdb\17.1_20260402' -Force"
echo      OK!

echo.
echo [4/18] Descarregant esp-rv32 (RISC-V compiler) ...
curl -L --ssl-no-revoke -o "%DL%\esp-rv32.zip" "https://github.com/espressif/crosstool-NG/releases/download/esp-14.2.0_20260121/riscv32-esp-elf-14.2.0_20260121-i686-w64-mingw32.zip"
if errorlevel 1 (echo ERROR! Continuant...)
mkdir "%TOOLS%\esp-rv32\2601" 2>nul
powershell -Command "Expand-Archive -Path '%DL%\esp-rv32.zip' -DestinationPath '%TOOLS%\esp-rv32\2601' -Force"
echo      OK!

echo.
echo [5/18] Descarregant riscv32-esp-elf-gdb ...
curl -L --ssl-no-revoke -o "%DL%\riscv32-gdb.zip" "https://github.com/espressif/binutils-gdb/releases/download/esp-gdb-v17.1_20260402/riscv32-esp-elf-gdb-17.1_20260402-i686-w64-mingw32.zip"
if errorlevel 1 (echo ERROR! Continuant...)
mkdir "%TOOLS%\riscv32-esp-elf-gdb\17.1_20260402" 2>nul
powershell -Command "Expand-Archive -Path '%DL%\riscv32-gdb.zip' -DestinationPath '%TOOLS%\riscv32-esp-elf-gdb\17.1_20260402' -Force"
echo      OK!

echo.
echo [6/18] Descarregant openocd-esp32 ...
curl -L --ssl-no-revoke -o "%DL%\openocd.zip" "https://github.com/espressif/openocd-esp32/releases/download/v0.12.0-esp32-20260424/openocd-esp32-win32-0.12.0-esp32-20260424.zip"
if errorlevel 1 (echo ERROR! Continuant...)
mkdir "%TOOLS%\openocd-esp32\v0.12.0-esp32-20260424" 2>nul
powershell -Command "Expand-Archive -Path '%DL%\openocd.zip' -DestinationPath '%TOOLS%\openocd-esp32\v0.12.0-esp32-20260424' -Force"
echo      OK!

echo.
echo [7/18] Descarregant esptool_py ...
curl -L --ssl-no-revoke -o "%DL%\esptool.zip" "https://github.com/espressif/esptool/releases/download/v5.3.1/esptool-v5.3.1-windows-amd64.zip"
if errorlevel 1 (echo ERROR! Continuant...)
mkdir "%TOOLS%\esptool_py\5.3.1" 2>nul
powershell -Command "Expand-Archive -Path '%DL%\esptool.zip' -DestinationPath '%TOOLS%\esptool_py\5.3.1' -Force"
echo      OK!

echo.
echo [8/18] Descarregant mkspiffs ...
curl -L --ssl-no-revoke -o "%DL%\mkspiffs.zip" "https://github.com/igrr/mkspiffs/releases/download/0.2.3/mkspiffs-0.2.3-arduino-esp32-win32.zip"
if errorlevel 1 (echo ERROR! Continuant...)
mkdir "%TOOLS%\mkspiffs\0.2.3" 2>nul
powershell -Command "Expand-Archive -Path '%DL%\mkspiffs.zip' -DestinationPath '%TOOLS%\mkspiffs\0.2.3' -Force"
echo      OK!

echo.
echo [9/18] Descarregant mklittlefs ...
curl -L --ssl-no-revoke -o "%DL%\mklittlefs.zip" "https://github.com/earlephilhower/mklittlefs/releases/download/4.0.2/i686-w64-mingw32-mklittlefs-db0513a.zip"
if errorlevel 1 (echo ERROR! Continuant...)
mkdir "%TOOLS%\mklittlefs\4.0.2-db0513a" 2>nul
powershell -Command "Expand-Archive -Path '%DL%\mklittlefs.zip' -DestinationPath '%TOOLS%\mklittlefs\4.0.2-db0513a' -Force"
echo      OK!

echo.
echo [10/18] Descarregant esp32-libs ...
curl -L --ssl-no-revoke -o "%DL%\esp32-libs.zip" "https://github.com/espressif/arduino-esp32/releases/download/3.3.11/esp32-libs-3.3.11.zip"
if errorlevel 1 (echo ERROR! Continuant...)
mkdir "%TOOLS%\esp32-libs\3.3.11" 2>nul
powershell -Command "Expand-Archive -Path '%DL%\esp32-libs.zip' -DestinationPath '%TOOLS%\esp32-libs\3.3.11' -Force"
echo      OK!

echo.
echo [11/18] Descarregant esp32c3-libs ...
curl -L --ssl-no-revoke -o "%DL%\esp32c3-libs.zip" "https://github.com/espressif/arduino-esp32/releases/download/3.3.11/esp32c3-libs-3.3.11.zip"
if errorlevel 1 (echo ERROR! Continuant...)
mkdir "%TOOLS%\esp32c3-libs\3.3.11" 2>nul
powershell -Command "Expand-Archive -Path '%DL%\esp32c3-libs.zip' -DestinationPath '%TOOLS%\esp32c3-libs\3.3.11' -Force"
echo      OK!

echo.
echo [12/18] Descarregant esp32c5-libs ...
curl -L --ssl-no-revoke -o "%DL%\esp32c5-libs.zip" "https://github.com/espressif/arduino-esp32/releases/download/3.3.11/esp32c5-libs-3.3.11.zip"
if errorlevel 1 (echo ERROR! Continuant...)
mkdir "%TOOLS%\esp32c5-libs\3.3.11" 2>nul
powershell -Command "Expand-Archive -Path '%DL%\esp32c5-libs.zip' -DestinationPath '%TOOLS%\esp32c5-libs\3.3.11' -Force"
echo      OK!

echo.
echo [13/18] Descarregant esp32c6-libs ...
curl -L --ssl-no-revoke -o "%DL%\esp32c6-libs.zip" "https://github.com/espressif/arduino-esp32/releases/download/3.3.11/esp32c6-libs-3.3.11.zip"
if errorlevel 1 (echo ERROR! Continuant...)
mkdir "%TOOLS%\esp32c6-libs\3.3.11" 2>nul
powershell -Command "Expand-Archive -Path '%DL%\esp32c6-libs.zip' -DestinationPath '%TOOLS%\esp32c6-libs\3.3.11' -Force"
echo      OK!

echo.
echo [14/18] Descarregant esp32h2-libs ...
curl -L --ssl-no-revoke -o "%DL%\esp32h2-libs.zip" "https://github.com/espressif/arduino-esp32/releases/download/3.3.11/esp32h2-libs-3.3.11.zip"
if errorlevel 1 (echo ERROR! Continuant...)
mkdir "%TOOLS%\esp32h2-libs\3.3.11" 2>nul
powershell -Command "Expand-Archive -Path '%DL%\esp32h2-libs.zip' -DestinationPath '%TOOLS%\esp32h2-libs\3.3.11' -Force"
echo      OK!

echo.
echo [15/18] Descarregant esp32p4-libs ...
curl -L --ssl-no-revoke -o "%DL%\esp32p4-libs.zip" "https://github.com/espressif/arduino-esp32/releases/download/3.3.11/esp32p4-libs-3.3.11.zip"
if errorlevel 1 (echo ERROR! Continuant...)
mkdir "%TOOLS%\esp32p4-libs\3.3.11" 2>nul
powershell -Command "Expand-Archive -Path '%DL%\esp32p4-libs.zip' -DestinationPath '%TOOLS%\esp32p4-libs\3.3.11' -Force"
echo      OK!

echo.
echo [16/18] Descarregant esp32p4_es-libs ...
curl -L --ssl-no-revoke -o "%DL%\esp32p4_es-libs.zip" "https://github.com/espressif/arduino-esp32/releases/download/3.3.11/esp32p4_es-libs-3.3.11.zip"
if errorlevel 1 (echo ERROR! Continuant...)
mkdir "%TOOLS%\esp32p4_es-libs\3.3.11" 2>nul
powershell -Command "Expand-Archive -Path '%DL%\esp32p4_es-libs.zip' -DestinationPath '%TOOLS%\esp32p4_es-libs\3.3.11' -Force"
echo      OK!

echo.
echo [17/18] Descarregant esp32s2-libs ...
curl -L --ssl-no-revoke -o "%DL%\esp32s2-libs.zip" "https://github.com/espressif/arduino-esp32/releases/download/3.3.11/esp32s2-libs-3.3.11.zip"
if errorlevel 1 (echo ERROR! Continuant...)
mkdir "%TOOLS%\esp32s2-libs\3.3.11" 2>nul
powershell -Command "Expand-Archive -Path '%DL%\esp32s2-libs.zip' -DestinationPath '%TOOLS%\esp32s2-libs\3.3.11' -Force"
echo      OK!

echo.
echo [18/18] Descarregant esp32s3-libs ...
curl -L --ssl-no-revoke -o "%DL%\esp32s3-libs.zip" "https://github.com/espressif/arduino-esp32/releases/download/3.3.11/esp32s3-libs-3.3.11.zip"
if errorlevel 1 (echo ERROR! Continuant...)
mkdir "%TOOLS%\esp32s3-libs\3.3.11" 2>nul
powershell -Command "Expand-Archive -Path '%DL%\esp32s3-libs.zip' -DestinationPath '%TOOLS%\esp32s3-libs\3.3.11' -Force"
echo      OK!

REM ------ dfu-util (paquet arduino) ------
echo.
echo Nota: dfu-util s'instal-la des del paquet arduino.
echo Si no el tens, Arduino IDE el descarregara automaticament.

echo.
echo ============================================
echo  INSTALACIO COMPLETADA!
echo ============================================
echo.
echo Ara:
echo  1. Obre Arduino IDE
echo  2. Ves a Tools ^> Board ^> esp32
echo  3. Selecciona la teva placa ESP32
echo.
echo Si vols esborrar les descarregues temporals:
echo  rmdir /S /Q "%DL%"
echo.
pause
