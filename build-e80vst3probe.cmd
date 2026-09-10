@echo off
REM BACKLOG E80 -- build the bare VST3 host probe (Windows).
REM Headers only: nothing from the VST3 SDK is compiled or linked.
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
cl /std:c++17 /EHsc /nologo /O2 /I C:\SE\SDKs\VST3_SDK ^
   tests\e80_vst3_feedback_probe.cpp /Fe:e80vst3probe.exe /link user32.lib
