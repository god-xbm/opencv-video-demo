@echo off
cd /d "%~dp0"
title Step 2 - Image mode (real photo)
echo ==========================================================
echo  STEP 2  Image mode / real photo
echo ----------------------------------------------------------
echo  Opens a 1920x1200 JPEG photo.
echo  Watch the console for the width / height / channels line.
echo  Clicking also works here, if you want to sample a pixel.
echo  Press ANY KEY in the image window to close it.
echo ==========================================================
echo.
cv_hw.exe img assets\photo2.jpg
echo.
echo ----------------------------------------------------------
echo  Exit code = %ERRORLEVEL%   (0 = success)
echo ----------------------------------------------------------
pause
