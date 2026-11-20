.pragma library
// Pure helpers for the window (no Qt calls), tested with Node in tests/gui/logic.test.mjs.

/// 75 -> "1:15", 3725 -> "1:02:05"; nothing known -> "0:00".
function formatTime(seconds) {
    if (!isFinite(seconds) || seconds < 0)
        seconds = 0;
    const s = Math.floor(seconds);
    const h = Math.floor(s / 3600);
    const m = Math.floor((s % 3600) / 60);
    const sec = s % 60;
    const two = n => (n < 10 ? "0" : "") + n;
    return h > 0 ? h + ":" + two(m) + ":" + two(sec) : m + ":" + two(sec);
}

/// Bytes as "740 KB", "1.2 GB".
function formatSize(bytes) {
    if (!isFinite(bytes) || bytes < 0)
        return "";
    const units = ["B", "KB", "MB", "GB", "TB"];
    let i = 0;
    let v = bytes;
    while (v >= 1000 && i < units.length - 1) {
        v /= 1000;
        ++i;
    }
    return (i === 0 || v >= 100 ? Math.round(v) : v.toFixed(1)) + " " + units[i];
}

/// How long ago `ms` (since the epoch) was, from `now`: "just now", "5 min ago", "3 h ago",
/// "yesterday", "4 days ago", then the date.
function ago(ms, now) {
    const d = Math.max(0, (now - ms) / 1000);
    if (d < 60)
        return "just now";
    if (d < 3600)
        return Math.floor(d / 60) + " min ago";
    if (d < 86400)
        return Math.floor(d / 3600) + " h ago";
    if (d < 2 * 86400)
        return "yesterday";
    if (d < 7 * 86400)
        return Math.floor(d / 86400) + " days ago";
    const date = new Date(ms);
    return date.getFullYear() + "-" + String(date.getMonth() + 1).padStart(2, "0") + "-"
        + String(date.getDate()).padStart(2, "0");
}

/// A track from mpv's track-list as a menu label: "English (eng) · ASS · default".
function trackLabel(t) {
    const parts = [];
    const name = t.title || t.lang || ("Track " + t.id);
    parts.push(t.title && t.lang ? name + " (" + t.lang + ")" : name);
    if (t.codec)
        parts.push(String(t.codec).toUpperCase());
    if (t.type === "audio" && t["demux-channel-count"])
        parts.push(t["demux-channel-count"] + " ch");
    if (t.external)
        parts.push("file");
    if (t.forced)
        parts.push("forced");
    return parts.join(" · ");
}

/// The tracks of one type ("sub", "audio"), in mpv's order.
function tracksOf(list, type) {
    return (list || []).filter(t => t && t.type === type && !t.albumart);
}

/// The id of the selected track of a type, or 0 for none.
function selectedId(list, type) {
    const t = tracksOf(list, type).find(x => x.selected);
    return t ? t.id : 0;
}

/// 1 -> "1×", 1.25 -> "1.25×".
function speedLabel(speed) {
    return (Math.round(speed * 100) / 100) + "×";
}

/// The next speed up or down from a fixed ladder.
function stepSpeed(speed, up) {
    const ladder = [0.25, 0.5, 0.75, 1, 1.25, 1.5, 1.75, 2, 2.5, 3, 4];
    if (up) {
        const n = ladder.find(s => s > speed + 0.001);
        return n === undefined ? ladder[ladder.length - 1] : n;
    }
    const lower = ladder.filter(s => s < speed - 0.001);
    return lower.length ? lower[lower.length - 1] : ladder[0];
}

/// The window title: "Title — reelo", or "reelo" with nothing open.
function windowTitle(title, idle) {
    return idle || !title ? "reelo" : title + " — reelo";
}

/// The text of a dropped or pasted list of URLs: local files as paths, the rest as is.
function dropped(urls) {
    const out = [];
    for (let i = 0; i < (urls || []).length; ++i) {
        const u = String(urls[i]);
        out.push(u.startsWith("file://") ? decodeURIComponent(u.slice(7)) : u);
    }
    return out;
}
