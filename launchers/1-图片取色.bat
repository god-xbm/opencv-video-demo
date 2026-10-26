@echo off
cd /d "%~dp0"
title Step 1 - Image mode (color picker)
echo ==========================================================
echo  STEP 1  Image mode / color picker
echo ----------------------------------------------------------
echo  A window will open showing a picture of color blocks.
echo  Use the LEFT mouse button to click a color block.
echo  This console will print the X,Y position and B/G/R value.
echo  Press ANY KEY in the image window to close it.
echo ==========================================================
echo.
cv_hw.exe img assets\calib_bgr.png
echo.
echo ----------------------------------------------------------
echo  Exit code = %ERRORLEVEL%   (0 = success)
echo ----------------------------------------------------------
pause
