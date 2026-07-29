/*
 * Данный файл является частью LogiHub for Linux (https://github.com/attackuwu/logihub/)
 *
 * SPDX-License-Identifier: CC0-1.0
 *
 * Насколько это возможно по закону, авторы передали эту работу
 * в общественное достояние согласно CC0 1.0 Universal.
 */
#include "device.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <linux/hidraw.h>

#define G102_VID            ((guint16)0x046d)
#define G102_PID_LIGHTSYNC  ((guint16)0xc092)
#define G102_PID_OLD        ((guint16)0xc084)
#define G203_PID            ((guint16)0xc08b)

#define HIDPP_DEV_IDX     0xFF
#define HIDPP_SHORT       0x10
#define HIDPP_LONG        0x11
#define SW_ID             0x0E

#define HW_MODE_OFF        0x00
#define HW_MODE_STATIC     0x01
#define HW_MODE_CYCLE      0x02
#define HW_MODE_WAVE       0x03
#define HW_MODE_BREATHING  0x04

static const guint16 LIGHTING_FEATURE_CODES[] = {
  0x8070,
  0x8071,
};

#define FALLBACK_FEAT_IDX 0x0E

static int    s_fd       = -1;
static guint8 s_feat_idx = FALLBACK_FEAT_IDX;

static void enable_software_control(int fd);

#define DBG(fmt, ...) fprintf(stderr, "[logitech-hub] " fmt "\n", ##__VA_ARGS__)

static void
flush_rx(int fd)
{
  guint8 tmp[20];
  int fl = fcntl(fd, F_GETFL);
  fcntl(fd, F_SETFL, fl | O_NONBLOCK);
  while (read(fd, tmp, sizeof(tmp)) > 0) {}
  fcntl(fd, F_SETFL, fl);
}

static gboolean
hidpp_send(int fd, const guint8 *buf, gsize len)
{
  ssize_t r = write(fd, buf, len);
  if (r == (ssize_t)len) return TRUE;
  DBG("  write(%zu) failed (errno %d)", len, errno);
  return FALSE;
}

static int
read_response(int fd, guint8 *buf, gsize len, int timeout_ms)
{
  fd_set rfds;
  struct timeval tv = {timeout_ms / 1000, (timeout_ms % 1000) * 1000};
  FD_ZERO(&rfds);
  FD_SET(fd, &rfds);
  int r = select(fd + 1, &rfds, NULL, NULL, &tv);
  if (r <= 0) return 0;
  return (int)read(fd, buf, len);
}

static gboolean
sysfs_vid_pid(int idx, guint16 *vid, guint16 *pid)
{
  char path[128];
  snprintf(path, sizeof(path),
           "/sys/class/hidraw/hidraw%d/device/uevent", idx);
  FILE *f = fopen(path, "r");
  if (!f) return FALSE;

  char line[256];
  gboolean ok = FALSE;
  while (fgets(line, sizeof(line), f)) {
    unsigned bus, v, p;
    if (sscanf(line, "HID_ID=%x:%x:%x", &bus, &v, &p) == 3) {
      *vid = (guint16)v;
      *pid = (guint16)p;
      ok = TRUE;
      break;
    }
  }
  fclose(f);
  return ok;
}

static int
sysfs_interface_number(int idx)
{
  char path[256];
  snprintf(path, sizeof(path),
           "/sys/class/hidraw/hidraw%d/device/../bInterfaceNumber", idx);
  FILE *f = fopen(path, "r");
  if (!f) return -1;
  int n = -1;
  fscanf(f, "%d", &n);
  fclose(f);
  return n;
}

static gboolean
is_supported_pid(guint16 pid)
{
  return pid == G102_PID_LIGHTSYNC
      || pid == G102_PID_OLD
      || pid == G203_PID;
}

static int
iroot_get_feature(int fd, guint16 feat_code)
{
  flush_rx(fd);
  guint8 q[7] = {
    HIDPP_SHORT, HIDPP_DEV_IDX, 0x00,
    (guint8)(0x00 | SW_ID),
    (guint8)(feat_code >> 8),
    (guint8)(feat_code & 0xFF),
    0x00
  };
  if (!hidpp_send(fd, q, sizeof(q))) return -1;
  guint8 r[20] = {0};
  int n = read_response(fd, r, sizeof(r), 800);
  if (n < 5) return -1;
  if (r[2] == 0xFF && r[3] == 0x8F) return -1;
  return (int)r[4];
}

static int
enumerate_features(int fd)
{
  DBG("--- HID++ feature list ---");

  int ifs_idx = iroot_get_feature(fd, 0x0001);
  if (ifs_idx <= 0) {
    DBG("  IFeatureSet (0x0001) not found (idx=%d)", ifs_idx);
    return 0;
  }
  DBG("  IFeatureSet @ 0x%02x", ifs_idx);

  flush_rx(fd);
  guint8 qc[7] = {HIDPP_SHORT, HIDPP_DEV_IDX, (guint8)ifs_idx,
                  (guint8)(0x00 | SW_ID), 0x00, 0x00, 0x00};
  if (!hidpp_send(fd, qc, sizeof(qc))) {
    DBG("  getCount send failed");
    return 0;
  }
  guint8 rc[20] = {0};
  int nc = read_response(fd, rc, sizeof(rc), 500);
  int count = (nc >= 5) ? (int)rc[4] : 32;
  DBG("  total features: %d", count);

  int lighting_idx = 0;

  for (int i = 1; i <= count && i < 64; i++) {
    flush_rx(fd);
    guint8 qi[7] = {HIDPP_SHORT, HIDPP_DEV_IDX, (guint8)ifs_idx,
                    (guint8)((1 << 4) | SW_ID), (guint8)i, 0x00, 0x00};
    if (!hidpp_send(fd, qi, sizeof(qi))) {
      DBG("  [%02d] send failed", i);
      continue;
    }
    guint8 ri[20] = {0};
    int ni = read_response(fd, ri, sizeof(ri), 400);
    if (ni < 6) {
      DBG("  [%02d] no response", i);
      continue;
    }
    guint16 fc = ((guint16)ri[4] << 8) | ri[5];

    const char *note = "";
    for (gsize k = 0; k < G_N_ELEMENTS(LIGHTING_FEATURE_CODES); k++) {
      if (fc == LIGHTING_FEATURE_CODES[k]) {
        if (lighting_idx == 0) lighting_idx = i;
        note = " *** LIGHTING ***";
        break;
      }
    }
    DBG("  [%02d] 0x%04x%s", i, fc, note);
  }

  DBG("--- end feature list ---");
  return lighting_idx;
}

DeviceResult
device_open(void)
{
  gboolean saw_eperm    = FALSE;
  int      fallback_fd  = -1;

  for (int pass = 0; pass < 2; pass++) {
    for (int i = 0; i < 32; i++) {
      guint16 vid, pid;
      if (!sysfs_vid_pid(i, &vid, &pid)) continue;
      if (vid != G102_VID || !is_supported_pid(pid)) continue;

      int ifnum = sysfs_interface_number(i);
      DBG("pass %d: hidraw%d  vid=%04x pid=%04x  interface=%d",
          pass, i, vid, pid, ifnum);

      if (pass == 0 && ifnum != 1) continue;

      char path[32];
      snprintf(path, sizeof(path), "/dev/hidraw%d", i);

      int fd = open(path, O_RDWR);
      if (fd < 0) {
        DBG("  open failed (errno %d)", errno);
        if (errno == EACCES) saw_eperm = TRUE;
        continue;
      }
      DBG("  opened fd=%d", fd);

      int ifs_check = iroot_get_feature(fd, 0x0001);
      if (ifs_check < 0) {
        DBG("  not an HID++ interface, skipping");
        close(fd);
        continue;
      }

      int lighting_idx = enumerate_features(fd);

      if (lighting_idx > 0) {
        if (fallback_fd >= 0) { close(fallback_fd); fallback_fd = -1; }
        s_feat_idx = (guint8)lighting_idx;
        s_fd = fd;
        DBG("connected — feat_idx=0x%02x", s_feat_idx);
        enable_software_control(s_fd);
        return DEVICE_OK;
      }

      if (fallback_fd < 0) {
        DBG("  no lighting feature found — saving as fallback fd=%d", fd);
        fallback_fd = fd;
      } else {
        close(fd);
      }
    }
  }

  if (fallback_fd >= 0) {
    s_feat_idx = FALLBACK_FEAT_IDX;
    s_fd = fallback_fd;
    DBG("using fallback fd=%d feat_idx=0x%02x", s_fd, s_feat_idx);
    enable_software_control(s_fd);
    return DEVICE_OK;
  }

  return saw_eperm ? DEVICE_PERM_DENIED : DEVICE_NOT_FOUND;
}

void
device_close(void)
{
  if (s_fd >= 0) { close(s_fd); s_fd = -1; }
}

gboolean
device_is_open(void)
{
  return s_fd >= 0;
}

static guint8
scale8(guint8 value, guint8 percent)
{
  return (guint8)(((int)value * percent) / 100);
}

static DeviceResult
send_packet(int fd, const guint8 *buf)
{
  DBG("  tx: %02x %02x %02x %02x | %02x %02x %02x %02x %02x | "
      "%02x %02x %02x %02x %02x %02x %02x | end=%02x",
      buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7],
      buf[8], buf[9], buf[10], buf[11], buf[12], buf[13], buf[14], buf[15],
      buf[16]);

  flush_rx(fd);
  ssize_t ret = write(fd, buf, 20);
  if (ret != 20) {
    DBG("  write failed (ret=%zd errno=%d)", ret, errno);
    return DEVICE_IO_ERROR;
  }

  guint8 resp[20] = {0};
  read_response(fd, resp, sizeof(resp), 200);
  return DEVICE_OK;
}

static void
enable_software_control(int fd)
{
  guint8 buf[20] = {0};
  buf[0x00] = HIDPP_LONG;
  buf[0x01] = HIDPP_DEV_IDX;
  buf[0x02] = s_feat_idx;
  buf[0x03] = 0x50;
  buf[0x04] = 0x01;
  buf[0x05] = 0x03;
  buf[0x06] = 0x07;

  DBG("enable software control: tx %02x %02x %02x %02x %02x %02x %02x",
      buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6]);

  flush_rx(fd);
  if (write(fd, buf, sizeof(buf)) != (ssize_t)sizeof(buf))
    DBG("  enable sw control write failed (errno %d)", errno);

  guint8 resp[20] = {0};
  read_response(fd, resp, sizeof(resp), 200);
}

DeviceResult
device_apply(const LightSettings *s)
{
  if (s_fd < 0) return DEVICE_NOT_FOUND;
  if (!s)       return DEVICE_IO_ERROR;

  guint8  bright = s->brightness > 100 ? 100 : s->brightness;
  guint8  bbyte  = bright == 0 ? 1 : bright;
  guint16 speed  = s->speed;
  guint8  sp_hi  = (guint8)(speed >> 8);
  guint8  sp_lo  = (guint8)(speed & 0xFF);

  guint8 buf[20] = {0};
  buf[0x00] = HIDPP_LONG;
  buf[0x01] = HIDPP_DEV_IDX;
  buf[0x02] = s_feat_idx;
  buf[0x03] = 0x10;
  buf[0x04] = 0x00;

  switch (s->mode) {
    case LIGHT_MODE_OFF:
      buf[0x05] = HW_MODE_OFF;
      break;

    case LIGHT_MODE_STATIC:
      buf[0x05] = HW_MODE_STATIC;
      buf[0x06] = scale8(s->r, bright);
      buf[0x07] = scale8(s->g, bright);
      buf[0x08] = scale8(s->b, bright);
      buf[0x09] = 0x02;
      break;

    case LIGHT_MODE_BREATHING:
      buf[0x05] = HW_MODE_BREATHING;
      buf[0x06] = s->r;
      buf[0x07] = s->g;
      buf[0x08] = s->b;
      buf[0x09] = sp_hi;
      buf[0x0A] = sp_lo;
      buf[0x0C] = bbyte;
      break;

    case LIGHT_MODE_CYCLE:
      buf[0x05] = HW_MODE_CYCLE;
      buf[0x0B] = sp_hi;
      buf[0x0C] = sp_lo;
      buf[0x0D] = bbyte;
      break;

    case LIGHT_MODE_WAVE:
      buf[0x05] = HW_MODE_WAVE;
      buf[0x0C] = sp_lo;
      buf[0x0D] = s->wave_rtl ? 0x06 : 0x01;
      buf[0x0E] = bbyte;
      buf[0x0F] = sp_hi;
      break;

    default:
      return DEVICE_IO_ERROR;
  }

  buf[0x10] = 0x01;

  DBG("apply mode=%d bright=%d speed=%d", s->mode, bright, speed);
  DeviceResult res = send_packet(s_fd, buf);
  DBG("apply → %s", res == DEVICE_OK ? "OK" : "FAIL");
  return res;
}

DeviceResult
device_lighting_on(guint8 r, guint8 g, guint8 b)
{
  LightSettings s = {
    .mode = LIGHT_MODE_STATIC, .r = r, .g = g, .b = b,
    .brightness = 100, .speed = 0, .wave_rtl = FALSE,
  };
  return device_apply(&s);
}

DeviceResult
device_lighting_off(void)
{
  LightSettings s = { .mode = LIGHT_MODE_OFF, .brightness = 100 };
  return device_apply(&s);
}
