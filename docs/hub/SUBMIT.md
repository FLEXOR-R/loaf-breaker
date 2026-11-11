# LOAF BREAKER on the FM-1 Firmware Hub

LOAF BREAKER is FLEXOR's fork of [CHOPPAH](https://github.com/profikid/fm1-break-choppah) (Laurens Profittlich). It
has its own name, files and package identity, and credits CHOPPAH everywhere it is shown (README, site, card text).
The hub's rules: <https://thefirmwarehub.com/fmhub.md> and <https://thefirmwarehub.com/kb/>. It takes a firmware by
**mail** (`zven@thefirmwarehub.com`), once the six points below hold.

## Identity

- **Package identity:** `FM-1_2623` (fm1.toml `id_prefix`): dev builds `FM-1_262300`, releases `FM-1_2623` + the
  package number as two digits each (0.220 is package 1.0.21: `FM-1_2623010021`; 1.00 will be 2.0.0:
  `FM-1_2623020000`, always above the last: `scripts/loaf_version.sh`). Checked free on the hub's catalogue on
  9 Nov 2026: no identity there starts with 2623, and 2623 starts none of theirs. CHOPPAH keeps its own, `FM-1_86`.
  Nothing is reserved on the hub: an identity is chosen, used in the releases, and given in the mail.
- **Files:** `loaf-breaker-X.fwsc` (fm1.toml `name`), the site's installer and the releases.
- **USB:** a MIDI device named "LOAF BREAKER FM-1", maker "LOAF BREAKER".
- **Card:** `fmhub.json` (root): LOAF BREAKER's text, FLEXOR as author, the site's emulator and manual, the picture
  `docs/img/loaf_breaker_card.png` (480 × 480 PNG), the screen's red.

## The six points

| # | The hub asks | LOAF BREAKER |
| --- | --- | --- |
| 1 | Open source, the whole source public, a LICENSE | MPL-2.0, LICENSE in the root. **The repository is private**: to be made public (FLEXOR's call). Its history is clean: the GLOUPI BREAKS photo is in no branch and no tag since 10 Nov 2026 (branches rewritten, then the 219 tags with it by the PURGE START-UP PHOTO action, since removed; GitHub's cached views of the old commits: a Support request) |
| 2 | A GitHub release with the .fwsc | Every push to `loaf`: release `loaf-vX.YYY` with `loaf-breaker-X.YYY.fwsc` |
| 3 | Its own `FM-1_` identity | `FM-1_2623` (above) |
| 4 | The update loader in the package | felucca-bsp's, shipped in every package |
| 5 | Tested on a real FM-1: from V15, from another firmware, and another installed from it | **To do** before the mail (before leaving LOAF BREAKER: SYSTEM › RESET › WIPE MEMORY, 0.263, its saves erased so the next firmware finds that memory blank) |
| 6 | `fmhub.json` in the root of the default branch | Done (`loaf`, the default branch) |

The site has no install code any more (0.261, public): the firmware installs from it, from the releases and from the hub.

## The mail, when 1 and 5 are done

> Hello, here is LOAF BREAKER, a firmware for the M-VAVE FM-1, by FLEXOR. It is a fork of CHOPPAH by Laurens
> Profittlich, credited as such.
> Repository: https://github.com/FLEXOR-R/loaf-breaker
> Identity: FM-1_2623 (releases FM-1_2623XXYYZZ) · Licence: MPL-2.0
> Tested on a real FM-1: installed from V15 and from another firmware, and another firmware installs again from it.
> It keeps its own settings and patterns and does not read other firmwares' projects or samples.
