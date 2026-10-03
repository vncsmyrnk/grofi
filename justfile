default:
  just --list

dbus-run-session:
  dbus-run-session gnome-shell --devkit --wayland
