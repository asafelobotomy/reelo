# Changelog

## 0.1.1 — 2026-11-21

- One window per session: opening a video or song while reelo is open plays it in that window
  and brings it forward, instead of starting a second player. `reelo --new-window FILE` still
  opens another.

## 0.1.0 — 2026-11-20

First release.

- libmpv in a Qt Quick window that follows uQ's theme live (colours, corners, fonts).
- Start screen with the newest videos and music in the library folder (`~/Downloads`).
- Queue of the folder's media in natural order; next/previous; drag and drop.
- Resume where you stopped; subtitle and audio track menus; speed; frame stepping.
- MPRIS 2 for uQ's media widget, `playerctl` and media keys.
- Keeps the screen on while a video plays.
- `reelo config`, `reelo default`, `reelo library` and a uQ Settings page.
