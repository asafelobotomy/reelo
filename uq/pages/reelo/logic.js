.pragma library
// Pure helpers for reelo's uQ Settings page: no Qt calls, so tests/uq/logic.test.mjs runs them
// with Node.

/// `reelo config show --json` -> {ok, values: {key: value}, settings: [...]}, or {ok:false, error}.
function parseConfig(result) {
    if (!result || result.error)
        return { ok: false, error: (result && result.error) || "No answer from reelo" };
    const out = result.output;
    if (!out || !Array.isArray(out.settings))
        return { ok: false, error: "reelo gave an answer this page doesn't understand" };
    const values = {};
    for (const s of out.settings)
        if (s && typeof s.key === "string")
            values[s.key] = s.value;
    return { ok: true, values: values, settings: out.settings };
}

/// `reelo default status --json` -> {ok, isDefault, video, audio, summary}.
function parseDefault(result) {
    if (!result || result.error || !result.output)
        return { ok: false, error: (result && result.error) || "No answer from reelo" };
    const o = result.output;
    const name = id => !id ? "nothing" : id === o.desktop_id ? "reelo" : prettyApp(id);
    let summary;
    if (o.default)
        summary = "reelo opens your videos and music.";
    else if (o.ours > 0)
        summary = "reelo opens " + o.ours + " of " + o.types + " kinds of file. Videos open in "
            + name(o.video) + ", music in " + name(o.audio) + ".";
    else
        summary = "Videos open in " + name(o.video) + ", music in " + name(o.audio) + ".";
    return { ok: true, isDefault: o.default === true, video: o.video || "", audio: o.audio || "",
             summary: summary };
}

/// "vlc.desktop" -> "VLC", "org.kde.dragonplayer.desktop" -> "dragonplayer", "mpv.desktop" -> "mpv".
function prettyApp(id) {
    let s = String(id || "").replace(/\.desktop$/, "");
    const known = { vlc: "VLC", mpv: "mpv", "io.mpv.Mpv": "mpv", celluloid: "Celluloid",
                    "io.github.celluloid_player.Celluloid": "Celluloid", totem: "Videos",
                    "org.gnome.Totem": "Videos", haruna: "Haruna", "org.kde.haruna": "Haruna" };
    if (known[s])
        return known[s];
    const parts = s.split(".");
    return parts[parts.length - 1] || s;
}

/// The argument `reelo config set` gets for a switch.
function boolArg(on) {
    return on ? "true" : "false";
}

/// A typed language list cleaned up: " EN, eng " -> "en,eng"; null when it isn't one.
function cleanLangs(text) {
    const t = String(text || "").trim().toLowerCase().replace(/\s+/g, "");
    if (t === "")
        return "";
    return /^[a-z]{2,3}(,[a-z]{2,3}){0,7}$/.test(t) ? t : null;
}

/// A typed folder: "~/Videos" or "/data/media"; null when it isn't one the page may send.
function cleanFolder(text) {
    const t = String(text || "").trim().replace(/\/+$/, "");
    if (t === "~")
        return null;
    return /^~?\/[A-Za-z0-9 ._+@'()\/-]{0,300}$/.test(t) && !/(^|\/)\.\.(\/|$)/.test(t) ? t : null;
}

/// The first line of what a failed change said, for the row under it.
function failure(result) {
    if (!result)
        return "No answer from reelo";
    if (result.cancelled)
        return "";
    const e = String(result.error || "");
    const m = e.match(/reelo: (.*)/);
    return (m ? m[1] : e).split("\n")[0];
}

/// "~/Downloads" with n items -> "12 videos and songs in ~/Downloads".
function librarySummary(result, folder) {
    if (!result || result.error || !result.output)
        return "";
    const items = result.output.items || [];
    if (items.length === 0)
        return "Nothing to play in " + folder + " yet.";
    const v = items.filter(i => i.kind === "video").length;
    const a = items.length - v;
    const parts = [];
    if (v)
        parts.push(v + (v === 1 ? " video" : " videos"));
    if (a)
        parts.push(a + (a === 1 ? " song" : " songs"));
    return parts.join(" and ") + " in " + folder + (items.length >= 60 ? " (newest 60)" : "") + ".";
}
