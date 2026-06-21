/*
 * Данный файл является частью LogiHub for Linux (https://github.com/attackuwu/logihub/)
 *
 * Copyright (C) 2026  attackuwu и участники
 *
 * Это свободное программное обеспечение: вы можете распространять и/или изменять
 * его на условиях GNU General Public License версии 2, опубликованной
 * Free Software Foundation.
 *
 * При нарушении условий лицензии GPL-2.0 дело будет направлено в
 * Free Software Foundation (FSF) и GitHub.
 */

#pragma once
#include <glib.h>

gboolean udev_rules_installed (void);
gboolean udev_install_rules (GError **error);
