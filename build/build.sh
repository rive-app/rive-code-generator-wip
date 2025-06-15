#!/bin/bash
set -euo pipefail

echo "Building rive_code_generator"

CONFIG=debug
KIND=
CLEAN=false
RUN=false
DEV=false
RUNTIME_PATH="$PWD/../rive-runtime/build"

for var in "$@"; do
  case "$var" in
    --help|-h)
      echo "Usage: ./build.sh [release] [clean] [run] [dev]"
      exit 0
      ;;
  esac
done

for var in "$@"; do
  case "$var" in
    release) CONFIG=release ;;
    clean) CLEAN=true ;;
    run) RUN=true ;;
    dev) DEV=true ;;
  esac
done

unameOut="$(uname -s)"
case "${unameOut}" in
Linux*) machine=linux ;;
Darwin*) machine=macosx ;;
MINGW*) machine=windows ;;
*) machine="unhandled:${unameOut}" ;;
esac
OS=$machine

if [[ $OS == "windows" ]]; then
    if ! command -v msbuild.exe &>/dev/null; then
        powershell "./build.ps1" $@
        exit $?
    fi
fi
if [[ $OS = "linux" ]]; then
    LOCAL_ARCH=$('arch')
    if [[ $LOCAL_ARCH == "aarch64" ]]; then
        LINUX_ARCH=arm64
    else
        LINUX_ARCH=x64
    fi
fi

# Setup PREMAKE
download_premake() {
    mkdir -p dependencies/bin
    pushd dependencies/bin
    echo Downloading Premake5
    if [[ $OS = "macosx" ]]; then
        curl https://github.com/premake/premake-core/releases/download/v5.0.0-beta3/premake-5.0.0-beta3-macosx.tar.gz -L -o premake_macosx.tar.gz
        # Export premake5 into bin
        tar -xvf premake_macosx.tar.gz 2>/dev/null
        # the zip for beta3 does not have x
        chmod +x premake5
        # Delete downloaded archive
        rm premake_macosx.tar.gz
    elif [[ $OS = "windows" ]]; then
        curl https://github.com/premake/premake-core/releases/download/v5.0.0-beta3/premake-5.0.0-beta3-windows.zip -L -o premake_windows.zip
        unzip premake_windows.zip
        rm premake_windows.zip
    elif [[ $OS = "linux" ]]; then
        pushd ..
        git clone --depth 1 --branch v5.0.0-beta2 https://github.com/premake/premake-core.git
        pushd premake-core
        if [[ $LINUX_ARCH == "arm64" ]]; then
            PREMAKE_MAKE_ARCH=ARM
        else
            PREMAKE_MAKE_ARCH=x86
        fi
        make -f Bootstrap.mak linux PLATFORM=$PREMAKE_MAKE_ARCH
        cp bin/release/* ../bin
        popd
        popd
        # curl https://github.com/premake/premake-core/releases/download/v5.0.0-beta2/premake-5.0.0-beta2-linux.tar.gz -L -o premake_linux.tar.gz
        # # Export premake5 into bin
        # tar -xvf premake_linux.tar.gz 2>/dev/null
        # # Delete downloaded archive
        # rm premake_linux.tar.gz
    fi
    popd
}
if [[ ! -f "dependencies/bin/premake5" ]]; then
    download_premake
fi

if [[ ! -d "dependencies/export-compile-commands" ]]; then
    pushd dependencies
    git clone https://github.com/tarruda/premake-export-compile-commands export-compile-commands
    popd
fi

export PREMAKE=$PWD/dependencies/bin/premake5

case "$OS" in
  macosx|linux) TARGET=gmake2 ;;
  windows) TARGET=vs2022; KIND=--shared; EXTRA_OUT=_shared ;;
esac

# export PREMAKE_PATH="$RUNTIME_PATH":$PREMAKE_PATH

if [[ $CLEAN == true ]]; then
    echo 'Cleaning...'
    rm -fR out
fi

OUT="out/lib/$CONFIG"
OUT="${OUT%/}" # Remove trailing slash if it exists
$PREMAKE --scripts=../rive-runtime/build/ --file=premake5_code_generator.lua $TARGET --config=$CONFIG --out=$OUT

if [ "$OS" = "macosx" ]; then
  NUM_CORES=$(($(sysctl -n hw.physicalcpu) + 1))
elif [ "$OS" = "linux" ]; then
  NUM_CORES=$(nproc)
elif [ "$OS" = "windows" ]; then
  NUM_CORES=$NUMBER_OF_PROCESSORS
else
  echo "Unsupported OS: $OS"
  exit 1
fi

if [[ $OS = "windows" ]]; then
    pushd "$OUT"
    msbuild.exe rive.sln -m:$NUM_CORES
    popd
else
    make -C $OUT -j$NUM_CORES
fi

EXECUTABLE_NAME=rive_code_generator
if [[ $OS = "windows" ]]; then
    EXECUTABLE="$EXECUTABLE_NAME.exe"
else
    EXECUTABLE="$EXECUTABLE_NAME"
fi

echo -e "\033[0;32m\nBuild complete: $OUT/$EXECUTABLE\033[0m"

if [[ $RUN == true ]]; then
    "$OUT/rive_code_generator" --help
fi
if [[ $DEV == true ]]; then
    pwd
    # "$OUT/$EXECUTABLE" -i ../samples/signage_v03.riv -o out/rive_generated.dart -t ../templates/dart_template.mustache
    # "$OUT/$EXECUTABLE" -i ../samples/rating.riv -o out/rive_generated.dart -t ../templates/dart_template.mustache
    # "$OUT/$EXECUTABLE" -i ../samples/ -o out/generated/rive_generated.dart -t ../templates/dart_template.mustache
    "$OUT/$EXECUTABLE" -i ../samples/ -o out/generated/rive_viewmodel.dart -t ../templates/viewmodel_template.mustache
    # "$OUT/$EXECUTABLE" -i ../samples/ -o out/generated/rive.json -t ../templates/json_template.mustache
    # "$OUT/$EXECUTABLE" -i ../samples/nested_test.riv -o out/rive_generated.dart --help
    # "$OUT/$EXECUTABLE" -i ../samples/ -o out/rive_generated.dart
fi