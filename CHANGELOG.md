# Changelog

## 0.1.3 — 2026-11-22

- **Moves like the desktop.** Hover and press colours fade in 100 ms instead of snapping, and fades, slides and toasts use uQ's shell speeds (100, 140 and 220 ms) with an ease-out curve instead of a mix of 70 to 250 ms and linear easing. The speeds are `Theme.motionFast`, `motionBase`, `motionSlow` and `Theme.easing`, matching uQ's `shell/core/Motion.qml`.

## 0.1.2 — 2026-11-21

- Follows the uQ theme through uQ's shared uq-theme module (`src/uqtheme.*`, uq-theme 1.0.0) instead of
  its own copy. It also catches uQ swapping the whole theme folder. A theme missing colours now
  counts as no theme (reelo's own dark palette), as in uQ's other apps.

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
