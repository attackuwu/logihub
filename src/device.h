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

typedef enum {
  DEVICE_OK,
  DEVICE_NOT_FOUND,
  DEVICE_PERM_DENIED,
  DEVICE_IO_ERROR,
} DeviceResult;

typedef enum {
  LIGHT_MODE_OFF       = 0,
  LIGHT_MODE_STATIC    = 1,
  LIGHT_MODE_BREATHING = 2,
  LIGHT_MODE_CYCLE     = 3,
  LIGHT_MODE_WAVE      = 4,
} LightMode;

typedef struct {
  LightMode mode;
  guint8    r, g, b;
  guint8    brightness;
  guint16   speed;
  gboolean  wave_rtl;
} LightSettings;

DeviceResult device_open   (void);
void         device_close  (void);
gboolean     device_is_open(void);

DeviceResult device_apply  (const LightSettings *s);

DeviceResult device_lighting_on  (guint8 r, guint8 g, guint8 b);
DeviceResult device_lighting_off (void);
