@echo off
cd /d "%~dp0"
title Step 3 - Video mode 1280x720@30
echo ==========================================================
echo  STEP 3  Video mode / 1280x720 @ 30fps / 180 frames
echo ----------------------------------------------------------
echo  A video window opens and plays by itself.
echo  Overlay in the top-left corner shows:
echo      Frame : current frame / total frames
echo      Size  : resolution
echo      FPS   : real processing speed
echo  It exits by itself when the video ends.
echo  To test the Esc key instead: click the video window once
echo  (to give it focus), then press Esc.
echo.
echo  Result is saved to: output\motion.mp4
echo ==========================================================
echo.
cv_hw.exe video assets\motion.mp4
echo.
echo ----------------------------------------------------------
echo  Exit code = %ERRORLEVEL%   (0 = success)
echo ----------------------------------------------------------
pause
