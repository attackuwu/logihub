#pragma once
#include <glib.h>

typedef enum {
  DEVICE_OK,
  DEVICE_NOT_FOUND,
  DEVICE_PERM_DENIED,
  DEVICE_IO_ERROR,
} DeviceResult;

/* Lighting effect modes (mapped internally to the device's HID++ codes). */
typedef enum {
  LIGHT_MODE_OFF       = 0,  /* lighting disabled              */
  LIGHT_MODE_STATIC    = 1,  /* single fixed colour            */
  LIGHT_MODE_BREATHING = 2,  /* fade a colour in and out       */
  LIGHT_MODE_CYCLE     = 3,  /* cycle through the colour wheel */
  LIGHT_MODE_WAVE      = 4,  /* moving rainbow wave            */
} LightMode;

/* Full description of a lighting state. */
typedef struct {
  LightMode mode;
  guint8    r, g, b;     /* colour — used by static / breathing / wave   */
  guint8    brightness;  /* 0..100                                       */
  guint16   speed;       /* effect period in ms (breathing/cycle/wave)   */
  gboolean  wave_rtl;    /* wave direction: TRUE = right-to-left          */
} LightSettings;

/* Scan /dev/hidraw* for a supported mouse and open the HID++ interface. */
DeviceResult device_open   (void);
void         device_close  (void);
gboolean     device_is_open(void);

/* Apply a full lighting configuration. */
DeviceResult device_apply  (const LightSettings *s);

/* Convenience wrappers (kept for simple on/off usage). */
DeviceResult device_lighting_on  (guint8 r, guint8 g, guint8 b);
DeviceResult device_lighting_off (void);
