@echo off
echo ============================================
echo  ToyOS - Debug (QEMU + GDB)
echo ============================================
echo Starting QEMU (paused, GDB server on :1234)...
start "" "D:\Software\Qemu\qemu-system-i386.exe" -m 4G -fda build\floppy.img -s -S -debugcon stdio
timeout /t 1 >nul
echo Connecting GDB...
echo.
"D:\Software\C++\w64devkit\bin\gdb.exe" -x debug.gdb
