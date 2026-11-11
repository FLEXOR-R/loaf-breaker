#!/usr/bin/env bash
# SPDX-License-Identifier: MPL-2.0
# LOAF BREAKER's release numbers (.github/workflows/loaf.yml). Two numbers for each release:
#   V    what people see: the tag loaf-vX.YY, the splash, the files (0.220, then 1.00, 1.01 ...)
#   PKG  the package's own, for fm1-base's build: X.Y[.Z], two digits each (the FM-1's identity FM-1_XXYYZZ); the
#        installer only installs a higher one over a lower one, so PKG must always go up with V:
#          0.02 .. 0.99      as they are
#          0.100 .. 0.198    0.99.1 .. 0.99.99
#          0.199 ..          1.0.0, 1.0.1 ... (0.220 is 1.0.21; room up to 0.10198: 1.99.99)
#          1.00 ..           2.0.0, 2.0.1 ... (X.YY is X+1 . YY/100 . YY%100: above every 0.x)
# ./LOAF_MAJOR is the floor of X: 0 now; set it to 1 (and push) for the release to be 1.00, then 1.01 ...
#   scripts/loaf_version.sh next [TAG ...]   the next V, from these tags' numbers (none given: origin's tags)
#   scripts/loaf_version.sh pkg V            V's PKG
#   scripts/loaf_version.sh check            every V from 0.02 to 2.300 in turn: its PKG higher than the last
set -e
cd "$(dirname "$0")/.."

pkg() {                                  # V -> PKG
    maj="${1%%.*}"; min="${1#*.}"
    case "$maj.$min" in *[!0-9.]*|.*|*.) echo "loaf_version: '$1' is not X.YY" >&2; exit 1 ;; esac
    m=$((10#$min))
    if [ "$maj" -ge 1 ]; then
        echo "$((maj + 1)).$((m / 100)).$((m % 100))"
    elif [ "${#min}" -le 2 ]; then
        echo "$1"
    elif [ "$m" -ge 199 ]; then
        echo "1.$(((m - 199) / 100)).$(((m - 199) % 100))"
    else
        echo "0.99.$((m - 99))"
    fi
}

key() {                                  # PKG -> a number that sorts as the installer does (X, Y, Z)
    x="${1%%.*}"; r="${1#*.}"; y="${r%%.*}"; z=0
    [ "$r" != "$y" ] && z="${r#*.}"
    echo $((x * 10000 + 10#$y * 100 + 10#$z))
}

case "$1" in
pkg)
    pkg "$2" ;;
next)
    shift
    if [ $# -gt 0 ]; then
        NUMS="$(printf '%s\n' "$@")"
    else                                 # (loaf-v and gloupi-v: one count)
        NUMS="$(git ls-remote --tags origin 'refs/tags/loaf-v*' 'refs/tags/gloupi-v*' | sed 's|\^{}$||' \
                | sed -n 's|.*refs/tags/[a-z]*-v||p')"
    fi
    FLOOR="$(tr -d '[:space:]' < LOAF_MAJOR 2>/dev/null || echo 0)"
    LAST="$(printf '%s\n' "$NUMS" | grep -E '^[0-9]+\.[0-9]{2,}$' | sort -t. -k1,1n -k2,2n | tail -1)"
    if [ -z "$LAST" ]; then
        printf '%d.02\n' "$FLOOR"
    elif [ "${LAST%%.*}" -lt "$FLOOR" ]; then
        printf '%d.00\n' "$FLOOR"        # (a new major: X.00)
    else                                 # (no 1.00 after 0.99: on to 0.100, 0.101 ...)
        printf '%d.%02d\n' "${LAST%%.*}" $((10#${LAST#*.} + 1))
    fi ;;
check)
    last=-1; n=0
    for v in $(m=2; while [ $m -le 300 ]; do printf '0.%02d ' $m; m=$((m + 1)); done) \
             $(m=0; while [ $m -le 300 ]; do printf '1.%02d 2.%02d ' $m $m; m=$((m + 1)); done | tr ' ' '\n' | sort -t. -k1,1n -k2,2n); do
        p="$(pkg "$v")"; k="$(key "$p")"
        for part in $(echo "$p" | tr '.' ' '); do
            [ "$part" -le 99 ] || { echo "loaf_version: $v -> $p, a part over 99" >&2; exit 1; }
        done
        [ "$k" -gt "$last" ] || { echo "loaf_version: $v -> $p is not above the one before" >&2; exit 1; }
        last="$k"; n=$((n + 1))
    done
    echo "loaf_version: $n versions, each package number above the last ($(pkg 0.220) for 0.220, $(pkg 1.00) for 1.00)" ;;
*)
    sed -n '3,15p' "$0" >&2; exit 1 ;;
esac
