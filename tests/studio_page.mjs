// SPDX-License-Identifier: MPL-2.0
// The Studio (LOAF BREAKER 0.9.87, app/site/editor.html + editor.js, the VST's look) end to end: the page in Chromium at
// a computer's size, its Web MIDI wired to the host simulator run as an FM-1 (its scenario read from this script's pipe:
// "sysex HEX" in, "wait 30", and the frames it sent meanwhile from a "sysex_log" file a cycle, closed and so whole when
// the next one opens). Before connecting: the Studio and the VST side by side, the VST's word at the top. Connect finds
// it; the eight categories, each one's tabs; a card's value turned on the FM-1; the pads: the black keys' names; PLAY.
// Screenshots of each category. Run by tests/studio_page.sh (it skips when there is no Playwright).
//   node tests/studio_page.mjs SITE_DIR OUT_DIR
import { spawn, execFileSync } from "node:child_process";
import { createServer } from "node:http";
import { readFileSync, existsSync, statSync, mkdirSync, createWriteStream, rmSync } from "node:fs";
import { join, extname, resolve } from "node:path";
import { createRequire } from "node:module";
const require = createRequire(import.meta.url);
const { chromium } = require("playwright");
const [site, out] = process.argv.slice(2);
let fails = 0;
const check = (ok, what) => { console.log((ok ? "  ok " : "FAIL ") + what); if (!ok) fails++; };

// the simulator, as an FM-1 at the other end of the cable: its scenario, this pipe
const logs = resolve(out, "studio_sx");
rmSync(logs, { recursive: true, force: true });   // (the cycles' files of a run before)
mkdirSync(logs, { recursive: true });
const fifo = join(logs, "scenario.fifo");          // (a named pipe: Node's own stdin pipe is a socket, no file to open)
if (!existsSync(fifo)) execFileSync("mkfifo", [fifo]);
const host = spawn("app/build/host/loaf-breaker_host", [fifo, out], { stdio: ["ignore", "pipe", "inherit"] });
const toSim = createWriteStream(fifo);
let said = "";
host.stdout.on("data", (d) => { said += d; });
let cyc = 0;
const logOf = (n) => join(logs, `c${n}.bin`);
toSim.write(`wait 4200\nsysex_log ${logOf(0)}\n`);
let toHost = [];                                   // what the page sent, for the next cycle
const frames = (b) => { const f = []; let s = -1; for (let i = 0; i < b.length; i++) { if (b[i] === 0xF0) s = i; else if (b[i] === 0xF7 && s >= 0) { f.push(b.subarray(s, i + 1)); s = -1; } } return f; };
async function cycle(extra = "") {                 // the page's bytes in, 30 ms of the FM-1, what it sent back
  const send = toHost; toHost = [];
  const n = cyc++;
  toSim.write(send.map((h) => "sysex " + h + "\n").join("") + extra + `wait 30\nsysex_log off\nsysex_log ${logOf(n + 1)}\n`);
  while (!existsSync(logOf(n + 1))) await new Promise((r) => setTimeout(r, 3));
  return frames(readFileSync(logOf(n)));
}
// the site, served
const types = { ".html": "text/html", ".js": "text/javascript", ".png": "image/png", ".webmanifest": "application/manifest+json" };
const srv = createServer((q, r) => {
  let p = join(site, decodeURIComponent(q.url.split("?")[0]));
  if (existsSync(p) && statSync(p).isDirectory()) p = join(p, "index.html");
  if (!existsSync(p)) { r.writeHead(404); r.end(); return; }
  r.writeHead(200, { "content-type": types[extname(p)] || "application/octet-stream" }); r.end(readFileSync(p));
}).listen(0);
const port = srv.address().port;

const browser = await chromium.launch();
const page = await browser.newPage({ viewport: { width: 1440, height: 900 }, deviceScaleFactor: 1 });
page.on("console", (m) => { if (m.type() === "error" || m.type() === "warning") console.log("  console: " + m.text()); });
page.on("pageerror", (e) => console.log("  pageerror: " + e.message));
await page.exposeFunction("__toHost", (hex) => { toHost.push(hex); });
await page.addInitScript(() => {                   // Web MIDI: one FM-1, its bytes to and from the simulator
  const input = { id: "i", name: "LOAF BREAKER", state: "connected", onmidimessage: null };
  const output = { id: "o", name: "LOAF BREAKER", state: "connected",
                   send(d) { window.__toHost(Array.from(d, (b) => b.toString(16).padStart(2, "0")).join("")); } };
  window.__fromHost = (hex) => { const d = Uint8Array.from(hex.match(/../g).map((h) => parseInt(h, 16))); input.onmidimessage?.({ data: d }); };
  navigator.requestMIDIAccess = async () => ({ inputs: new Map([["i", input]]), outputs: new Map([["o", output]]), onstatechange: null });
});
let running = true, extra = "", extraDone = null;
const loop = (async () => {                        // the cable, a cycle after another
  while (running) {
    const x = extra, done = extraDone; extra = ""; extraDone = null;
    const got = await cycle(x);
    if (done) done();
    for (const f of got) await page.evaluate((h) => window.__fromHost(h), Buffer.from(f).toString("hex")).catch(() => {});
  }
})();
const settle = (ms) => new Promise((r) => setTimeout(r, ms));
const expect = async (what, v) => {               // a probe of the FM-1's, checked in the next cycle
  const was = said.length;
  await new Promise((r) => { extra = `expect ${what} ${v}\n`; extraDone = r; });
  await settle(50);
  return said.slice(was).includes(`ok ${what}`);
};

await page.goto(`http://localhost:${port}/editor/`);
await settle(300);
check(/far better/.test(await page.textContent("#vstbar")), "the VST's word at the top");
check((await page.getAttribute("#vstbar a", "href")) === "../vst/", "its link: the site's VST page");
check(/Recommended/.test(await page.textContent("#view")), "before connecting: the Studio and the VST side by side");
await page.screenshot({ path: join(out, "studio_welcome.png") });
await page.click("#connect");
await settle(3500);
check(await page.evaluate(() => document.getElementById("dot").className.includes("on")), "Connect finds the FM-1");
check((await page.$$("#cats button")).length === 8, "the eight categories");
await settle(2500);
const pads = await page.$$eval("#bkPads button", (b) => b.map((x) => x.textContent));
check(pads.length === 11 && pads.every((t) => t.length), `the pads: the eleven black keys\' functions (${pads.join(" ")})`);
const shot = async (cat, tab, name) => {
  await page.click(`#cats button:nth-child(${cat})`);
  await settle(400);
  if (tab) await page.click(`#nav button:nth-child(${tab})`);
  await settle(2600);
  await page.screenshot({ path: join(out, name) });
};
await shot(1, 0, "studio_live.png");
await shot(2, 1, "studio_kick.png");
const crumb = await page.textContent("#crumb");
check(/Sounds \/ Kick/i.test(crumb), `where you are: ${crumb}`);
const bars = await page.$$eval(".kn:not(.blank) .bar", (b) => b.length);
check(bars > 8, `the kick's cards, a bar a value (${bars})`);
const k = await page.$(".kn:not(.blank):not(.choice)");
const v0 = await k.$eval(".v", (e) => e.textContent);
await k.focus();
for (let i = 0; i < 3; i++) await page.keyboard.press("ArrowRight");
await settle(1500);
const v1 = await k.$eval(".v", (e) => e.textContent);
check(v1 !== v0, `a value turned on the FM-1 (${v0} -> ${v1})`);
await shot(2, 2, "studio_drums.png");
await shot(3, 1, "studio_seq.png");
await shot(3, 2, "studio_patterns.png");
await shot(4, 0, "studio_tracks.png");
await shot(5, 0, "studio_mix.png");
await shot(6, 0, "studio_fx.png");
await shot(7, 0, "studio_arp.png");
await shot(8, 0, "studio_system.png");
await page.click("#play");
await settle(800);
await settle(1200);
check(/stop/i.test(await page.textContent("#play")), "PLAY: it plays (the header's PLAY says STOP)");
await page.click("#cats button:nth-child(3)");
await settle(3000);
await page.screenshot({ path: join(out, "studio_seq_playing.png") });
running = false;
await loop;
toSim.end();
await browser.close();
srv.close();
host.kill();
console.log(fails ? `studio_page: ${fails} failed` : "studio_page: all passed");
process.exit(fails ? 1 : 0);
