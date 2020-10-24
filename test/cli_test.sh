#!/bin/sh

fails=0

root="$(dirname "$(realpath "$0")")"

# Test failure when datadir can't be found:
env -i "$root"/../encnote8 --mode ls 2>/dev/null
if [ "$?" -ne 1 ]; then
	(>&2 echo "FAIL: No datadir.")
	fails=$(("$fails" + 1))
fi

# Ensure the data directory exists... (Because runners are weird.)
datadir="$("$root"/../encnote8 --datadir)"
mkdir -p "$datadir"

# Test creating new repo...
"$root"/../encnote8 --mode ls >/dev/null
if [ "$?" -ne 0 ]; then
	(>&2 echo "FAIL: Unable to initialise new repository.")
	fails=$(("$fails" + 1))
fi
if [ ! -r "${datadir}key" ]; then
	(>&2 echo "FAIL: Unable to initialise new repository.")
	fails=$(("$fails" + 1))
fi
if [ ! -r "${datadir}data" ]; then
	(>&2 echo "FAIL: Unable to initialise new repository.")
	fails=$(("$fails" + 1))
fi

# TODO: Test setting keyfile
# TODO: Test setting datafile
# TODO: Test help
# TODO: Test helpinfo
# TODO: Test copy
# TODO: Test ls
# TODO: Test view
# TODO: Test edit
# TODO: Test clone
# TODO: Test rename
# TODO: Test generate
# TODO: Test delete
# TODO: Test dump
# TODO: Test custom mode
# TODO: Test pre-hook
# TODO: Test post-hook
# TODO: Test preencrypt hook

exit "$fails"
