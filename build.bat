@echo off
rem Usage: build.bat        - build d3d9.dll and the config tool
rem        build.bat probe  - build the logging (investigation) d3d9.dll
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars32.bat" >nul 2>&1
cd /d %~dp0
if not exist out mkdir out
set SRC=%1
if "%SRC%"=="" set SRC=main
cl /nologo /O2 /MT /LD /EHsc /utf-8 /D_CRT_SECURE_NO_WARNINGS src\%SRC%.cpp /Fo:out\ /Fe:out\d3d9.dll /link /DEF:src\d3d9.def user32.lib gdi32.lib || exit /b 1
if not "%SRC%"=="main" exit /b 0
cl /nologo /O2 /MT /EHsc /utf-8 /DUNICODE /D_UNICODE /D_CRT_SECURE_NO_WARNINGS src\config.cpp /Fo:out\ /Fe:out\dynamarisa_hd_config.exe /link /SUBSYSTEM:WINDOWS user32.lib gdi32.lib comctl32.lib || exit /b 1
