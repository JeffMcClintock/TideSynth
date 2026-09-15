@echo off
REM BACKLOG E80 -- build the bare VST3 host probe (Windows).
REM Headers only: nothing from the VST3 SDK is compiled or linked.
REM
REM ole32.lib added 2026-09-16: the probe's own header comment has named it
REM since the file was written, this script did not, and --save's getState arm
REM is the first thing to need it.
setlocal
call :vcvars || exit /b 1
cl /std:c++17 /EHsc /nologo /O2 /I C:\SE\SDKs\VST3_SDK ^
   tests\e80_vst3_feedback_probe.cpp /Fe:e80vst3probe.exe /link user32.lib ole32.lib
exit /b %ERRORLEVEL%

:vcvars
REM Either Visual Studio instance builds this probe -- it needs no MFC. 18 is
REM tried first because that is the one TideSynth itself is configured against
REM on this box (CMAKE_GENERATOR_INSTANCE); 2022 is what earlier runs used.
set "VCV=C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
if not exist "%VCV%" set "VCV=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
if not exist "%VCV%" echo no vcvars64.bat found & exit /b 1
call "%VCV%" >nul
exit /b 0
