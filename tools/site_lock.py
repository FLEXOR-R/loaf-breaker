#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
"""LOAF BREAKER: the site's firmware behind an install code (run after tools/site_style.py):

  LOAF_INSTALL_CODE=... tools/site_lock.py SITE_DIR

The package (SITE_DIR/firmware/loaf-breaker-X.fwsc) is encrypted with the code (PBKDF2-SHA256, 600,000 rounds, then
AES-256-GCM) into loaf-breaker-X.fwsc.lock, and the plain one is taken off the site: without the code nobody gets the
firmware, from the installer or by its URL. The installer page asks for the code in front of it and decrypts the
package in the browser (WebCrypto); fm1-base's installer itself is left as it is: its fetch of the package is answered
with the decrypted bytes once the code is right (its own checks, the identity and the image's, follow as ever).
With no code (LOAF_INSTALL_CODE empty or unset) the package is taken off and the page says installing is closed.
The emulator and the manual stay open. M-VAVE's own firmware can always be put back from the page."""
import hashlib
import os
import re
import secrets
import sys
from pathlib import Path

ITER = 600_000
MAGIC = b"LBK1"

GATE = r"""<style>
#lock{position:fixed;inset:0;z-index:50;display:flex;align-items:center;justify-content:center;padding:16px;
background:rgba(12,11,10,.94)}
#lock form,#lock .closed{max-width:420px;width:100%;background:#16140f;border:1px solid #3a352b;border-radius:10px;
padding:24px;color:#f1ead9;font:16px/1.5 system-ui,sans-serif}
#lock h2{margin:0 0 4px;font-size:22px;letter-spacing:.02em}
#lock p{margin:0 0 16px;color:#b9b09c;font-size:14px}
#lock input{width:100%;box-sizing:border-box;padding:12px;border-radius:6px;border:1px solid #4a4436;background:#0c0b0a;
color:#f1ead9;font-size:18px;letter-spacing:.12em}
#lock button{margin-top:12px;width:100%;padding:12px;border:0;border-radius:6px;background:#ff8470;color:#1a0907;
font-weight:700;font-size:16px;cursor:pointer}
#lock button:disabled{opacity:.6;cursor:wait}
#lock .err{color:#ff6b5a;min-height:1.5em;margin:8px 0 0;font-size:14px}
</style>
<div id="lock" role="dialog" aria-modal="true" aria-labelledby="lock-t">__BODY__</div>
<script>
(() => {
  const PKG = "__PKG__", ITER = __ITER__;
  let give, bytes = null;
  const ready = new Promise((ok) => { give = ok; });
  const realFetch = window.fetch.bind(window);
  window.fetch = (u, o) => {                       // the installer's fetch of the package: answered once unlocked
    const s = typeof u === "string" ? u : (u && u.url) || "";
    if (PKG && s.split("?")[0].endsWith("/" + PKG))
      return ready.then((b) => b ? new Response(b.slice(0), { status: 200 })
                                 : new Response("", { status: 403, statusText: "Locked" }));
    return realFetch(u, o);
  };
  async function open(code) {
    const r = await realFetch("../firmware/" + PKG + ".lock", { cache: "no-store" });
    if (!r.ok) throw new Error("HTTP " + r.status);
    const raw = new Uint8Array(await r.arrayBuffer());
    if (String.fromCharCode(...raw.slice(0, 4)) !== "LBK1") throw new Error("not a locked package");
    const base = await crypto.subtle.importKey("raw", new TextEncoder().encode(code), "PBKDF2", false, ["deriveKey"]);
    const key = await crypto.subtle.deriveKey({ name: "PBKDF2", salt: raw.slice(4, 20), iterations: ITER, hash: "SHA-256" },
                                              base, { name: "AES-GCM", length: 256 }, false, ["decrypt"]);
    return new Uint8Array(await crypto.subtle.decrypt({ name: "AES-GCM", iv: raw.slice(20, 32) }, key, raw.slice(32)));
  }
  window.addEventListener("DOMContentLoaded", () => {
    const f = document.getElementById("lock-f");
    if (!f) { give(null); return; }
    const inp = document.getElementById("lock-code"), err = document.getElementById("lock-err"),
          go = document.getElementById("lock-go");
    inp.focus();
    f.addEventListener("submit", async (e) => {
      e.preventDefault();
      const code = inp.value.trim();
      if (!code) return;
      go.disabled = true; err.textContent = "";
      try {
        bytes = await open(code);
        document.getElementById("lock").remove();
        give(bytes);
      } catch (x) {
        err.textContent = x && x.name === "OperationError" ? "Wrong code · Code incorrect" : "Could not load the firmware: " + x.message;
        inp.select();
      } finally { go.disabled = false; }
    });
  });
})();
</script>
"""

FORM = """<form id="lock-f" autocomplete="off">
<h2 id="lock-t">Install code</h2>
<p>LOAF BREAKER installs with the code FLEXOR gives you.<br>Entre le code que FLEXOR t'a donné.</p>
<input id="lock-code" type="password" inputmode="text" aria-label="Install code" placeholder="CODE">
<button id="lock-go" type="submit">Unlock · Déverrouiller</button>
<p id="lock-err" class="err" role="alert"></p>
</form>"""

CLOSED = """<div class="closed"><h2 id="lock-t">Installing is closed</h2>
<p>LOAF BREAKER is not open to install right now. The emulator and the manual are.<br>
L'installation est fermée pour le moment.</p><p><a href="../" style="color:#ff8470">Back</a></p></div>"""


def lock(site, code):
    site = Path(site)
    pkgs = [p for p in (site / "firmware").glob("loaf-breaker-*.fwsc")]
    name = pkgs[0].name if pkgs else ""
    for p in pkgs:
        if code:
            from cryptography.hazmat.primitives.ciphers.aead import AESGCM
            salt, iv = secrets.token_bytes(16), secrets.token_bytes(12)
            key = hashlib.pbkdf2_hmac("sha256", code.encode("utf-8"), salt, ITER, 32)
            (p.parent / (p.name + ".lock")).write_bytes(MAGIC + salt + iv + AESGCM(key).encrypt(iv, p.read_bytes(), None))
        p.unlink()                                   # the plain package: off the site either way
    page = site / "install" / "index.html"
    if page.exists():
        s = page.read_text(encoding="utf-8")
        gate = (GATE.replace("__BODY__", FORM if code and name else CLOSED)
                .replace("__PKG__", name if code else "").replace("__ITER__", str(ITER)))
        s, n = re.subn(r"(<body[^>]*>)", lambda m: m.group(1) + "\n" + gate, s, count=1)
        if not n:
            raise SystemExit(f"{page}: no <body>")
        page.write_text(s, encoding="utf-8")
    print(f"site_lock: {site}: " + (f"{name} locked behind the install code" if code and name else
                                    "no install code: installing closed, the package taken off"))


if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit(__doc__)
    lock(sys.argv[1], os.environ.get("LOAF_INSTALL_CODE", "").strip())
