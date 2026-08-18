/*
 * Данный файл является частью LogiHub for Linux (https://github.com/attackuwu/logihub/)
 *
 * SPDX-License-Identifier: CC0-1.0
 *
 * Насколько это возможно по закону, авторы передали эту работу
 * в общественное достояние согласно CC0 1.0 Universal.
 */

#include "udev_install.h"

#include <errno.h>
#include <sys/stat.h>

#define UDEV_DEST "/etc/udev/rules.d/99-logitech-hub.rules"

static const char UDEV_RULES[] =
  "# Logitech G102/G203 LIGHTSYNC access rules\n"
  "SUBSYSTEM==\"hidraw\", ATTRS{idVendor}==\"046d\", ATTRS{idProduct}==\"c092\", MODE=\"0666\"\n"
  "SUBSYSTEM==\"hidraw\", ATTRS{idVendor}==\"046d\", ATTRS{idProduct}==\"c084\", MODE=\"0666\"\n"
  "SUBSYSTEM==\"hidraw\", ATTRS{idVendor}==\"046d\", ATTRS{idProduct}==\"c08b\", MODE=\"0666\"\n";

gboolean
udev_rules_installed(void)
{
  struct stat st;
  return stat(UDEV_DEST, &st) == 0;
}

gboolean
udev_install_rules(GError **error)
{
  gchar *quoted_rules = g_shell_quote(UDEV_RULES);
  gchar *shell_cmd = g_strdup_printf(
      "printf '%%s' %s > '%s' && "
      "udevadm control --reload-rules && "
      "udevadm trigger --action=add --subsystem-match=hidraw",
      quoted_rules, UDEV_DEST);
  g_free(quoted_rules);

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
