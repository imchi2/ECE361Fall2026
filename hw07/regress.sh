#!/bin/sh
# hw07 regression test: run ./balance on every file in inputs/ and compare
# BOTH its output and its exit status with the files of the same name in
# expected/ (a .out file with the exact output, a .status file with the
# number). As in HW6.
# Usage, in hw07/ after make:   sh regress.sh
# Exits with 1 if any case differs, so that `make test` fails.
fail=0
out=regress_out.tmp
rm -f "$out"
for input in inputs/*; do
    # inputs/balanced.txt becomes just "balanced", so it can be matched
    # with expected/balanced.out and expected/balanced.status.
    name=$(basename "$input")
    name=${name%.txt}
    # Run into a file first (not a pipe) so a crashing ./balance is
    # caught by its exit status instead of hiding behind diff's.
    ./balance < "$input" > "$out"
    code=$?
    want=$(cat "expected/$name.status")
    if ! diff "expected/$name.out" "$out" > /dev/null; then
        # Show the diff on failure: expected lines first, so lines
        # with "<" are missing from the real output and lines with
        # ">" are extra.
        echo "FAIL  regression $name (output)"
        diff "expected/$name.out" "$out"
        fail=1
    elif [ "$code" -ne "$want" ]; then
        # The output matched but the program reported the wrong
        # success or failure.
        echo "FAIL  regression $name (exit status: want $want, got $code)"
        fail=1
    else
        echo "PASS  regression $name"
    fi
done
rm -f "$out"
exit $fail
