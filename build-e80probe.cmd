@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
cl /std:c11 /nologo /O2 /I C:\SE\TideSynth\build-e19win\_deps\clap-src\include ^
   tests\e80_clap_feedback_probe.c /Fe:e80probe.exe /link user32.lib
