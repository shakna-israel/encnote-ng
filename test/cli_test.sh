#!/bin/sh

fails=0

root="$(dirname "$(realpath "$0")")"

# Test failure when datadir can't be found:
env -i "$root"/../encnote8 --mode ls 2>/dev/null
if [ "$?" -ne 1 ]; then
	(>&2 echo "FAIL: No datadir.")
	fails=$(($fails + 1))
fi

# Ensure the data directory exists... (Because runners are weird.)
datadir="$("$root"/../encnote8 --datadir)"
mkdir -p "$datadir"

# Test creating new repo...
"$root"/../encnote8 --mode ls >/dev/null
if [ "$?" -ne 0 ]; then
	(>&2 echo "FAIL: Unable to initialise new repository.")
	fails=$(($fails + 1))
fi
if [ ! -r "${datadir}key" ]; then
	(>&2 echo "FAIL: Unable to initialise new repository.")
	fails=$(($fails + 1))
fi
if [ ! -r "${datadir}data" ]; then
	(>&2 echo "FAIL: Unable to initialise new repository.")
	fails=$(($fails + 1))
fi

# Test setting keyfile
# Test setting datafile
key="$(mktemp)"
data="$(mktemp)"
"$root"/../encnote8 --mode ls --keyfile "$key" --datafile "$data" >/dev/null
if [ "$?" -ne 0 ]; then
	(>&2 echo "FAIL: Unable to initialise new custom repository.")
	fails=$(($fails + 1))
fi
if [ ! -r "$key" ]; then
	(>&2 echo "FAIL: Unable to initialise new custom repository.")
	fails=$(($fails + 1))
fi
if [ ! -r "$data" ]; then
	(>&2 echo "FAIL: Unable to initialise new custom repository.")
	fails=$(($fails + 1))
fi
if [ "$(wc -c "$key" | awk '{print $1}')" -lt 1 ]; then
	(>&2 echo "FAIL: Unable to initialise new custom repository.")
	fails=$(($fails + 1))
fi
if [ "$(wc -c "$data" | awk '{print $1}')" -lt 1 ]; then
	(>&2 echo "FAIL: Unable to initialise new custom repository.")
	fails=$(($fails + 1))
fi

# Test help
n="$("$root"/../encnote8 --help | wc -l | awk '{print $1}')"
if [ "$n" -lt 10 ]; then
	(>&2 echo "FAIL: Unable to get help")
	fails=$(($fails + 1))
fi

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

rm "$key"
rm "$data"

exit "$fails"
