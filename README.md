[![GitHub main branch check runs](https://img.shields.io/github/check-runs/vncsmyrnk/grofi/main?style=plastic&logo=github&label=CI%20workflow)](https://github.com/vncsmyrnk/grofi/actions/workflows/ci.yaml)

A GNOME Shell extension that feeds open windows to the bundled `grofi-windows` Rofi plugin and combines it with Rofi's built-in `drun` mode. This provides one search view for open windows and installed applications.

Building requires Rofi 2.0 or newer with plugin headers, GLib/GIO development files, npm, and a C compiler.

## Install

```sh
autoreconf -fi
./configure
make install
```

## Rofi configuration

The Rofi executable and optional arguments are read from GSettings on every launch, so changes apply without restarting GNOME Shell:

```sh
gsettings set org.gnome.shell.extensions.grofi rofi-extra-arguments \
  "['-theme', 'Arc-Dark', '-monitor', '-1']"
```

Reset value to its packaged default:

```sh
gsettings reset org.gnome.shell.extensions.grofi rofi-extra-arguments
```

The extension always appends the plugin path, combi modes, icon support, and initial mode required by its window-selection protocol.
