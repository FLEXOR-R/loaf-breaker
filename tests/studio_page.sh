#!/bin/sh
# SPDX-License-Identifier: MPL-2.0
# The Studio (LOAF BREAKER 0.9.87, app/site/editor.html, the VST's look) end to end, in Chromium at a computer's size,
# its Web MIDI wired to the host simulator as an FM-1 over USB (tests/studio_page.mjs): the VST's word, connect, the
# categories and tabs, a value turned, the pads, PLAY; a screenshot of each category. The site's pages built first
# (tools/site_style.py on a bare site). Needs ./build.sh --host and Playwright with its Chromium; without Playwright it
# says so and passes.
#   tests/studio_page.sh
set -e
cd "$(dirname "$0")/.."
if ! node -e "require('playwright')" 2>/dev/null; then
    echo "studio_page: no Playwright here, skipped"
    exit 0
fi
O=app/build/studio
S="$O/site"
mkdir -p "$S"
cp -r app/site/img "$S/"
python3 tools/site_style.py "$S" > /dev/null
timeout -k 5 200 node tests/studio_page.mjs "$S" "$O"
