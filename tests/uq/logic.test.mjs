// Tests for reelo's uQ Settings page (uq/pages/reelo/): logic.js and the manifest.
//   node --test 'tests/**/*.test.mjs'
import assert from "node:assert/strict";
import { readFileSync, readdirSync } from "node:fs";
import { dirname, join } from "node:path";
import test from "node:test";
import { fileURLToPath } from "node:url";

import { loadLogic } from "./load-logic.mjs";
import { parseToml } from "./toml-lite.mjs";

const here = dirname(fileURLToPath(import.meta.url));
const pageDir = join(here, "..", "..", "uq", "pages", "reelo");
const L = loadLogic(join(pageDir, "logic.js"));
const manifest = parseToml(readFileSync(join(pageDir, "page.toml"), "utf8"));
const qml = readdirSync(pageDir).filter((f) => f.endsWith(".qml"))
  .map((f) => readFileSync(join(pageDir, f), "utf8")).join("\n");

test("manifest: uq_api 2, every command used, every arg pattern anchored", () => {
  assert.equal(manifest.uq_api, 2);
  assert.equal(manifest.id, "reelo");
  for (const c of manifest.command) {
    assert.ok(c.argv[0] === "/usr/bin/reelo", c.id);
    assert.ok(qml.includes(`"${c.id}"`), `${c.id} is used by the page`);
    for (const a of c.args || [])
      assert.match(a.pattern, /^\^.*\$$/, `${c.id}.${a.name}`);
  }
  for (const s of manifest.setting)
    assert.ok(qml.includes(`settingId: "${s.id}"`), `setting ${s.id} has a row`);
});

test("manifest: the value pattern takes what the page sends and nothing that starts with -", () => {
  const set = manifest.command.find((c) => c.id === "config-set");
  const value = new RegExp(set.args[1].pattern);
  for (const ok of ["true", "false", "auto", "off", "en,eng", "~/Downloads", "/data/My Videos"])
    assert.ok(value.test(ok), ok);
  for (const bad of ["-rf", "en;rm", "relative/dir", "~/a\nb"])
    assert.ok(!value.test(bad), bad);
});

test("parseConfig", () => {
  const ok = L.parseConfig({ code: 0, output: { settings: [{ key: "resume", value: true }, { key: "hwdec", value: "off" }] } });
  assert.equal(ok.ok, true);
  assert.deepEqual(ok.values, { resume: true, hwdec: "off" });
  assert.equal(L.parseConfig({ error: "timed out" }).error, "timed out");
  assert.equal(L.parseConfig({ code: 0, output: {} }).ok, false);
});

test("parseDefault", () => {
  const out = (o) => L.parseDefault({ code: 0, output: Object.assign({ desktop_id: "io.github.asafelobotomy.reelo.desktop", types: 34 }, o) });
  assert.equal(out({ default: true, ours: 34 }).summary, "reelo opens your videos and music.");
  assert.equal(out({ default: false, ours: 0, video: "vlc.desktop", audio: "" }).summary,
               "Videos open in VLC, music in nothing.");
  assert.match(out({ default: false, ours: 3, video: "io.github.asafelobotomy.reelo.desktop", audio: "mpv.desktop" }).summary,
               /^reelo opens 3 of 34 kinds of file\. Videos open in reelo, music in mpv\.$/);
  assert.equal(L.parseDefault({ error: "x" }).ok, false);
});

test("input cleaning", () => {
  assert.equal(L.cleanLangs(" EN, eng "), "en,eng");
  assert.equal(L.cleanLangs(""), "");
  assert.equal(L.cleanLangs("english"), null);
  assert.equal(L.cleanFolder("~/Videos/"), "~/Videos");
  assert.equal(L.cleanFolder("/data/My Videos"), "/data/My Videos");
  assert.equal(L.cleanFolder("Videos"), null);
  assert.equal(L.cleanFolder("~/../etc"), null);
  assert.equal(L.cleanFolder("~"), null);
  assert.equal(L.boolArg(true), "true");
});

test("failure and library summary", () => {
  assert.equal(L.failure({ code: 2, error: "exit 2: reelo: hwdec is one of auto, off" }), "hwdec is one of auto, off");
  assert.equal(L.failure({ error: "Cancelled", cancelled: true }), "");
  assert.equal(L.librarySummary({ code: 0, output: { items: [{ kind: "video" }, { kind: "audio" }, { kind: "video" }] } }, "~/Downloads"),
               "2 videos and 1 song in ~/Downloads.");
  assert.equal(L.librarySummary({ code: 0, output: { items: [] } }, "~/Downloads"), "Nothing to play in ~/Downloads yet.");
});
