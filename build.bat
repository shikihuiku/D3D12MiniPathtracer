@echo off
setlocal

REM Find MSBuild
set MSBUILD_PATH=
for /f "tokens=*" %%i in ('"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe') do set MSBUILD_PATH=%%i

if not exist "%MSBUILD_PATH%" (
    echo Error: MSBuild not found. Please install Visual Studio 2022.
    exit /b 1
)

REM Parse arguments
set CONFIG=Debug
set PLATFORM=x64
set REBUILD=0

:parse_args
if "%1"=="" goto :build
if /i "%1"=="release" set CONFIG=Release
if /i "%1"=="debug" set CONFIG=Debug
if /i "%1"=="rebuild" set REBUILD=1
if /i "%1"=="clean" goto :clean
shift
goto :parse_args

:clean
echo Cleaning %CONFIG% build...
"%MSBUILD_PATH%" D3D12MiniPathtracer.sln /t:Clean /p:Configuration=%CONFIG% /p:Platform=%PLATFORM%
goto :end

:build
echo Building %CONFIG% configuration...
if %REBUILD%==1 (
    "%MSBUILD_PATH%" D3D12MiniPathtracer.sln /t:Rebuild /p:Configuration=%CONFIG% /p:Platform=%PLATFORM%
) else (
    "%MSBUILD_PATH%" D3D12MiniPathtracer.sln /p:Configuration=%CONFIG% /p:Platform=%PLATFORM%
)

if %ERRORLEVEL% neq 0 (
    echo Build failed!
    exit /b %ERRORLEVEL%
)

echo Build succeeded!
echo Executable: bin\%CONFIG%\D3D12MiniPathtracer.exe

:end
endlocal
