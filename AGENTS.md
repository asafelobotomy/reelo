# Agent notes for reelo

| Path | What |
|---|---|
| `src/config, media, playlist, library, theme, defaultapp, cli` | `reelo_core`: no window, no mpv. The CLI and `tests/core` use only this |
| `src/mpvobject` | libmpv in a QQuickFramebufferObject (OpenGL render API) and the observed properties |
| `src/app` | the `App` QML singleton: queue → mpv, settings → mpv options, resume, keep-awake |
| `src/mpris` | MPRIS 2 on the session bus |
| `qml/` | the window; pure helpers in `qml/logic.js` (Node-tested) |
| `uq/pages/reelo` | the uQ Settings page (uq_api 2); every command is a `reelo` subcommand |

Rules: the window is OpenGL only (`QQuickWindow::setGraphicsApi`); files load only after the
renderer exists (`MpvObject::markReady`), or mpv starts them without video. Never read the user's
mpv config. Every text the uQ page shows goes through `UqLabel`. Keep files under ~400 lines.

Verify: `cmake --build build && ctest --test-dir build && node --test 'tests/**/*.test.mjs' &&
qmltestrunner -import tests/uq/stub -input tests/uq/tst_page.qml && sh tests/gui/smoke.sh build`.

Release: bump `VERSION` (and the metainfo release) in a PR; merging it tags vX.Y.Z and runs
release.yml, whose `aur` artifact holds the AUR files. The AUR push is manual.
