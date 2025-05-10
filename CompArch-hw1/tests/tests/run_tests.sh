#!/bin/bash

# Assumes:
# - Your .trc files are named like input1.trc, input2.trc, ..., input150.trc
# - The expected output files are named like example1.out, example2.out, ...
# - bp_main is compiled and in the same directory

for i in {4..149}
do
    input_file="example${i}.trc"
    output_file="out${i}.out"
    expected_file="example${i}.out"

    echo "Running test $i..."
    ./bp_main "$input_file" > "$output_file"

    if diff -q "$output_file" "$expected_file" > /dev/null; then
        echo "Test $i: ✅ Passed"
    else
        echo "Test $i: ❌ Failed"
        diff "$output_file" "$expected_file"  # Optional: show diff
    fi
    echo "----------------------------------------"
done
