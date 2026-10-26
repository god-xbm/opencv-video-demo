@echo off
cd /d "%~dp0"
title Step 4 - Video mode 640x480@25 AVI
echo ==========================================================
echo  STEP 4  Video mode / 640x480 @ 25fps / 125 frames
echo ----------------------------------------------------------
echo  Second test clip, different resolution, fps and container
echo  (.avi instead of .mp4). Proves the code is not hard-coded
echo  to one specific file.
echo.
echo  Result is saved to: output\motion2.mp4
echo ==========================================================
echo.
cv_hw.exe video assets\motion2.avi
echo.
echo ----------------------------------------------------------
echo  Exit code = %ERRORLEVEL%   (0 = success)
echo ----------------------------------------------------------
pause
