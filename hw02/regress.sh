#!/bin/sh
# regress.sh: the HW2 regression test. Runs ./readings on every input file in
# data/ and compares its output with the file of the same name in expected/.
# edge.txt is also run with a threshold of 25 (expected/edge_25.txt).
# Usage, in hw02/ after make:   sh regress.sh
# Exits with 1 if any output differs, so that make test fails.
fail=0
for input in data/*.txt; do
    name=$(basename "$input")
    if ./readings < "$input" | diff - "expected/$name" > /dev/null; then
        echo "PASS  regression $name"
    else
        echo "FAIL  regression $name"
        fail=1
    fi
done
if ./readings 25 < data/edge.txt | diff - expected/edge_25.txt > /dev/null; then
    echo "PASS  regression edge.txt, threshold 25"
else
    echo "FAIL  regression edge.txt, threshold 25"
    fail=1
fi
exit $fail
