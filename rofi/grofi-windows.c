#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <glib.h>
#include <gmodule.h>

#include <rofi/helper.h>
#include <rofi/mode-private.h>
#include <rofi/rofi-icon-fetcher.h>

typedef struct {
  char *label;
  char *icon_name;
  uint32_t icon_fetch_uid;
  unsigned int icon_fetch_size;
} GrofiWindowEntry;

typedef struct {
  GPtrArray *entries;
} GrofiWindowsModeData;

static void grofi_window_entry_free(gpointer pointer) {
  GrofiWindowEntry *entry = pointer;

  g_free(entry->label);
  g_free(entry->icon_name);
  g_free(entry);
}

static gboolean write_all(int fd, const char *data, size_t length) {
  while (length > 0) {
    ssize_t written = write(fd, data, length);

    if (written < 0) {
      if (errno == EINTR) {
        continue;
      }

      return FALSE;
    }
    if (written == 0) {
      errno = EIO;
      return FALSE;
    }

    data += written;
    length -= (size_t)written;
  }

  return TRUE;
}

static int grofi_windows_mode_init(Mode *mode) {
  if (mode_get_private_data(mode) != NULL) {
    return TRUE;
  }

  GrofiWindowsModeData *data = g_new0(GrofiWindowsModeData, 1);
  data->entries = g_ptr_array_new_with_free_func(grofi_window_entry_free);

  FILE *input = fdopen(dup(STDIN_FILENO), "r");
  if (input == NULL) {
    g_warning("Unable to read grofi-windows input: %s", g_strerror(errno));
    g_ptr_array_free(data->entries, TRUE);
    g_free(data);
    return FALSE;
  }

  char *line = NULL;
  size_t capacity = 0;
  ssize_t length;

  while ((length = getline(&line, &capacity, input)) >= 0) {
    while (length > 0 &&
           (line[length - 1] == '\n' || line[length - 1] == '\r')) {
      line[--length] = '\0';
    }

    GrofiWindowEntry *entry = g_new0(GrofiWindowEntry, 1);
    char *separator = strchr(line, '\t');

    if (separator == NULL) {
      entry->label = g_strdup(line);
    } else {
      *separator = '\0';
      entry->icon_name = line[0] == '\0' ? NULL : g_strdup(line);
      entry->label = g_strdup(separator + 1);
    }

    g_ptr_array_add(data->entries, entry);
  }

  gboolean read_succeeded = feof(input);
  free(line);
  fclose(input);

  if (!read_succeeded) {
    g_warning("Unable to read grofi-windows entries: %s", g_strerror(errno));
    g_ptr_array_free(data->entries, TRUE);
    g_free(data);
    return FALSE;
  }

  mode_set_private_data(mode, data);
  return TRUE;
}

static void grofi_windows_mode_destroy(Mode *mode) {
  GrofiWindowsModeData *data = mode_get_private_data(mode);
  if (data == NULL) {
    return;
  }

  g_ptr_array_free(data->entries, TRUE);
  g_free(data);
  mode_set_private_data(mode, NULL);
}

static unsigned int grofi_windows_mode_get_num_entries(const Mode *mode) {
  const GrofiWindowsModeData *data = mode_get_private_data(mode);
  return data == NULL ? 0 : data->entries->len;
}

static char *grofi_windows_mode_get_display_value(const Mode *mode,
                                                  unsigned int selected_line,
                                                  int *state,
                                                  GList **attribute_list,
                                                  int get_entry) {
  const GrofiWindowsModeData *data = mode_get_private_data(mode);

  if (state != NULL) {
    *state = 0;
  }
  if (attribute_list != NULL) {
    *attribute_list = NULL;
  }
  if (!get_entry || data == NULL || selected_line >= data->entries->len) {
    return NULL;
  }

  const GrofiWindowEntry *entry =
      g_ptr_array_index(data->entries, selected_line);
  return g_strdup(entry->label);
}

static int grofi_windows_mode_token_match(const Mode *mode,
                                          rofi_int_matcher **tokens,
                                          unsigned int selected_line) {
  const GrofiWindowsModeData *data = mode_get_private_data(mode);

  if (data == NULL || selected_line >= data->entries->len) {
    return FALSE;
  }

  const GrofiWindowEntry *entry =
      g_ptr_array_index(data->entries, selected_line);
  return helper_token_match(tokens, entry->label);
}

static cairo_surface_t *grofi_windows_mode_get_icon(const Mode *mode,
                                                    unsigned int selected_line,
                                                    unsigned int height) {
  GrofiWindowsModeData *data = mode_get_private_data(mode);

  if (data == NULL || selected_line >= data->entries->len) {
    return NULL;
  }

  GrofiWindowEntry *entry = g_ptr_array_index(data->entries, selected_line);
  if (entry->icon_name == NULL) {
    return NULL;
  }

  if (entry->icon_fetch_uid == 0 || entry->icon_fetch_size != height) {
    entry->icon_fetch_uid =
        rofi_icon_fetcher_query(entry->icon_name, (int)height);
    entry->icon_fetch_size = height;
  }

  return rofi_icon_fetcher_get(entry->icon_fetch_uid);
}

static ModeMode grofi_windows_mode_result(Mode *mode,
                                          int menu_retv,
                                          char **input,
                                          unsigned int selected_line) {
  GrofiWindowsModeData *data = mode_get_private_data(mode);
  (void)input;

  if (menu_retv & MENU_NEXT) {
    return NEXT_DIALOG;
  }
  if (menu_retv & MENU_PREVIOUS) {
    return PREVIOUS_DIALOG;
  }
  if (menu_retv & MENU_QUICK_SWITCH) {
    return menu_retv & MENU_LOWER_MASK;
  }

  if ((menu_retv & MENU_OK) && data != NULL &&
      selected_line < data->entries->len) {
    char response[32];
    int length = g_snprintf(response, sizeof(response), "%u\n", selected_line);

    if (!write_all(STDOUT_FILENO, response, (size_t)length)) {
      g_warning("Unable to send grofi-windows selection: %s",
                g_strerror(errno));
    }
  }

  return MODE_EXIT;
}

G_MODULE_EXPORT Mode mode = {
    .abi_version = ABI_VERSION,
    .name = "grofi-windows",
    .cfg_name_key = "display-grofi-windows",
    .display_name = NULL,
    ._init = grofi_windows_mode_init,
    ._destroy = grofi_windows_mode_destroy,
    ._get_num_entries = grofi_windows_mode_get_num_entries,
    ._result = grofi_windows_mode_result,
    ._token_match = grofi_windows_mode_token_match,
    ._get_display_value = grofi_windows_mode_get_display_value,
    ._get_icon = grofi_windows_mode_get_icon,
    ._get_completion = NULL,
    ._preprocess_input = NULL,
    ._get_message = NULL,
    .private_data = NULL,
    .free = NULL,
    ._create = NULL,
    ._completer_result = NULL,
    .ed = NULL,
    .module = NULL,
    .fallback_icon_fetch_uid = 0,
    .fallback_icon_not_found = 0,
    .type = MODE_TYPE_SWITCHER,
};
