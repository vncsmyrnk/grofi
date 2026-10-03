/* extension.js
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

import Gio from 'gi://Gio';
import GLib from 'gi://GLib';
import Meta from 'gi://Meta';
import Shell from 'gi://Shell';
import Clutter from 'gi://Clutter';
import { Extension } from 'resource:///org/gnome/shell/extensions/extension.js';
import * as Main from 'resource:///org/gnome/shell/ui/main.js';

declare const global: Shell.Global;

Gio._promisify(Gio.Subprocess.prototype, 'communicate_utf8_async');

export default class GRofi extends Extension {
  _gsettings?: Gio.Settings

  async launchRofi() {
    try {
      const windows = global.display.get_tab_list(Meta.TabList.NORMAL_ALL, null);
      const windowTracker = Shell.WindowTracker.get_default();
      const sanitize = (value: string) => value.replace(/[\t\r\n\0]/g, ' ');
      const records = windows.map(window => {
        const label = sanitize(
          window.get_title() ?? window.get_wm_class() ?? 'Untitled window',
        );
        const icon = windowTracker
          .get_window_app(window)
          ?.get_app_info()
          ?.get_icon()
          ?.to_string();

        return `${sanitize(icon ?? '')}\t${label}`;
      });
      const input = records.length === 0 ? '' : `${records.join('\n')}\n`;
      const pluginPath = GLib.build_filenamev([this.path, 'plugins']);
      const settings = this._gsettings;
      if (settings === undefined) {
        throw new Error('extension settings are unavailable');
      }

      const extraArguments = settings.get_strv('rofi-extra-arguments');

      const proc = Gio.Subprocess.new(
        [
          "rofi",
          ...extraArguments,
          "-plugin-path", pluginPath,
        ],
        Gio.SubprocessFlags.STDIN_PIPE | Gio.SubprocessFlags.STDOUT_PIPE | Gio.SubprocessFlags.STDERR_PIPE,
      );
      const [stdout, stderr] = await proc.communicate_utf8_async(input, null);

      if (!proc.get_successful()) {
        throw new Error(stderr || `rofi exited with status ${proc.get_exit_status()}`);
      }

      const selection = stdout.trim();
      if (selection === '') {
        return;
      }
      if (!/^\d+$/.test(selection)) {
        throw new Error(`rofi returned an invalid window index: ${selection}`);
      }

      const selectedIndex = Number(selection);
      const selectedWindow = windows[selectedIndex];
      if (selectedWindow === undefined) {
        throw new Error(`rofi returned an out-of-range window index: ${selectedIndex}`);
      }

      Main.activateWindow(selectedWindow);
    } catch (error) {
      logError(error);
    }
  }

  enable() {
    this._gsettings = this.getSettings();

    Main.wm.addKeybinding("launch-rofi", this._gsettings, Meta.KeyBindingFlags.NONE, Shell.ActionMode.NORMAL,
      async (display: Meta.Display, window: Meta.Window, event: Clutter.Event, binding: Meta.KeyBinding): Promise<void> => {
        console.warn(display, window, event, binding);
        await this.launchRofi();
      });
  }

  disable() {
    console.warn("disabled");
    Main.wm.removeKeybinding("launch-rofi");
  }
}
