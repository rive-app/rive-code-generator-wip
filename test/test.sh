#!/bin/bash

#region Variables
# Check if the terminal supports colors
if [ -t 1 ]; then
    GREEN=$(tput setaf 2)
    RED=$(tput setaf 1)
    YELLOW=$(tput setaf 3)
    BLUE=$(tput setaf 4)
    BOLD=$(tput bold)
    RESET=$(tput sgr0)
    HYPERLINK_START=$'\e]8;;'
    HYPERLINK_END=$'\e]8;;\e\\'
else
    GREEN="" RED="" YELLOW="" BLUE="" BOLD="" RESET=""
    HYPERLINK_START="" HYPERLINK_END=""
fi

# Define the command as a reusable variable
RIVE_GENERATOR="../build/out/lib/release/rive_code_generator"

# Add a new variable for the update flag
UPDATE_EXPECTED=false
#endregion

#region Functions
parse_args() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            --update-expected)
                UPDATE_EXPECTED=true
                shift
                ;;
            *)
                shift
                ;;
        esac
    done
}

run_test() {
    local test_name="$1"
    local command="$2"
    local expected_file="$3"
    local output_file="${4:-}"
    local line_number="${5:-${BASH_LINENO[0]}}"
    local test_result=0

    echo "${BOLD}Running test: ${test_name}${RESET}"

    if [[ -n "$output_file" ]]; then
        eval "$command -o $output_file"
        output=$(cat "$output_file")
    else
        output=$(eval "$command")
    fi

    if [[ "$UPDATE_EXPECTED" == true ]]; then
        echo "$output" > "$expected_file"
        echo "${YELLOW}Updated expected output for \"${test_name}\"${RESET}"
    else
        result=$(cat "$expected_file")

        if [[ "$output" == "$result" ]]; then
            echo "${GREEN}\"${test_name}\" test passed${RESET}"
        else
            echo "${RED}\"${test_name}\" test failed${RESET}"
            echo
            echo "${YELLOW}${BOLD}--- GOT ----${RESET}"
            echo "$output"
            echo "${YELLOW}${BOLD}--- END ---${RESET}"
            echo "${BLUE}${BOLD}--- EXPECTED ---${RESET}"
            echo "$result"
            echo "${BLUE}${BOLD}--- END ---${RESET}"
            echo
            file_path=$(realpath "${BASH_SOURCE[0]}")
            echo "${RED}Test name: \"${test_name}\"${RESET}"
            echo -e "${RED}Test failed: ${HYPERLINK_START}file://${file_path}#L${line_number}${HYPERLINK_END}${file_path}:${line_number}${HYPERLINK_END}${RESET}"
            
            test_result=1
        fi
    fi
    echo

    rm -rf "$output_file"

    return $test_result
}
#endregion

parse_args "$@"

#region Tests
# --help generates output to stdout
run_test "Help" "$RIVE_GENERATOR --help" "expected/help.txt"

# Generate correct JSON output for all .riv files
run_test "All JSON" "$RIVE_GENERATOR -i ../samples/ -t ../templates/json_template.mustache" "expected/all.json" "output/all.json"

# Generate correct Dart output for a single .riv file
run_test "Rating Dart" "$RIVE_GENERATOR -i ../samples/rating.riv -t ../templates/dart_template.mustache" "expected/rating.dart" "output/rating.dart"
#endregion

# Exit with an error if any test failed
exit ${PIPESTATUS[0]}
