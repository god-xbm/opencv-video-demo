@echo off
cd /d "%~dp0"
title Step 5 - Error handling demo
echo ==========================================================
echo  STEP 5  Error handling (no window will open here)
echo ==========================================================
echo.
echo ---- [A] no arguments at all ----------------------------
cv_hw.exe
echo    Exit code = %ERRORLEVEL%   (expect 2)
echo.
echo ---- [B] file does not exist ---------------------------
cv_hw.exe img assets\no_such_file.jpg
echo    Exit code = %ERRORLEVEL%   (expect 3)
echo.
echo ---- [C] a video file passed to image mode --------------
cv_hw.exe img assets\motion.mp4
echo    Exit code = %ERRORLEVEL%   (expect 4)
echo.
echo ---- [D] unknown mode ----------------------------------
cv_hw.exe foo assets\photo1.jpg
echo    Exit code = %ERRORLEVEL%   (expect 2)
echo.
echo ==========================================================
echo  All four should print a Chinese hint and NOT crash.
echo ==========================================================
pause
