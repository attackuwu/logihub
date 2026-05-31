#pragma once
#include <glib.h>

gboolean udev_rules_installed (void);
gboolean udev_install_rules (GError **error);
