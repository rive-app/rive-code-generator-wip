# Define the possible paths
release_path="../build/out/lib/release/rive_code_generator"
debug_path="../build/out/lib/debug/rive_code_generator"

# Check for release build first, then debug build
if [[ -f "$release_path" ]]; then
    generator_path="$release_path"
elif [[ -f "$debug_path" ]]; then
    generator_path="$debug_path"
else
    echo "Error: rive_code_generator not found in release or debug builds"
    exit 1
fi

# Execute the generator with the stored path
"$generator_path" --help

# Generate a JSON file from all the sample .riv files using a custom template
"$generator_path" -i ../samples/ -o generated/rive_all.json -t ../templates/json_template.mustache

# Generate a Dart file from one .riv file
"$generator_path" -i ../samples/rating.riv -o generated/rive_rating.dart -t ../templates/dart_template.mustache

