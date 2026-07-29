/*
 * Данный файл является частью LogiHub for Linux (https://github.com/attackuwu/logihub/)
 *
 * SPDX-License-Identifier: CC0-1.0
 *
 * Насколько это возможно по закону, авторы передали эту работу
 * в общественное достояние согласно CC0 1.0 Universal.
 */

#pragma once
#include <glib.h>

gboolean udev_rules_installed (void);
gboolean udev_install_rules (GError **error);
