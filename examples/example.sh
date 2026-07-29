# Get the directory where this script is located
script_dir="$(cd "$(dirname "$0")" && pwd)"

# Define the possible paths
release_path="$script_dir/../build/out/lib/release/rive_code_generator"
debug_path="$script_dir/../build/out/lib/debug/rive_code_generator"

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

# Generate a JSON file from all the sample .riv files using Mustache template
"$generator_path" -i "$script_dir/../samples/" -o "$script_dir/generated/rive_all.json" -t "$script_dir/../templates/mustache/json_template.mustache"

# Generate a Dart file from one .riv file using Mustache template
"$generator_path" -i "$script_dir/../samples/rating.riv" -o "$script_dir/generated/rive_rating.dart" -t "$script_dir/../templates/mustache/dart_template.mustache"

# Generate Swift code using inja template with all sample .riv files
# Note: Using modular swift_example.inja with {% include %} directives
# Run from parent directory so includes can resolve correctly
(cd "$script_dir/.." && "$generator_path" -i "$script_dir/../samples/" -o "$script_dir/generated/rive_all.swift" -t "$script_dir/../templates/custom/Swift/swift_example.inja" -e inja)

# Generate Swift code using inja template for each individual .riv file
for riv_file in "$script_dir/../samples/"*.riv; do
    filename=$(basename "$riv_file" .riv)
    echo "Generating Swift code for $filename..."
    (cd "$script_dir/.." && "$generator_path" -i "$riv_file" -o "$script_dir/generated/${filename}.swift" -t "$script_dir/../templates/custom/Swift/swift_example.inja" -e inja)
done

