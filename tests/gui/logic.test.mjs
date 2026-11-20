// Tests for the window's pure helpers (qml/logic.js).
//   node --test tests/gui/ tests/uq/
import assert from "node:assert/strict";
import { dirname, join } from "node:path";
import test from "node:test";
import { fileURLToPath } from "node:url";

import { loadLogic } from "../uq/load-logic.mjs";

const here = dirname(fileURLToPath(import.meta.url));
const L = loadLogic(join(here, "..", "..", "qml", "logic.js"));

test("formatTime", () => {
  assert.equal(L.formatTime(0), "0:00");
  assert.equal(L.formatTime(75.9), "1:15");
  assert.equal(L.formatTime(3725), "1:02:05");
  assert.equal(L.formatTime(NaN), "0:00");
  assert.equal(L.formatTime(-4), "0:00");
});

test("formatSize", () => {
  assert.equal(L.formatSize(740), "740 B");
  assert.equal(L.formatSize(1_200_000_000), "1.2 GB");
  assert.equal(L.formatSize(38_025_149), "38.0 MB");
});

test("ago", () => {
  const now = Date.UTC(2026, 10, 20, 12);
  assert.equal(L.ago(now - 10_000, now), "just now");
  assert.equal(L.ago(now - 5 * 60_000, now), "5 min ago");
  assert.equal(L.ago(now - 3 * 3600_000, now), "3 h ago");
  assert.equal(L.ago(now - 30 * 3600_000, now), "yesterday");
  assert.equal(L.ago(now - 4 * 86400_000, now), "4 days ago");
  assert.match(L.ago(now - 40 * 86400_000, now), /^2026-10-1\d$/);
});

test("tracks", () => {
  const list = [
    { id: 1, type: "video", selected: true },
    { id: 1, type: "audio", lang: "eng", codec: "aac", "demux-channel-count": 2, selected: true },
    { id: 2, type: "audio", lang: "jpn", title: "Commentary", codec: "aac" },
    { id: 1, type: "sub", lang: "en", external: true, codec: "subrip" },
    { id: 3, type: "video", albumart: true },
  ];
  assert.equal(L.tracksOf(list, "audio").length, 2);
  assert.equal(L.tracksOf(list, "video").length, 1);
  assert.equal(L.selectedId(list, "audio"), 1);
  assert.equal(L.selectedId(list, "sub"), 0);
  assert.equal(L.trackLabel(list[1]), "eng · AAC · 2 ch");
  assert.equal(L.trackLabel(list[2]), "Commentary (jpn) · AAC");
  assert.equal(L.trackLabel(list[3]), "en · SUBRIP · file");
});

test("speed", () => {
  assert.equal(L.speedLabel(1), "1×");
  assert.equal(L.speedLabel(1.25), "1.25×");
  assert.equal(L.stepSpeed(1, true), 1.25);
  assert.equal(L.stepSpeed(1, false), 0.75);
  assert.equal(L.stepSpeed(4, true), 4);
  assert.equal(L.stepSpeed(0.25, false), 0.25);
  assert.equal(L.stepSpeed(1.1, false), 1);
});

test("windowTitle and dropped", () => {
  assert.equal(L.windowTitle("Film", false), "Film — reelo");
  assert.equal(L.windowTitle("Film", true), "reelo");
  assert.deepEqual(L.dropped(["file:///home/a/My%20Film.mkv", "https://x.org/v"]),
                   ["/home/a/My Film.mkv", "https://x.org/v"]);
});
