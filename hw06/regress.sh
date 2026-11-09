#!/bin/sh
# hw06 regression test: replay every scenario in scenarios/ and compare
# its trace with the file of the same name in expected/, as in HW2.
# Usage, in hw06/ after make:   sh regress.sh
# Exits with 1 if any trace differs, so that `make test` fails.
fail=0
out=regress_out.tmp
rm -f "$out"
for scen in scenarios/*.txt; do
    name=$(basename "$scen")
    # Replay into a file first (not a pipe) so a crashing ./replay is
    # caught by its exit status instead of hiding behind diff's.
    if ! ./replay "$scen" > "$out"; then
        echo "FAIL  regression $name (replay exited nonzero)"
        fail=1
    elif diff "expected/$name" "$out" > /dev/null; then
        echo "PASS  regression $name"
    else
        # Show the diff on failure: expected lines first, so lines
        # with "<" are missing from the real trace and lines with
        # ">" are extra.
        echo "FAIL  regression $name"
        diff "expected/$name" "$out"
        fail=1
    fi
done
rm -f "$out"
exit $fail
