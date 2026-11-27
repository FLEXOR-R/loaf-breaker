#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
"""CHOPPAH's look for the site fm1-base's tools/make_site.py builds (run it after make_site.py):

  tools/site_style.py SITE_DIR

- install/index.html (fm1-base's installer): site.css and app/site/install.css in place of its style, and the
  site's header above it; its markup and script stay as they are;
- emu/index.html (fm1-base's emulator): site.css and app/site/emu.css in place of its style (its Barlow stays for the
  panel's printed labels), the site's
  header and an introduction in place of its title, and a "where to start" row under the panel; its panel, markup
  and script stay as they are;
- index.html and manual/index.html: app/site/site.css (with its fonts) in place of fm1-base's style, a class on
  <body> for each page, and a "Controls" link in the site's header (to the manual's control map);
- manual/index.html: the control map (app/site/controlmap.html: explorer, buttons, the whole map, who owns the
  knobs) after the heading of the section "The control map", in place of that section's text (the Markdown's text
  is for readers on GitHub); and the contents follow the reading (the section on screen marked).

The pages keep everything else make_site.py gave them: their text, links, images, tables and the contents."""
import re
import shutil
import sys
from pathlib import Path

APP = Path(__file__).resolve().parents[1] / "app"
FONTS = ('<link rel="preconnect" href="https://fonts.googleapis.com">\n'
         '<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>\n'
         '<link rel="stylesheet" href="https://fonts.googleapis.com/css2?family=Barlow+Condensed:wght@500;600;700;800'
         '&family=Barlow:wght@400;500;600&family=Pirata+One&family=JetBrains+Mono:wght@400;500&display=swap">\n')

# Look A (27 Nov 2026, FLEXOR: "une identité visuelle un petit peu plus marquée ... des onglets un peu plus digestes"):
# the site in five entries, each its pages (a row of tabs under the header): (entry, its first page, its pages)
SECTIONS = [
    ("VST", "vst/", [("vst/", "The plugin")]),
    ("Firmware", "install/", [("install/", "Install · Update"), ("emu/", "Try it"), ("backup/", "Backup")]),
    ("Sounds", "breaks/", [("breaks/", "Breaks"), ("import/", "Drum samples")]),
    ("Tools", "tools/", [("tools/", "All tools"), ("editor/", "Studio"), ("live/", "Live"), ("mirror/", "Mirror"),
                         ("app/", "App")]),
    ("Manual", "manual/", [("manual/#start-in-five-minutes", "Start in 5 minutes"), ("manual/", "The manual"),
                           ("manual/#controls", "Control map")]),
]
# the header's own style: on every page, whatever the page's own (fm1-base's backup and mirror, the Studio)
TOP_CSS = """<style>
.lbtop { position: sticky; top: 0; z-index: 50; margin: 0 calc(-1 * max(16px, env(safe-area-inset-left, 0px))) 30px;
  padding: 0 max(16px, env(safe-area-inset-left, 0px)); background: rgba(8, 8, 8, .94); backdrop-filter: blur(6px);
  border-bottom: 1px solid #2a2926; font-family: "Barlow Condensed", "Arial Narrow", sans-serif; color: #ebe5d8; }
.lbtop .lbin { max-width: 1160px; margin: 0 auto; }
.lbtop .lbbar { display: flex; align-items: center; gap: 10px 18px; padding: 10px 0; flex-wrap: wrap; }
.lbtop a { color: inherit; text-decoration: none; }
.lbtop .lbhome { display: flex; align-items: center; gap: 10px; margin-right: auto; }
.lbtop .lbhome img { width: 36px; height: 36px; border-radius: 2px; display: block; }
.lbtop .lbhome span { font: 400 26px/1 "Pirata One", "Times New Roman", serif; letter-spacing: .01em; }
.lbtop .lbmenu { display: flex; gap: 2px; flex-wrap: wrap; }
.lbtop .lbmenu a { font: 600 15px/1 "Barlow Condensed", sans-serif; letter-spacing: .14em; text-transform: uppercase;
  color: #8f897e; padding: 10px 12px; border-radius: 2px; }
.lbtop .lbmenu a:hover { color: #ebe5d8; background: #1a1917; }
.lbtop .lbmenu a[aria-current] { color: #ebe5d8; box-shadow: inset 0 -2px 0 #ebe5d8; }
.lbtop .lbinst { font: 700 15px/1 "Barlow Condensed", sans-serif; letter-spacing: .13em; text-transform: uppercase;
  background: #ebe5d8; color: #000; padding: 11px 15px; border-radius: 2px; }
.lbtop .lbinst:hover { background: #fff; }
.lbtop .lbsub { display: flex; gap: 4px; flex-wrap: wrap; padding: 0 0 10px; }
.lbtop .lbsub a { font: 600 13px/1 "Barlow Condensed", sans-serif; letter-spacing: .12em; text-transform: uppercase;
  padding: 7px 10px; border: 1px solid #2a2926; border-radius: 2px; color: #8f897e; }
.lbtop .lbsub a:hover { color: #ebe5d8; border-color: #5c5852; }
.lbtop .lbsub a[aria-current] { background: #ebe5d8; color: #000; border-color: #ebe5d8; }
.lbtop a:focus-visible { outline: 2px solid #ebe5d8; outline-offset: 2px; }
@media (max-width: 760px) {
  .lbtop { position: static; }
  .lbtop .lbmenu { order: 3; width: 100%; }
  .lbtop .lbmenu a { padding: 9px 8px; font-size: 14px; letter-spacing: .1em; }
  .lbtop .lbhome span { font-size: 22px; }
}
</style>
"""
TOP_FONTS = ('<link rel="stylesheet" href="https://fonts.googleapis.com/css2?family=Barlow+Condensed:wght@600;700'
             '&family=Pirata+One&display=swap">\n')


def top(page, up="../", fonts=True):
    """the site's header (look A): the LOAF LORD and the name, the five entries, INSTALL, and the entry's pages as tabs;
    page: one of SECTIONS' pages ("install/", "manual/#controls" ...) or "" (the front page); up: the way to the root"""
    entry = next((e for e in SECTIONS if any(h == page for h, _ in e[2])), None)
    menu = "".join(f'<a href="{up}{first}"' + (' aria-current="page"' if e is entry else "") + f">{name}</a>"
                   for e in SECTIONS for name, first, _ in [e])
    sub = ""
    if entry and len(entry[2]) > 1:
        sub = (f'<nav class="lbsub" aria-label="{entry[0]}">' +
               "".join(f'<a href="{up}{h}"' + (' aria-current="page"' if h == page else "") + f">{t}</a>"
                       for h, t in entry[2]) + "</nav>")
    return ((TOP_FONTS if fonts else "") + TOP_CSS + '<div class="lbtop"><div class="lbin"><div class="lbbar">'
            f'<a class="lbhome" href="{up or "./"}"><img src="{up}img/favicon.png" alt=""><span>Loaf Breaker</span></a>'
            f'<nav class="lbmenu" aria-label="Site">{menu}</nav><a class="lbinst" href="{up}install/">Install</a></div>'
            f"{sub}</div></div>\n")


SPY = """<script>
(function () {                         /* the contents follow the reading: the section on screen marked */
  var links = {}, nav = document.querySelector("nav.contents");
  if (!nav || !("IntersectionObserver" in window)) return;
  nav.querySelectorAll("a[href^='#']").forEach(function (a) { links[a.getAttribute("href").slice(1)] = a; });
  var seen = new Set();
  var io = new IntersectionObserver(function (es) {
    es.forEach(function (e) { if (e.isIntersecting) seen.add(e.target.id); else seen.delete(e.target.id); });
    var first = Object.keys(links).find(function (id) { return seen.has(id); });
    if (!first) return;
    Object.keys(links).forEach(function (id) {
      if (id === first) links[id].setAttribute("aria-current", "true"); else links[id].removeAttribute("aria-current");
    });
  }, { rootMargin: "-80px 0px -55% 0px" });
  Object.keys(links).forEach(function (id) { var h = document.getElementById(id); if (h) io.observe(h); });
})();
</script>
"""


def restyle(path, cls, controls_href, extra=""):
    s = path.read_text(encoding="utf-8")
    css = (APP / "site" / "site.css").read_text(encoding="utf-8") + extra
    s, n = re.subn(r"<style>.*?</style>", lambda m: FONTS + "<style>" + css + "</style>", s, count=1, flags=re.S)
    if n != 1:
        raise SystemExit(f"{path}: no <style> to replace")
    s = s.replace("<body>", f'<body class="{cls}">', 1)
    return s


PAGE_OF = {"VST": "vst/", "Emulator": "emu/", "Install": "install/", "Manual": "manual/", "Breaks": "breaks/",
           "Import": "import/", "Studio": "editor/", "Live": "live/", "App": "app/", "Tools": "tools/",
           "Backup": "backup/", "Mirror": "mirror/"}


def header(current):
    """the header of a page one level down (its name as before: "Install", "Studio" ...)"""
    return top(PAGE_OF.get(current, ""))


EMU_START = """<div class="start-here" aria-label="Where to start">
  <div><b>Press PLAY</b><p>A breakbeat starts, with a kick under it. The screen scrolls the sound by.</p></div>
  <div><b>Tap the white keys</b><p>Each one plays a piece of the break. Tap a few to rearrange the groove; hold one and it repeats.</p></div>
  <div><b>Hold a black key</b><p>An effect while you hold it: a roll, a filter, a stop like a record. The big knob changes the break.</p></div>
</div>
"""

FOOT = """<footer class="lbfoot"><div class="lbin"><span>LOAF BREAKER by FLEXOR · built on CHOPPAH</span><nav>
<a href="{up}manual/">Manual</a><a href="{up}install/">Install</a><a href="{up}vst/">VST</a>
<a href="https://github.com/FLEXOR-R/loaf-breaker">Source</a></nav></div></footer>
<style>
.lbfoot { border-top: 1px solid #2a2926; margin-top: 70px; padding: 26px 0 10px; color: #5c5852; font: 400 14px/1.5 "Barlow", sans-serif; }
.lbfoot .lbin { max-width: 1160px; margin: 0 auto; display: flex; gap: 16px; flex-wrap: wrap; justify-content: space-between; }
.lbfoot nav { display: flex; gap: 16px; flex-wrap: wrap; }
.lbfoot a { color: #8f897e; text-decoration: none; }
.lbfoot a:hover { color: #ebe5d8; }
</style>
"""


def foot(s, up="../"):
    return s.replace("</body>", FOOT.replace("{up}", up) + "</body>", 1)


def emu(site):
    """the emulator ("Try it"): the site's style and fonts, its header, a sentence, three gestures under the panel"""
    page = site / "emu" / "index.html"
    if not page.exists():
        return
    s = restyle(page, "emu", "", (APP / "site" / "emu.css").read_text(encoding="utf-8"))
    m = re.search(r"<header>\s*<h1>([^<]*)</h1>\s*</header>\s*<main>", s)
    if not m:
        raise SystemExit(f"{page}: no <header><h1> then <main> to replace")
    s = (s[:m.start()] + header("Emulator") + '<main>\n<div class="intro-lb"><div class="eyebrow">Try it</div>'
         "<h1>Play it in your browser</h1><p>The real firmware, running in this page, with sound. <b>No FM-1 needed</b>: "
         "click the keys and the knobs, touch them, or use your computer's keyboard.</p></div>" + s[m.end():])
    s = s.replace('<details class="speak" id="speak">', EMU_START + '<details class="speak" id="speak">', 1)
    page.write_text(foot(s), encoding="utf-8")


INSTALL_INTRO = """<div class="intro-lb"><div class="eyebrow">Install · Update</div><h1>Put LOAF BREAKER on your FM-1</h1>
  <p>About a minute, from this page. <b>You need</b> the FM-1, its USB cable, and Chrome or Edge on a computer. Already on
  LOAF BREAKER? The same button updates it. Your FM-1 can go back to its original firmware at any time, from here too.</p></div>
  <div class="steps-lb"><div><b><em>1</em>Plug in</b>The FM-1, by USB, to this computer.</div>
  <div><b><em>2</em>Install</b>Press Install below; the browser asks once to use MIDI devices: allow it.</div>
  <div><b><em>3</em>Wait</b>For the bar to fill. Don't unplug the cable while it writes.</div></div>"""


def install(site):
    """the installer: the site's style, its header above its main, a sentence and three steps in place of its title"""
    page = site / "install" / "index.html"
    if not page.exists():
        return
    s = restyle(page, "install", "", (APP / "site" / "install.css").read_text(encoding="utf-8"))
    s = s.replace('<body class="install">', '<body class="install">\n' + header("Install"), 1)
    s, n = re.subn(r"<h1>[^<]*</h1>", INSTALL_INTRO, s, count=1)
    if n != 1:
        raise SystemExit(f"{page}: no <h1>")
    page.write_text(foot(s), encoding="utf-8")


FM1_CSS = """<style>
:root:root:root { --ground: #080808; --ink: #ebe5d8; --muted: #8f897e; --rule: #2a2926; --accent: #ebe5d8; --teal: #ebe5d8;
  --card: #111110; --accent-ink: #ebe5d8; --on-accent: #000; color-scheme: dark; }
body { padding-top: 0 !important; }
main h1 { font-family: "Barlow Condensed", "Arial Narrow", sans-serif !important; font-weight: 700 !important;
  text-transform: uppercase; letter-spacing: .02em; font-size: clamp(34px, 5vw, 54px) !important; line-height: 1.02 !important; }
main h2 { font-family: "Barlow Condensed", "Arial Narrow", sans-serif !important; text-transform: uppercase; letter-spacing: .04em; }
button, label.pick { border-radius: 2px !important; font-family: "Barlow Condensed", sans-serif !important; letter-spacing: .1em;
  text-transform: uppercase; }
.card { border-radius: 3px !important; }
</style>
"""


def fm1_page(site, name, current, h1, lede):
    """fm1-base's own pages (backup/, mirror/): their markup and script as they are, the site's header and colours, a
    plain title and sentence"""
    page = site / name / "index.html"
    if not page.exists():
        return
    s = page.read_text(encoding="utf-8")
    s = s.replace("</head>", FM1_CSS + "</head>", 1)
    s = re.sub(r"<body([^>]*)>", lambda m: "<body" + m.group(1) + ">\n" + header(current), s, count=1)
    s = re.sub(r"<h1>[^<]*</h1>", "<h1>" + h1 + "</h1>", s, count=1)
    if lede:
        s = re.sub(r'(<p class="lede">|<header>\s*<h1>[^<]*</h1>\s*<p>)(.*?)(</p>)',
                   lambda m: m.group(1) + lede + m.group(3), s, count=1, flags=re.S)
    page.write_text(foot(s), encoding="utf-8")


ATELIER_HTML = """<h1>Atelier</h1>
  <p class="atlead">Installe n'importe quel LOAF BREAKER : la TEST du moment, une ancienne version, un firmware fait sur une
  branche. Page secrète : dans aucun menu, ni dans les moteurs de recherche.</p>
  <div id="atdrop" class="atdrop">
    <p><strong>Glisse ici un .fwsc</strong> (ou le .zip d'une build GitHub)</p>
    <p><label for="atfile" class="sr">Fichier du firmware</label><input type="file" id="atfile" accept=".fwsc,.zip"></p>
    <p id="atstate" role="status"></p>
  </div>
  <ul class="small atwhere">
    <li><strong>TEST du moment</strong> : <a data-at="/releases" target="_blank" rel="noopener">Releases</a>, le brouillon
      « LOAF BREAKER TEST » (connecté à GitHub : toi seul le vois), fichier loaf-breaker-A.B.C-testN.fwsc.</li>
    <li><strong>Une branche à toi</strong> : pousse-la en <code>perso/un-nom</code> (ou Actions › LOAF BREAKER › Run
      workflow sur ta branche) : un brouillon « LOAF BREAKER PERSO un-nom » arrive dans
      <a data-at="/releases" target="_blank" rel="noopener">Releases</a>.</li>
    <li><strong>Une ancienne version</strong> : <a data-at="/releases" target="_blank" rel="noopener">Releases</a>,
      loaf-breaker-A.B.C.fwsc.</li>
  </ul>"""

ATELIER_CSS = """<style>
.atdrop { border: 2px dashed currentColor; border-radius: 10px; padding: 14px 16px; margin: 18px 0; opacity: .95; }
.atdrop.over { border-style: solid; background: rgba(255, 255, 255, .06); }
.atdrop p { margin: 6px 0; }
#atstate { font-weight: 600; min-height: 1.4em; }
#atstate.bad { color: #ff6b6b; }
.atwhere li { margin: 4px 0; }
</style>
"""


def atelier(site):
    """LOAF BREAKER (0.9.75, FLEXOR: "un onglet secret ... pour installer les firmware de test ou les firmwares perso du
    git"): atelier/, the installer (install/, already restyled) with any LOAF BREAKER package dropped on it in place of
    the site's own (app/site/atelier.js inside its module); in no menu, noindex"""
    src = site / "install" / "index.html"
    if not src.exists():
        return
    s = src.read_text(encoding="utf-8")
    js = (APP / "site" / "atelier.js").read_text(encoding="utf-8")
    load = "    const r = await fetch(meta.pkg);\n"
    click = '$("go").addEventListener("click", () => run(image, meta.product, false));'
    for mark in (load, click, "</head>", "<title>"):
        if s.count(mark) != 1:
            raise SystemExit(f"install/index.html: {mark.strip()!r} not found once (atelier)")
    s, n = re.subn(r"<h1>.*?</h1>", ATELIER_HTML, s, count=1, flags=re.S)
    if n != 1:
        raise SystemExit("install/index.html: no <h1> (atelier)")
    s = s.replace(load, '    say("Choisis un firmware ci-dessus."); return;   /* (the atelier: no package of its own) */\n' + load, 1)
    s = s.replace(click, js.replace("</script", "<\\/script") + "\n" + click, 1)
    s = re.sub(r"<title>[^<]*</title>", "<title>Atelier · LOAF BREAKER</title>", s, count=1)
    s = s.replace("</head>", '<meta name="robots" content="noindex, nofollow">\n' + ATELIER_CSS + "</head>", 1)
    (site / "atelier").mkdir(parents=True, exist_ok=True)
    (site / "atelier" / "index.html").write_text(s, encoding="utf-8")


def site_name():
    conf = (APP / "fm1.toml").read_text(encoding="utf-8")
    m = re.search(r'^title\s*=\s*"([^"]+)"', conf, re.M)
    n = re.search(r'^name\s*=\s*"([^"]+)"', conf, re.M)
    return (m.group(1) if m else None), (n.group(1) if n else None)


def rebrand(site):
    """LOAF BREAKER: a fork's site in its own name (fm1.toml title, the build keeping CHOPPAH's): the pages'
    titles and words, the package's file (choppah-X.fwsc -> loaf-breaker-X.fwsc); the credits to CHOPPAH kept"""
    new, old = site_name()
    if not new or not old or new == old:
        return
    stem = new.lower().replace(" ", "-")
    renames = {}
    for f in (site / "firmware").glob(old.lower() + "-*"):
        to = f.with_name(stem + f.name[len(old):])
        renames[f.name] = to.name
        f.rename(to)
    keep = re.compile(r'(?<=fm1-break-choppah">)' + re.escape(old) + "|" + re.escape(old) +
                      r"(?=</strong>, Laurens| by Laurens|'s author)")   # (the credits: the link, the sentences)
    for f in site.rglob("*"):
        if f.suffix not in (".html", ".js", ".json", ".webmanifest") or not f.is_file():
            continue
        t = f.read_text(encoding="utf-8")
        u = t
        for a, b in renames.items():
            u = u.replace(a, b)
        u = keep.sub("\0KEEP\0", u)
        u = u.replace(old, new)                     # (upper case: the name; its URLs are lower case)
        u = u.replace("<title>" + new + ": " + new, "<title>" + new + ": the manual")
        u = u.replace("\0KEEP\0", old)
        if u != t:
            f.write_text(u, encoding="utf-8")


def logo(site):
    """LOAF BREAKER: every page's tab icon, LOAF LORD's face (app/site/img, tools/make_site_logo.py); ?v= the icon's
    size, so a browser that kept an older one takes the new"""
    ico = site / "img" / "favicon.png"
    if not ico.exists():
        return
    v = ico.stat().st_size
    for f in site.rglob("index.html"):
        up = "../" * (len(f.relative_to(site).parts) - 1)
        s = f.read_text(encoding="utf-8")
        s = s.replace("</head>", f'<link rel="icon" type="image/png" sizes="64x64" href="{up}img/favicon.png?v={v}">\n'
                                 f'<link rel="apple-touch-icon" href="{up}img/apple-touch-icon.png?v={v}">\n</head>', 1)
        f.write_text(s, encoding="utf-8")


def breaks(site):
    """the BREAKS page (app/site/breaks.html, CHOPPAH 1.4's break manager: the library to hear, your own breaks over
    Web MIDI): the site's style and header, at breaks/, and its library (tools/break_lib.py) at breaks/lib/"""
    page = (APP / "site" / "breaks.html").read_text(encoding="utf-8")
    css = (APP / "site" / "site.css").read_text(encoding="utf-8")
    if "/*SITE_CSS*/" not in page or "<!--HEADER-->" not in page:
        raise SystemExit("app/site/breaks.html: no /*SITE_CSS*/ or <!--HEADER--> to fill")
    page = page.replace("<style>/*SITE_CSS*/", FONTS + "<style>" + css, 1).replace("<!--HEADER-->", header("Breaks"), 1)
    page = foot(page)
    (site / "breaks").mkdir(parents=True, exist_ok=True)
    (site / "breaks" / "index.html").write_text(page, encoding="utf-8")
    import break_lib                               # (0.9.32, FLEXOR: the library the page plays and sends: lib/)
    break_lib.main(site / "breaks" / "lib")


def imp(site):
    """the IMPORT page (app/site/import.html, LOAF BREAKER 0.9.45, FLEXOR: your own snare, hat and clap on the FM-1 over
    Web MIDI): the site's style and header, at import/"""
    page = (APP / "site" / "import.html").read_text(encoding="utf-8")
    css = (APP / "site" / "site.css").read_text(encoding="utf-8")
    if "/*SITE_CSS*/" not in page or "<!--HEADER-->" not in page:
        raise SystemExit("app/site/import.html: no /*SITE_CSS*/ or <!--HEADER--> to fill")
    page = page.replace("<style>/*SITE_CSS*/", FONTS + "<style>" + css, 1).replace("<!--HEADER-->", header("Import"), 1)
    page = foot(page)
    (site / "import").mkdir(parents=True, exist_ok=True)
    (site / "import" / "index.html").write_text(page, encoding="utf-8")


def editor(site):
    """the Studio (app/site/editor.html and editor.js, LOAF BREAKER 0.9.43: the whole FM-1 from a computer over Web MIDI,
    as a DAW): the site's style, header and the script inlined, at editor/"""
    page = (APP / "site" / "editor.html").read_text(encoding="utf-8")
    css = (APP / "site" / "site.css").read_text(encoding="utf-8")
    js = (APP / "site" / "editor.js").read_text(encoding="utf-8")   # (0.9.43: the Studio's script, its own file)
    if "/*SITE_CSS*/" not in page or "<!--HEADER-->" not in page or "/*SCRIPT*/" not in page:
        raise SystemExit("app/site/editor.html: no /*SITE_CSS*/, <!--HEADER--> or /*SCRIPT*/ to fill")
    page = page.replace("<style>/*SITE_CSS*/", FONTS + "<style>" + css, 1).replace("<!--HEADER-->", header("Studio"), 1)
    page = page.replace("/*SCRIPT*/", js.replace("</script", "<\\/script"), 1)
    (site / "editor").mkdir(parents=True, exist_ok=True)
    (site / "editor" / "index.html").write_text(page, encoding="utf-8")
    for f in (APP / "site" / "studio_img").glob("*.png"):   # (0.9.87: the VST's look, the LOAF LORD in the header)
        shutil.copyfile(f, site / "editor" / f.name)


def page_of(site, name, current):
    """LOAF BREAKER (0.9.71): a page of app/site/NAME.html at NAME/ (app.html: app/), the site's style and header"""
    page = (APP / "site" / f"{name}.html").read_text(encoding="utf-8")
    css = (APP / "site" / "site.css").read_text(encoding="utf-8")
    if "/*SITE_CSS*/" not in page or "<!--HEADER-->" not in page:
        raise SystemExit(f"app/site/{name}.html: no /*SITE_CSS*/ or <!--HEADER--> to fill")
    page = page.replace("<style>/*SITE_CSS*/", FONTS + "<style>" + css, 1).replace("<!--HEADER-->", header(current), 1)
    if "<!--FOOT-->" in page:
        page = page.replace("<!--FOOT-->", FOOT.replace("{up}", "../"), 1)
    elif name == "vst":
        page = foot(page)
    (site / name).mkdir(parents=True, exist_ok=True)
    (site / name / "index.html").write_text(page, encoding="utf-8")


def home(site):
    """the front page (look A, 27 Nov 2026): app/site/home.html in place of make_site.py's, the release's version read
    from it"""
    page = site / "index.html"
    old = page.read_text(encoding="utf-8") if page.exists() else ""
    v = re.search(r"Version ([0-9][^ <]*)", old)
    s = (APP / "site" / "home.html").read_text(encoding="utf-8")
    css = (APP / "site" / "site.css").read_text(encoding="utf-8")
    s = s.replace("<style>/*SITE_CSS*/", FONTS + "<style>" + css, 1).replace("<!--HEADER-->", top("", ""), 1)
    s = s.replace("<!--FOOT-->", FOOT.replace("{up}", ""), 1).replace("{{VERSION}}", v.group(1) if v else "dev")
    page.write_text(s, encoding="utf-8")
    for f in ("lb_title.png", "lord_face.png", "home_seq.png"):   # (home_seq: tests/scenarios/homeseq.txt, GO HARD)
        src = APP / "site" / "img" / f
        if src.exists():
            (site / "img").mkdir(parents=True, exist_ok=True)
            shutil.copyfile(src, site / "img" / f)


def pwa(site):
    """LOAF BREAKER (0.9.71, FLEXOR: an Android app): the site installable as an app: its manifest and service worker at
    the root (their scope the whole site), every page linking the manifest, the theme's colour, the worker registered"""
    for f in ("manifest.webmanifest", "sw.js"):
        (site / f).write_text((APP / "site" / f).read_text(encoding="utf-8"), encoding="utf-8")
    for f in site.rglob("index.html"):
        up = "../" * (len(f.relative_to(site).parts) - 1)
        s = f.read_text(encoding="utf-8")
        if 'rel="manifest"' in s:
            continue
        reg = "" if "serviceWorker" in s else (f'<script>if ("serviceWorker" in navigator) navigator.serviceWorker'
                                               f'.register("{up}sw.js").catch(function () {{}});</script>\n')
        s = s.replace("</head>", f'<link rel="manifest" href="{up}manifest.webmanifest">\n'
                                 f'<meta name="theme-color" content="#0e0d0b">\n'
                                 f'<meta name="mobile-web-app-capable" content="yes">\n' + reg + "</head>", 1)
        f.write_text(s, encoding="utf-8")


def main(site):
    site = Path(site)
    rebrand(site)
    install(site)
    atelier(site)
    emu(site)
    breaks(site)
    imp(site)
    editor(site)
    page_of(site, "live", "Live")        # (0.9.71: the FM-1 in the hand, the app's)
    page_of(site, "app", "App")
    page_of(site, "vst", "VST")          # (24 Nov 2026, FLEXOR: "un onglet VST sur le site": the plugin, its pictures)
    page_of(site, "tools", "Tools")      # (27 Nov 2026, look A: the four tools on one page)
    fm1_page(site, "backup", "Backup", "Keep your work safe",
             "Your kicks, patterns and tracks, saved as one file on your computer. Put them back after an update, or on "
             "another FM-1. Connect the FM-1 with a USB cable; this page works in Chrome or Edge, nothing to install.")
    fm1_page(site, "mirror", "Mirror", "The FM-1's screen, big",
             "Its screen and panel live on your computer, to film it, stream it or play it from here.")
    for f in (APP / "site" / "vst_img").glob("*.png"):
        (site / "vst" / f.name).write_bytes(f.read_bytes())
    home(site)
    man = site / "manual" / "index.html"
    if not man.exists():
        pwa(site)
        print(f"site_style: {site}: front page restyled (no manual)")
        return
    s = restyle(man, "manual", "#controls")
    s = s.replace('<body class="manual">', '<body class="manual">\n' + top("manual/"), 1)
    s = foot(s)
    h = re.search(r'<h2 id="(s\d+)"><span class="num">\d+</span>The control map</h2>', s)
    if not h:
        raise SystemExit(f"{man}: no section \"The control map\" (app/docs/MANUAL.md)")
    end = re.compile(r"<h2 |</main>").search(s, h.end())
    widget = (APP / "site" / "controlmap.html").read_text(encoding="utf-8")
    widget = re.sub(r"<!--.*?-->\s*", "", widget, count=1, flags=re.S)
    s = (s[:h.start()] + '<span id="controls"></span>' + s[h.start():h.end()] + "\n" + widget + "\n" + s[end.start():])
    s = s.replace("</body>", SPY + "</body>", 1)
    man.write_text(s, encoding="utf-8")
    logo(site)
    pwa(site)
    print(f"site_style: {site}: the front page and the manual restyled, the control map in section {h.group(1)}")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit(__doc__)
    main(sys.argv[1])
