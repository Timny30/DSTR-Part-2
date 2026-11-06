#!/usr/bin/env bash
# Runs all tests. Run from the project folder: bash scripts/test.sh
set -u
FAILED=0
FLAGS="-std=c++17 -g -Wall -Wextra -Iinclude -Itests"
CORE_SOURCES="src/ParcelSystem.cpp src/core/*.cpp src/modules/*.cpp"
mkdir -p build

echo "== Banned containers =="
if grep -rnE '#include *<(vector|list|stack|queue|priority_queue|deque|map|set|unordered_map|unordered_set)>' src include; then
    echo "FAILED: banned container found"
    FAILED=1
else
    echo "None found"
fi

echo "== Data structures =="
for test in tests/ds/*.cpp; do
    name=$(basename "$test" .cpp)
    if g++ $FLAGS "$test" -o "build/$name.exe"; then
        "./build/$name.exe" || FAILED=1
    else
        echo "FAILED to compile $test"
        FAILED=1
    fi
done

echo "== Integration =="
for test in tests/integration/*.cpp; do
    name=$(basename "$test" .cpp)
    if g++ $FLAGS "$test" $CORE_SOURCES -o "build/$name.exe"; then
        "./build/$name.exe" || FAILED=1
    else
        echo "FAILED to compile $test"
        FAILED=1
    fi
done

echo "== Scenarios =="
if g++ $FLAGS src/*.cpp src/core/*.cpp src/modules/*.cpp src/menus/*.cpp -o build/scenario_app.exe; then
    for input in tests/scenarios/*.in; do
        expected="${input%.in}.expected"
        name=$(basename "$input" .in)
        # Keep only messages and record rows, which is what each scenario checks.
        actual=$(./build/scenario_app.exe --no-demo --fixed-time "2026-01-01 09:00" < "$input" \
                 | grep -E '^(\[|(P[0-9]{4}|REQ[0-9]{4}|RES[0-9]{3}|ACT[0-9]{4}) )')
        if diff --strip-trailing-cr <(echo "$actual") "$expected"; then
            echo "$name: passed"
        else
            echo "$name: FAILED"
            FAILED=1
        fi
    done
else
    echo "FAILED to compile the program"
    FAILED=1
fi

if [ "$FAILED" -eq 0 ]; then
    echo "ALL TESTS PASSED"
else
    echo "SOME TESTS FAILED"
fi
exit "$FAILED"
