#!/bin/sh
set -e
echo "Building rive_code_generator"


source ../rive-runtime/dependencies/config_directories.sh
# if [[ "$OSTYPE" == "darwin"* ]]; then
#     # macOS-specific code
#     echo "Detected macOS. Running macOS build script..."
#     source ../rive-runtime/dependencies/macosx/config_directories.sh
# else
#     echo "This script currently only supports macOS."
#     exit 1
# fi

CONFIG=debug

for var in "$@"; do
    if [[ $var = "release" ]]; then
        CONFIG=release
    fi
done

if [[ ! -f "$DEPENDENCIES/bin/premake5" ]]; then
    pushd $DEPENDENCIES_SCRIPTS
    ./get_premake5.sh
    popd
fi

echo "Using premake5 from $DEPENDENCIES/bin/premake5"
export PREMAKE=$DEPENDENCIES/bin/premake5
$PREMAKE --version

export PREMAKE_PATH="$PWD/../rive-runtime/build":$PREMAKE_PATH

OUT=out/lib/$CONFIG
$PREMAKE --scripts=../build --file=./premake5_code_generator.lua --no-rive-decoders --config=$CONFIG --out=$OUT gmake2

for var in "$@"; do
    if [[ $var = "clean" ]]; then
        make -C $OUT clean
    fi
done

make -C $OUT -j$(($(sysctl -n hw.physicalcpu) + 1))

for var in "$@"; do
    if [[ $var = "run" ]]; then
        $OUT/rive_code_generator --help
    fi
    if [[ $var = "dev" ]]; then
        # $OUT/rive_code_generator -i ../samples/signage_v03.riv -o out/rive_generated.dart -t ../templates/dart_template.mustache
        # $OUT/rive_code_generator -i ../samples/rating.riv -o out/rive_generated.dart -t ../templates/dart_template.mustache
        # $OUT/rive_code_generator -i ../samples/ -o out/generated/rive_generated.dart -t ../templates/dart_template.mustache
        $OUT/rive_code_generator -i ../samples/ -o out/generated/rive_viewmodel.dart -t ../templates/viewmodel_template.mustache
        # $OUT/rive_code_generator -i ../samples/ -o out/generated/rive.json -t ../templates/json_template.mustache
        # $OUT/rive_code_generator -i ../samples/nested_test.riv -o out/rive_generated.dart --help
        # $OUT/rive_code_generator -i ../samples/ -o out/rive_generated.dart
    fi
done