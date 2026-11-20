#!/bin/sh
# Starts reelo under Xvfb on a private session bus, plays a generated test video, and checks
# what uQ's media widget would see over MPRIS: status, metadata, pause, seek, next, quit. Exits non-zero on
# any QML error or failed check. Needs: xorg-server-xvfb, dbus, playerctl, ffmpeg, imagemagick.
#
#   sh tests/gui/smoke.sh BUILD_DIR [SHOTS_DIR]
set -eu
build=${1:?usage: smoke.sh BUILD_DIR [SHOTS_DIR]}
shots=${2:-}
work=$(mktemp -d)
trap 'kill $(jobs -p) 2>/dev/null || true; rm -rf "$work"' EXIT

export HOME="$work/home" XDG_CONFIG_HOME="$work/home/.config" XDG_STATE_HOME="$work/home/.local/state"
export DISPLAY=:97 QT_QPA_PLATFORM=xcb QT_FORCE_STDERR_LOGGING=1 REELO_MPV_OPTIONS=ao=null
mkdir -p "$XDG_CONFIG_HOME/reelo" "$work/media"
printf '{"library": "%s"}\n' "$work/media" > "$XDG_CONFIG_HOME/reelo/config.json"

ffmpeg -loglevel error -f lavfi -i "testsrc2=size=640x360:rate=25:duration=20" \
    -f lavfi -i "sine=frequency=440:duration=20" -metadata title="Smoke Test" \
    -c:v libx264 -preset ultrafast -pix_fmt yuv420p -c:a aac "$work/media/smoke.mkv"
ffmpeg -loglevel error -f lavfi -i "sine=frequency=220:duration=10" -metadata title="Second" \
    "$work/media/zz-second.opus"

Xvfb :97 -screen 0 1280x800x24 >/dev/null 2>&1 &
sleep 1
eval "$(dbus-launch --sh-syntax)"
"$build/reelo" "$work/media/smoke.mkv" >"$work/log" 2>&1 &
pid=$!

fail() { echo "FAIL: $*"; echo "--- reelo's output"; cat "$work/log"; exit 1; }
wait_for() { # seconds, description, command...
    t=$1; what=$2; shift 2
    i=0
    until "$@" >/dev/null 2>&1; do
        i=$((i + 1)); [ $i -lt $((t * 4)) ] || fail "$what"; sleep 0.25
    done
}
pos() { playerctl -p reelo position 2>/dev/null | cut -d. -f1; }

wait_for 15 "reelo on the session bus" playerctl -p reelo status
wait_for 15 "playback starts" sh -c '[ "$(playerctl -p reelo position | cut -d. -f1)" -ge 1 ]'
[ "$(playerctl -p reelo status)" = Playing ] || fail "status is not Playing"
[ "$(playerctl -p reelo metadata title)" = "Smoke Test" ] || fail "title from the file's tags"
[ "$(playerctl -p reelo metadata mpris:length)" -gt 19000000 ] || fail "length"
playerctl -p reelo pause
wait_for 5 "pause over MPRIS" sh -c '[ "$(playerctl -p reelo status)" = Paused ]'
playerctl -p reelo position 12
wait_for 5 "seek over MPRIS" sh -c '[ "$(playerctl -p reelo position | cut -d. -f1)" -ge 11 ]'
[ -n "$shots" ] && { mkdir -p "$shots"; import -window root "$shots/smoke-paused.png"; }
playerctl -p reelo next
wait_for 8 "next file in the folder" sh -c '[ "$(playerctl -p reelo metadata title)" = Second ]'
playerctl -p reelo play
dbus-send --session --type=method_call --dest=org.mpris.MediaPlayer2.reelo /org/mpris/MediaPlayer2 org.mpris.MediaPlayer2.Quit
wait_for 8 "quits over MPRIS" sh -c "! kill -0 $pid"
[ -d "$XDG_STATE_HOME/reelo/watch_later" ] || fail "resume positions folder"

if grep -E "ReferenceError|TypeError|Unable to assign|is not a type|is not installed|refused" "$work/log"; then
    fail "QML or mpv errors in the log"
fi
echo "smoke test passed"
