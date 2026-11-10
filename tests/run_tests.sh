#!/usr/bin/env bash
# Runs every tests/*.txt program and compares its output with the matching .expected file.
# pass_* tests must exit with status 0, fail_* tests with status 1.
#
# Usage: tests/run_tests.sh [path/to/interpreter]
# Without an argument, the interpreter is built with g++ into a temporary directory.

set -u
cd "$(dirname "$0")"

if [ $# -ge 1 ]; then
    interpreter="$1"
else
    build_dir="$(mktemp -d)"
    trap 'rm -rf "$build_dir"' EXIT
    interpreter="$build_dir/interpreter"
    g++ -std=c++17 -Wall -Wextra -pedantic -o "$interpreter" ../*.cpp || exit 1
fi

passed=0
failed=0
for source in *.txt; do
    name="${source%.txt}"
    actual="$("$interpreter" "$source" 2>&1)"
    status=$?
    expected="$(cat "$name.expected")"

    case "$name" in
        pass_*) expected_status=0 ;;
        *)      expected_status=1 ;;
    esac

    if [ "$actual" == "$expected" ] && [ "$status" -eq "$expected_status" ]; then
        passed=$((passed + 1))
    else
        failed=$((failed + 1))
        echo "FAIL $name (exit $status, expected $expected_status)"
        diff <(echo "$expected") <(echo "$actual") | sed 's/^/    /'
    fi
done

echo "$passed passed, $failed failed"
[ "$failed" -eq 0 ]
