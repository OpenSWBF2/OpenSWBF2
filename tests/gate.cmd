@echo off
setlocal
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" goto novs
set "VS="
for /f "delims=" %%i in ('"%VSWHERE%" -latest -version [17.0^,18.0^) -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath') do set "VS=%%i"
if not exist "%VS%\MSBuild\Current\Bin\MSBuild.exe" goto novs
"%VS%\MSBuild\Current\Bin\MSBuild.exe" "%~dp0gate\OpenSWBF2Gate.vcxproj" /p:Configuration=Release /p:Platform=x64 /m /v:quiet /nologo
if errorlevel 1 goto nobuild
"%~dp0..\bin\Tools\OpenSWBF2Gate.exe" %*
exit /b %ERRORLEVEL%
:novs
echo gate: Visual Studio 2022 with the C++ x86/x64 build tools not found.
exit /b 2
:nobuild
echo gate: the gate tool did not build.
exit /b 2
