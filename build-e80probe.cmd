@echo off
REM BACKLOG E80 -- build the bare CLAP host probe (Windows).
REM
REM The CLAP headers come from whichever TIDE build tree you have; pass its
REM path as %1. Default is the tree name this run used. It used to be
REM hard-coded to C:\SE\TideSynth\build-e19win, which is one box's scratch
REM tree from 2026-09-02 and does not exist in a git worktree (2026-09-16).
setlocal
set "TIDE_BUILD=%~1"
if "%TIDE_BUILD%"=="" set "TIDE_BUILD=build-e80cen"
if not exist "%TIDE_BUILD%\_deps\clap-src\include" (
  echo no CLAP headers under "%TIDE_BUILD%\_deps\clap-src\include"
  echo usage: build-e80probe.cmd [path-to-a-configured-TIDE-build-tree]
  exit /b 1
)
call :vcvars || exit /b 1
cl /std:c11 /nologo /O2 /I "%TIDE_BUILD%\_deps\clap-src\include" ^
   tests\e80_clap_feedback_probe.c /Fe:e80probe.exe /link user32.lib
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
