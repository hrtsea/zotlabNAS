@echo off
chcp 65001 >nul
set PLATFORMIO_NO_UV=1
cd f:\nasmonitor\TuneBar-main\sketch
echo Uploading firmware to COM5...
echo.
"C:\Users\steven\.platformio\penv\Scripts\platformio.exe" run --target upload --environment esp32-s3-devkitc1-n16r8
if %ERRORLEVEL% neq 0 (
    echo Upload failed with error code %ERRORLEVEL%
    echo.
    echo Trying alternative method...
    echo.
    "C:\Users\steven\.platformio\penv\Scripts\esptool.exe" --port COM5 --baud 921600 --after hard_reset write_flash 0x0 .pio\build\esp32-s3-devkitc1-n16r8\firmware.bin
)
pause