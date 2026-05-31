#pragma once
#include <glib.h>

/* TRUE if our udev rule file is already installed. */
gboolean udev_rules_installed (void);

/*
 * Install udev rules via pkexec (Polkit password dialog).
 * Returns TRUE on success.
 */
gboolean udev_install_rules (GError **error);
