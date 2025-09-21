@echo off
REM TankDuel Windows Build Script
REM This script builds TankDuel for Windows using Visual Studio or MinGW

setlocal EnableDelayedExpansion

echo TankDuel Windows Build Script
echo =============================

REM Parse command line arguments
set BUILD_TYPE=Release
set CLEAN_BUILD=false
set PACKAGE=false
set GENERATOR=""

:parse_args
if "%~1"=="" goto end_parse
if "%~1"=="-d" (
    set BUILD_TYPE=Debug
    shift
    goto parse_args
)
if "%~1"=="--debug" (
    set BUILD_TYPE=Debug
    shift
    goto parse_args
)
if "%~1"=="-c" (
    set CLEAN_BUILD=true
    shift
    goto parse_args
)
if "%~1"=="--clean" (
    set CLEAN_BUILD=true
    shift
    goto parse_args
)
if "%~1"=="-p" (
    set PACKAGE=true
    shift
    goto parse_args
)
if "%~1"=="--package" (
    set PACKAGE=true
    shift
    goto parse_args
)
if "%~1"=="--vs2019" (
    set GENERATOR="Visual Studio 16 2019"
    shift
    goto parse_args
)
if "%~1"=="--vs2022" (
    set GENERATOR="Visual Studio 17 2022"
    shift
    goto parse_args
)
if "%~1"=="--mingw" (
    set GENERATOR="MinGW Makefiles"
    shift
    goto parse_args
)
if "%~1"=="-h" goto show_help
if "%~1"=="--help" goto show_help

echo Unknown option: %~1
goto show_help

:show_help
echo Usage: %0 [OPTIONS]
echo Options:
echo   -d, --debug     Build in Debug mode (default: Release)
echo   -c, --clean     Clean build directory before building
echo   -p, --package   Create distribution packages
echo   --vs2019        Use Visual Studio 2019 generator
echo   --vs2022        Use Visual Studio 2022 generator
echo   --mingw         Use MinGW generator
echo   -h, --help      Show this help message
exit /b 0

:end_parse

REM Get script directory
set SCRIPT_DIR=%~dp0
set PROJECT_ROOT=%SCRIPT_DIR%..

cd /d "%PROJECT_ROOT%"

set BUILD_DIR=build

REM Clean build if requested
if "%CLEAN_BUILD%"=="true" (
    echo Cleaning build directory...
    if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
)

REM Create build directory
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
cd /d "%BUILD_DIR%"

REM Auto-detect generator if not specified
if "%GENERATOR%"=="" (
    echo Auto-detecting build environment...
    where cl >nul 2>&1
    if !errorlevel! equ 0 (
        echo Found Visual Studio compiler
        REM Try to detect VS version
        if defined VS170COMNTOOLS (
            set GENERATOR="Visual Studio 17 2022"
            echo Using Visual Studio 2022
        ) else if defined VS160COMNTOOLS (
            set GENERATOR="Visual Studio 16 2019"
            echo Using Visual Studio 2019
        ) else (
            set GENERATOR="Visual Studio 16 2019"
            echo Using default Visual Studio generator
        )
    ) else (
        where gcc >nul 2>&1
        if !errorlevel! equ 0 (
            echo Found GCC compiler, using MinGW
            set GENERATOR="MinGW Makefiles"
        ) else (
            echo No suitable compiler found. Please install Visual Studio or MinGW.
            exit /b 1
        )
    )
)

echo Using generator: !GENERATOR!

REM Configure
echo Configuring CMake...
cmake .. -G !GENERATOR! -DCMAKE_BUILD_TYPE=%BUILD_TYPE%
if !errorlevel! neq 0 (
    echo CMake configuration failed!
    exit /b 1
)

REM Build
echo Building TankDuel...
cmake --build . --config %BUILD_TYPE%
if !errorlevel! neq 0 (
    echo Build failed!
    exit /b 1
)

REM Check if executable was created
if exist "%BUILD_TYPE%\TankDuel.exe" (
    echo Build successful! Executable created: %BUILD_TYPE%\TankDuel.exe
) else if exist "TankDuel.exe" (
    echo Build successful! Executable created: TankDuel.exe
) else (
    echo Build failed! Executable not found.
    exit /b 1
)

REM Package if requested
if "%PACKAGE%"=="true" (
    echo Creating distribution packages...
    cmake --build . --target package --config %BUILD_TYPE%
    if !errorlevel! equ 0 (
        echo Packages created successfully!
        echo Available packages:
        dir /b *.exe *.msi *.zip 2>nul
    ) else (
        echo Package creation failed.
    )
)

echo Build complete!
endlocal