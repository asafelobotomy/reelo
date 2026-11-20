// reelo's page in uQ Settings (docs/settings-pages.md in the uQ repository), laid out like
// Settings' own pages (uq_api = 2). Every read and change is a `reelo` command declared in
// page.toml; all text goes through UqLabel. The pure parts live in logic.js.
import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.uq.settings
import "logic.js" as Logic

SettingsPage {
    id: page

    property var ctx
    property string highlight
    /// What the last reads said: null until the first answer.
    property var config: null
    property var defaults: null
    property string configError: ""
    property string librarySummary: ""
    /// The key whose change is running ("" when none), and what each change said, by key.
    property string workingKey: ""
    property var notes: ({})
    property bool dead: false
    property var watches: []

    pageId: "reelo"

    Component.onDestruction: dead = true

    onCtxChanged: {
        if (!ctx)
            return;
        page.watches.push(ctx.command.watch("config", r => {
            if (page.dead)
                return;
            const c = Logic.parseConfig(r);
            page.configError = c.ok ? "" : c.error;
            if (c.ok)
                page.config = c.values;
        }));
        page.watches.push(ctx.command.watch("default-status", r => {
            if (!page.dead)
                page.defaults = Logic.parseDefault(r);
        }));
        readLibrary();
    }

    function value(key) {
        return page.config ? page.config[key] : undefined;
    }

    function readLibrary() {
        ctx.command.run("library", [], r => {
            if (!page.dead)
                page.librarySummary = Logic.librarySummary(r, page.value("library") || "the library folder");
        });
    }

    function note(key, text) {
        const n = Object.assign({}, page.notes);
        n[key] = text;
        page.notes = n;
    }

    /// One change; the watched read shows the new value when the file changes.
    function setKey(key, value) {
        if (!ctx || page.workingKey !== "")
            return;
        page.workingKey = key;
        note(key, "");
        const done = r => {
            if (page.dead)
                return;
            page.workingKey = "";
            if (r.error)
                note(key, Logic.failure(r));
            if (key === "library")
                readLibrary();
        };
        if (value === "")
            ctx.command.run("config-reset", [key], done);
        else
            ctx.command.run("config-set", [key, value], done);
    }

    function makeDefault() {
        page.workingKey = "default";
        note("default", "");
        ctx.command.run("default-set", [], r => {
            if (page.dead)
                return;
            page.workingKey = "";
            if (r.error)
                note("default", Logic.failure(r));
        });
    }

    banners: [
        ErrorCard {
            visible: page.configError !== ""
            title: "Couldn't read reelo's settings"
            detail: page.configError
        }
    ]

    SettingsTab {
        title: "Media player"
        iconName: "io.github.asafelobotomy.reelo"

        SettingsCard {
            title: "Default player"
            iconName: "applications-multimedia"
            help: "Which app opens a video or song when you open it from Files or a download."

            SettingRow {
                settingId: "default"
                label: page.defaults && page.defaults.ok ? page.defaults.summary : "Checking…"
                highlight: page.highlight
                QQC2.Button {
                    text: "Make reelo the default"
                    visible: !!page.defaults && page.defaults.ok && !page.defaults.isDefault
                    enabled: page.workingKey === ""
                    onClicked: page.makeDefault()
                }
                QQC2.Button {
                    text: "Open reelo"
                    onClicked: page.ctx.openApp("io.github.asafelobotomy.reelo.desktop")
                }
            }
            UqLabel {
                visible: text !== ""
                text: page.notes["default"] || ""
                color: (page.ctx && page.ctx.theme.colors && page.ctx.theme.colors.error) || "red"
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }
        }

        SettingsCard {
            title: "Playing"
            iconName: "media-playback-start"

            SwitchRow {
                settingId: "resume"
                label: "Resume where you stopped"
                highlight: page.highlight
                value: page.value("resume") === true
                enabled: page.config !== null && page.workingKey === ""
                onChosen: v => page.setKey("resume", Logic.boolArg(v))
            }
            SwitchRow {
                settingId: "autoplay_next"
                label: "Play the next file in the folder"
                highlight: page.highlight
                value: page.value("autoplay_next") === true
                enabled: page.config !== null && page.workingKey === ""
                onChosen: v => page.setKey("autoplay_next", Logic.boolArg(v))
            }
            SwitchRow {
                settingId: "keep_awake"
                label: "Keep the screen on during videos"
                highlight: page.highlight
                value: page.value("keep_awake") === true
                enabled: page.config !== null && page.workingKey === ""
                onChosen: v => page.setKey("keep_awake", Logic.boolArg(v))
            }
            SwitchRow {
                settingId: "hwdec"
                label: "Decode on the graphics card"
                highlight: page.highlight
                value: page.value("hwdec") === "auto"
                enabled: page.config !== null && page.workingKey === ""
                onChosen: v => page.setKey("hwdec", v ? "auto" : "off")
                HelpTip { tip: "Uses less power. Turn it off if a video shows green blocks or stutters." }
            }
        }

        SettingsCard {
            title: "Subtitles and audio"
            iconName: "media-view-subtitles-symbolic"

            SwitchRow {
                settingId: "subtitles"
                label: "Show subtitles when a file has them"
                highlight: page.highlight
                value: page.value("subtitles") === "auto"
                enabled: page.config !== null && page.workingKey === ""
                onChosen: v => page.setKey("subtitles", v ? "auto" : "off")
            }
            SettingRow {
                settingId: "sub_lang"
                label: "Subtitle languages"
                highlight: page.highlight
                QQC2.TextField {
                    id: subLang
                    placeholderText: "en,eng"
                    text: page.value("sub_lang") || ""
                    Layout.preferredWidth: 140
                    onAccepted: saveSub.clicked()
                }
                QQC2.Button {
                    id: saveSub
                    text: "Save"
                    enabled: page.workingKey === "" && Logic.cleanLangs(subLang.text) !== null
                             && Logic.cleanLangs(subLang.text) !== (page.value("sub_lang") || "")
                    onClicked: page.setKey("sub_lang", Logic.cleanLangs(subLang.text))
                }
            }
            SettingRow {
                settingId: "audio_lang"
                label: "Audio languages"
                highlight: page.highlight
                QQC2.TextField {
                    id: audioLang
                    placeholderText: "ja,jpn"
                    text: page.value("audio_lang") || ""
                    Layout.preferredWidth: 140
                    onAccepted: saveAudio.clicked()
                }
                QQC2.Button {
                    id: saveAudio
                    text: "Save"
                    enabled: page.workingKey === "" && Logic.cleanLangs(audioLang.text) !== null
                             && Logic.cleanLangs(audioLang.text) !== (page.value("audio_lang") || "")
                    onClicked: page.setKey("audio_lang", Logic.cleanLangs(audioLang.text))
                }
            }
            UqLabel {
                visible: text !== ""
                text: page.notes["sub_lang"] || page.notes["audio_lang"] || ""
                color: (page.ctx && page.ctx.theme.colors && page.ctx.theme.colors.error) || "red"
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }
        }

        SettingsCard {
            title: "Library"
            iconName: "folder-videos"
            help: "reelo's start screen lists the newest videos and music in this folder. AV Morning Star and hauler save to ~/Downloads."

            SettingRow {
                settingId: "library"
                label: "Folder"
                highlight: page.highlight
                QQC2.TextField {
                    id: folder
                    placeholderText: "~/Downloads"
                    text: page.value("library") || ""
                    Layout.preferredWidth: 200
                    onAccepted: saveFolder.clicked()
                }
                QQC2.Button {
                    id: saveFolder
                    text: "Save"
                    enabled: page.workingKey === "" && Logic.cleanFolder(folder.text) !== null
                             && Logic.cleanFolder(folder.text) !== page.value("library")
                    onClicked: page.setKey("library", Logic.cleanFolder(folder.text))
                }
            }
            UqLabel {
                text: page.notes["library"] || page.librarySummary
                color: page.notes["library"] ? ((page.ctx && page.ctx.theme.colors && page.ctx.theme.colors.error) || "red")
                                             : ((page.ctx && page.ctx.theme.colors && page.ctx.theme.colors.text_muted) || "gray")
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }
        }
    }
}
