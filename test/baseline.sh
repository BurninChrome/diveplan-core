#!/bin/sh
# Regression baseline over the real dive logs in test/xml/.
#
# This is NOT a conformance check. It records what the model currently outputs,
# including the defects listed in test/vectors/expected-failures.txt, and fails
# when that output changes. Its only job is to make change visible.
#
#   ./baseline.sh check      compare current output against the stored baseline
#   ./baseline.sh record     regenerate the baseline (do this deliberately,
#                            in the same commit as an approved model change)
#
# Never regenerate to make a red build green. A diff here means either you
# changed the model on purpose, or you broke it.

set -eu

here=$(dirname "$0")
dive="$here/../src/dive"
parse="$here/parse_dive.py"
xml="$here/xml"
baseline="$here/baseline.sha256"
mode="${1:-check}"

[ -x "$dive" ] || { echo "no dive binary at $dive — run make first"; exit 2; }

emit() {
    for f in "$xml"/*.xml; do
        # SHA of the full 36-field output, so any drift in any compartment shows.
        sum=$(python3 "$parse" -f "$f" | "$dive" 2>/dev/null | sha256sum | cut -d' ' -f1)
        echo "$sum  $(basename "$f")"
    done
}

case "$mode" in
record)
    emit > "$baseline"
    echo "recorded $(wc -l < "$baseline") dive profiles to $(basename "$baseline")"
    echo "commit this together with the change that made it necessary"
    ;;
check)
    [ -f "$baseline" ] || { echo "no baseline; run '$0 record' first"; exit 2; }
    tmp=$(mktemp)
    trap 'rm -f "$tmp"' EXIT
    emit > "$tmp"
    if diff -u "$baseline" "$tmp" > /dev/null; then
        echo "baseline: $(wc -l < "$baseline") profiles unchanged"
    else
        echo "baseline: MODEL OUTPUT CHANGED"
        echo
        diff -u "$baseline" "$tmp" | sed -n '3,$p' | head -40
        echo
        echo "If this was intended, re-run '$0 record' and commit the result"
        echo "with the change that caused it. If not, you have a regression."
        exit 1
    fi
    ;;
*)
    echo "usage: $0 [check|record]" >&2
    exit 2
    ;;
esac
