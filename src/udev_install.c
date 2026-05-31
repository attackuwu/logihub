/*
 * Install bundled udev rules so /dev/hidraw* is accessible without root.
 * Looks for udev/99-logitech-hub.rules next to the executable (release layout)
 * or one directory up (meson build layout: build/logihub → ../udev/…).
 */

#include "udev_install.h"

#include <errno.h>
#include <glib/gstdio.h>
#include <limits.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>

#define UDEV_DEST "/etc/udev/rules.d/99-logitech-hub.rules"
#define RULES_NAME "99-logitech-hub.rules"

static gboolean
file_readable(const char *path)
{
  return access(path, R_OK) == 0;
}

static gboolean
find_rules_source(char *out, gsize out_len)
{
  GError *err = NULL;
  gchar *exe = g_file_read_link("/proc/self/exe", &err);
  if (!exe) {
    if (err) g_error_free(err);
    return FALSE;
  }

  gchar *dir = g_path_get_dirname(exe);
  g_free(exe);

  g_snprintf(out, out_len, "%s/udev/%s", dir, RULES_NAME);
  if (file_readable(out)) {
    g_free(dir);
    return TRUE;
  }

  g_snprintf(out, out_len, "%s/../udev/%s", dir, RULES_NAME);
  g_free(dir);
  return file_readable(out);
}

gboolean
udev_rules_installed(void)
{
  struct stat st;
  return stat(UDEV_DEST, &st) == 0;
}

gboolean
udev_install_rules(GError **error)
{
  char src[PATH_MAX];
  if (!find_rules_source(src, sizeof(src))) {
    g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_NOENT,
                "Не найден файл udev/%s рядом с программой", RULES_NAME);
    return FALSE;
  }

  gchar *shell_cmd = g_strdup_printf(
      "cp '%s' '%s' && "
      "udevadm control --reload-rules && "
      "udevadm trigger --action=add --subsystem-match=hidraw",
      src, UDEV_DEST);

  gchar *argv[] = {"pkexec", "sh", "-c", shell_cmd, NULL};

  int status = 0;
  GError *spawn_err = NULL;
  if (!g_spawn_sync(NULL, argv, NULL,
                    G_SPAWN_SEARCH_PATH,
                    NULL, NULL, NULL, NULL, &status, &spawn_err)) {
    g_free(shell_cmd);
    if (spawn_err) {
      g_propagate_error(error, spawn_err);
    } else {
      g_set_error(error, G_SPAWN_ERROR, G_SPAWN_ERROR_FAILED,
                  "Не удалось запустить pkexec");
    }
    return FALSE;
  }
  g_free(shell_cmd);

  if (spawn_err) {
    g_propagate_error(error, spawn_err);
    return FALSE;
  }

  if (!g_spawn_check_wait_status(status, error)) {
    return FALSE;
  }

  return udev_rules_installed();
}
