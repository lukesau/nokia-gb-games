#!/bin/sh
# Runs a script through the JAR's own classes in the harness (README.md
# here): usage: run.sh SCRIPT, with JARDIR the directory the JAR is unzipped
# in, CLASSES where to compile to, and FONTMETRICS the metrics file
# tools/munkiki_fontmetrics.py writes.
set -e
H="$(cd "$(dirname "$0")" && pwd)"
JARDIR="${JARDIR:-$H/../../build/munkiki_jar}"
CLASSES="${CLASSES:-$H/../../build/munkiki_harness}"
FONTMETRICS="${FONTMETRICS:-$H/../../build/assets/fontmetrics.txt}"
SCRIPT="$1"
shift || true
if [ -z "$SCRIPT" ]; then
    echo "usage: $0 SCRIPT" >&2
    exit 2
fi
need=0
if [ ! -f "$CLASSES/Harness.class" ]; then
    need=1
elif [ -n "$(find "$H/src" -name '*.java' -newer "$CLASSES/Harness.class")" ]; then
    need=1
fi
if [ "$need" = 1 ]; then
    mkdir -p "$CLASSES"
    find "$H/src" -name '*.java' > "$CLASSES/.sources"
    javac --release 8 -Xlint:-options -cp "$JARDIR" -d "$CLASSES" @"$CLASSES/.sources"
fi
exec java -Dfontmetrics="$FONTMETRICS" -cp "$CLASSES:$JARDIR" "$@" Harness "$SCRIPT"
