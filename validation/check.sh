#!/usr/bin/env bash
# Script to run all standard radian checks
# expects $1 to contain path to radian executable
# expects to find test files in $2/*.radian

radian=$1
testdir=$2

tests=$(find $testdir -type f -name "*.radian")
error=0
for test in $tests; do
	if $radian $test; then
		echo -n "."
	else
		error=1
		echo ""
		echo "Failed: $test"
	fi
done
echo ""
if [ "$error" = "0" ]; then
	echo "Check suite passed."
fi
exit $error

