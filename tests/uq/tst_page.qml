// reelo's uQ Settings page against the stub org.uq.settings module and a fake ctx:
//   QT_QPA_PLATFORM=offscreen qmltestrunner -import tests/uq/stub -input tests/uq/tst_page.qml
import QtQuick
import QtTest

Item {
    id: root
    width: 900
    height: 700

    readonly property url pageUrl: Qt.resolvedUrl("../../uq/pages/reelo/Page.qml")

    property var answers: ({
                config: { code: 0, output: { settings: [
                            { key: "resume", value: true }, { key: "autoplay_next", value: true },
                            { key: "keep_awake", value: false }, { key: "hwdec", value: "auto" },
                            { key: "subtitles", value: "auto" }, { key: "sub_lang", value: "" },
                            { key: "audio_lang", value: "" }, { key: "library", value: "~/Downloads" }] } },
                "default-status": { code: 0, output: { desktop_id: "io.github.asafelobotomy.reelo.desktop",
                                    types: 34, ours: 0, default: false, video: "vlc.desktop", audio: "vlc.desktop" } },
                library: { code: 0, output: { folder: "~/Downloads", items: [{ kind: "video" }, { kind: "audio" }] } },
                "config-set": { code: 0, output: "ok" },
                "config-reset": { code: 0, output: "ok" },
                "default-set": { code: 0, output: { default: true } }
            })

    FakeContext {
        id: fake
        confirmed: ["default-set"]
        commands: root.answers
    }

    Loader {
        id: loader
        anchors.fill: parent
    }

    function find(item, pred) {
        if (pred(item))
            return item;
        for (let i = 0; i < item.children.length; ++i) {
            const hit = find(item.children[i], pred);
            if (hit)
                return hit;
        }
        if (item.contentItem && item.contentItem !== item) {
            const hit = find(item.contentItem, pred);
            if (hit)
                return hit;
        }
        return null;
    }
    function label(text) { return find(loader.item, i => i.text === text && i.hasOwnProperty("wrapMode")); }
    function row(id) { return find(loader.item, i => i.settingId === id); }
    function button(text) { return find(loader.item, i => i.text === text && i.hasOwnProperty("checkable")); }

    TestCase {
        name: "ReeloPage"
        when: windowShown

        function init() {
            fake.commands = root.answers;
            fake.calls = [];
            loader.source = "";
            loader.source = root.pageUrl;
            verify(loader.status === Loader.Ready, "the page loads");
            loader.item.ctx = fake.ctx;
        }

        function test_reads_and_shows() {
            tryCompare(row("default"), "label", "Videos open in VLC, music in VLC.");
            compare(fake.calls[0], ["watch", "config"]);
            tryVerify(() => label("1 video and 1 song in ~/Downloads.") !== null);
            tryCompare(row("resume"), "value", true);
            tryCompare(row("keep_awake"), "value", false);
            tryCompare(row("hwdec"), "value", true);
        }

        function test_a_switch_sets_the_key() {
            tryCompare(row("keep_awake"), "value", false);
            row("keep_awake").chosen(true);
            tryVerify(() => fake.calls.some(c => c[0] === "run" && c[1] === "config-set"));
            const call = fake.calls.find(c => c[1] === "config-set");
            compare(call[2], ["keep_awake", "true"]);
        }

        function test_hwdec_off_sends_off() {
            tryCompare(row("hwdec"), "value", true);
            row("hwdec").chosen(false);
            tryVerify(() => fake.calls.some(c => c[1] === "config-set"));
            compare(fake.calls.find(c => c[1] === "config-set")[2], ["hwdec", "off"]);
        }

        function test_make_default() {
            tryVerify(() => button("Make reelo the default") !== null && button("Make reelo the default").visible);
            button("Make reelo the default").clicked();
            tryVerify(() => fake.calls.some(c => c[1] === "default-set"));
        }

        function test_open_app() {
            tryVerify(() => button("Open reelo") !== null);
            button("Open reelo").clicked();
            tryVerify(() => fake.calls.some(c => c[0] === "openApp" && c[1] === "io.github.asafelobotomy.reelo.desktop"));
        }

        function test_failed_read_shows_error() {
            fake.commands = Object.assign({}, fake.commands, { config: { error: "timed out" } });
            loader.source = "";
            loader.source = root.pageUrl;
            loader.item.ctx = fake.ctx;
            tryCompare(loader.item, "configError", "timed out");
        }
    }
}
