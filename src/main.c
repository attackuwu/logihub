/*
 * Данный файл является частью LogiHub for Linux (https://github.com/attackuwu/logihub/)
 *
 * SPDX-License-Identifier: CC0-1.0
 *
 * Насколько это возможно по закону, авторы передали эту работу
 * в общественное достояние согласно CC0 1.0 Universal.
 */

#include <gtk/gtk.h>
#include <math.h>
#include "device.h"
#include "udev_install.h"

#define WHEEL_SIZE  240
#define WHEEL_CX    120.0
#define WHEEL_CY    120.0
#define R_OUTER     116.0
#define R_INNER     92.0
#define SQ_SIDE     124
#define SQ_X0       (WHEEL_CX - SQ_SIDE / 2.0)
#define SQ_Y0       (WHEEL_CY - SQ_SIDE / 2.0)

enum { REGION_NONE = 0, REGION_RING, REGION_SQUARE };

typedef struct {
  GtkWidget *indicator;
  GtkWidget *state_label;

  GtkWidget *mode_drop;
  GtkWidget *wheel_area;
  GtkWidget *bright_scale;
  GtkWidget *speed_scale;
  GtkWidget *dir_switch;

  GtkWidget *color_section;
  GtkWidget *speed_row;
  GtkWidget *dir_row;

  GtkWidget *off_button;
  GtkWidget *retry_button;

  gboolean   connected;
  gboolean   suppress;

  double     hue, sat, val;

  int        drag_region;
  double     drag_start_x, drag_start_y;
} AppWidgets;

static const char *APP_CSS =
  "window.app-window {"
  "  background-color: #0b0b14;"
  "}"

  ".device-card {"
  "  background-color: #13131f;"
  "  border-radius: 24px;"
  "  padding: 32px 34px 28px 34px;"
  "  border: 1px solid rgba(255,255,255,0.06);"
  "}"

  ".mouse-icon {"
  "  color: #3b82f6;"
  "}"

  ".device-name {"
  "  font-size: 24px;"
  "  font-weight: 800;"
  "  color: #e8e8ff;"
  "  letter-spacing: -0.5px;"
  "}"

  ".device-sub {"
  "  font-size: 11px;"
  "  font-weight: 600;"
  "  letter-spacing: 2px;"
  "  color: #3b82f6;"
  "  margin-bottom: 16px;"
  "}"

  ".indicator-dot {"
  "  border-radius: 999px;"
  "  min-width: 10px;"
  "  min-height: 10px;"
  "}"

  ".indicator-on {"
  "  background-color: #60a5fa;"
  "  box-shadow: 0 0 10px 3px rgba(96,165,250,0.7);"
  "}"

  ".indicator-off {"
  "  background-color: #333355;"
  "}"

  ".indicator-error {"
  "  background-color: #f87171;"
  "  box-shadow: 0 0 8px 2px rgba(248,113,113,0.5);"
  "}"

  ".state-label {"
  "  font-size: 13px;"
  "  font-weight: 600;"
  "  color: #9999bb;"
  "  margin-left: 8px;"
  "}"

  ".status-row {"
  "  margin-bottom: 20px;"
  "}"

  ".section-sep {"
  "  background-color: rgba(255,255,255,0.06);"
  "  min-height: 1px;"
  "  margin: 6px 0 14px 0;"
  "}"

  ".ctl-label {"
  "  font-size: 13px;"
  "  font-weight: 600;"
  "  color: #c7c7e6;"
  "}"

  ".ctl-row {"
  "  margin-bottom: 14px;"
  "}"

  "dropdown {"
  "  background-color: #1c1c2e;"
  "  border-radius: 10px;"
  "  border: 1px solid rgba(255,255,255,0.08);"
  "  color: #e8e8ff;"
  "  min-height: 34px;"
  "}"

  "dropdown button {"
  "  background: transparent;"
  "  border: none;"
  "  box-shadow: none;"
  "  color: #e8e8ff;"
  "}"

  "scale { min-width: 150px; }"

  "scale highlight {"
  "  background-color: #3b82f6;"
  "  border-radius: 999px;"
  "}"

  "scale trough {"
  "  background-color: #23233a;"
  "  border-radius: 999px;"
  "  min-height: 6px;"
  "}"

  "scale slider {"
  "  background-color: #e8e8ff;"
  "  border-radius: 999px;"
  "  min-width: 16px;"
  "  min-height: 16px;"
  "  box-shadow: 0 1px 4px rgba(0,0,0,0.4);"
  "}"

  "button.btn-off {"
  "  background-color: transparent;"
  "  border-radius: 14px;"
  "  border: 1.5px solid rgba(255,255,255,0.10);"
  "  outline: none;"
  "  box-shadow: none;"
  "  padding: 14px 0;"
  "  min-width: 200px;"
  "}"

  "button.btn-off label {"
  "  color: #b9b9d8;"
  "  font-size: 14px;"
  "  font-weight: 700;"
  "}"

  "button.btn-off:hover {"
  "  background-color: rgba(255,255,255,0.05);"
  "  border-color: rgba(255,255,255,0.18);"
  "}"

  "button.btn-off:disabled label {"
  "  color: rgba(119,119,170,0.25);"
  "}"

  "button.btn-retry {"
  "  background-color: transparent;"
  "  border: none;"
  "  outline: none;"
  "  box-shadow: none;"
  "  padding: 8px 20px;"
  "  margin-top: 6px;"
  "}"

  "button.btn-retry label {"
  "  color: #555577;"
  "  font-size: 13px;"
  "  text-decoration: underline;"
  "}"

  "button.btn-retry:hover label {"
  "  color: #7777aa;"
  "}";

static void
hsv_to_rgb(double h, double s, double v, double *r, double *g, double *b)
{
  if (s <= 0.0) { *r = *g = *b = v; return; }
  h = fmod(h, 1.0);
  if (h < 0) h += 1.0;
  h *= 6.0;
  int    i = (int)h;
  double f = h - i;
  double p = v * (1.0 - s);
  double q = v * (1.0 - s * f);
  double t = v * (1.0 - s * (1.0 - f));
  switch (i) {
    case 0:  *r = v; *g = t; *b = p; break;
    case 1:  *r = q; *g = v; *b = p; break;
    case 2:  *r = p; *g = v; *b = t; break;
    case 3:  *r = p; *g = q; *b = v; break;
    case 4:  *r = t; *g = p; *b = v; break;
    default: *r = v; *g = p; *b = q; break;
  }
}

static double
clamp01(double x)
{
  if (x < 0.0) return 0.0;
  if (x > 1.0) return 1.0;
  return x;
}

static void
clear_indicator_classes(GtkWidget *ind)
{
  gtk_widget_remove_css_class(ind, "indicator-on");
  gtk_widget_remove_css_class(ind, "indicator-off");
  gtk_widget_remove_css_class(ind, "indicator-error");
}

static guint16
slider_to_speed_ms(double slider)
{
  double ms = 20000.0 - (slider / 100.0) * (20000.0 - 1000.0);
  if (ms < 1000.0)  ms = 1000.0;
  if (ms > 20000.0) ms = 20000.0;
  return (guint16)ms;
}

static void
collect_settings(AppWidgets *w, LightSettings *s)
{
  guint sel = gtk_drop_down_get_selected(GTK_DROP_DOWN(w->mode_drop));
  switch (sel) {
    case 0:  s->mode = LIGHT_MODE_STATIC;    break;
    case 1:  s->mode = LIGHT_MODE_BREATHING; break;
    case 2:  s->mode = LIGHT_MODE_CYCLE;     break;
    case 3:  s->mode = LIGHT_MODE_WAVE;      break;
    default: s->mode = LIGHT_MODE_STATIC;    break;
  }

  double r, g, b;
  hsv_to_rgb(w->hue, w->sat, w->val, &r, &g, &b);
  s->r = (guint8)(r * 255.0 + 0.5);
  s->g = (guint8)(g * 255.0 + 0.5);
  s->b = (guint8)(b * 255.0 + 0.5);

  s->brightness = (guint8)gtk_range_get_value(GTK_RANGE(w->bright_scale));
  s->speed      = slider_to_speed_ms(gtk_range_get_value(GTK_RANGE(w->speed_scale)));
  s->wave_rtl   = gtk_switch_get_active(GTK_SWITCH(w->dir_switch));
}

static void
update_control_visibility(AppWidgets *w)
{
  guint sel = gtk_drop_down_get_selected(GTK_DROP_DOWN(w->mode_drop));
  gboolean uses_color = (sel == 0 || sel == 1);
  gboolean uses_speed = (sel == 1 || sel == 2 || sel == 3);
  gboolean uses_dir   = (sel == 3);

  gtk_widget_set_visible(w->color_section, uses_color);
  gtk_widget_set_visible(w->speed_row,     uses_speed);
  gtk_widget_set_visible(w->dir_row,       uses_dir);
}

static void
set_status(AppWidgets *w, const char *text, gboolean on)
{
  clear_indicator_classes(w->indicator);
  gtk_widget_add_css_class(w->indicator, on ? "indicator-on" : "indicator-off");
  gtk_label_set_text(GTK_LABEL(w->state_label), text);
}

static void
apply_now(AppWidgets *w)
{
  if (!w->connected || w->suppress) return;

  LightSettings s;
  collect_settings(w, &s);
  if (device_apply(&s) == DEVICE_OK)
    set_status(w, "Подсветка обновлена", TRUE);
  else
    set_status(w, "Ошибка команды", FALSE);
}

static void
set_controls_enabled(AppWidgets *w, gboolean enabled)
{
  gtk_widget_set_sensitive(w->mode_drop,    enabled);
  gtk_widget_set_sensitive(w->wheel_area,   enabled);
  gtk_widget_set_sensitive(w->bright_scale, enabled);
  gtk_widget_set_sensitive(w->speed_scale,  enabled);
  gtk_widget_set_sensitive(w->dir_switch,   enabled);
  gtk_widget_set_sensitive(w->off_button,   enabled);
}

static void
set_device_error(AppWidgets *w, DeviceResult r)
{
  w->connected = FALSE;
  clear_indicator_classes(w->indicator);
  gtk_widget_add_css_class(w->indicator, "indicator-error");
  set_controls_enabled(w, FALSE);
  gtk_widget_set_visible(w->retry_button, TRUE);

  switch (r) {
    case DEVICE_NOT_FOUND:
      gtk_label_set_text(GTK_LABEL(w->state_label), "Устройство не найдено");
      break;
    case DEVICE_PERM_DENIED:
      gtk_label_set_text(GTK_LABEL(w->state_label),
                         "Нет доступа — переподключите мышь");
      break;
    default:
      gtk_label_set_text(GTK_LABEL(w->state_label), "Ошибка устройства");
      break;
  }
}

static void
draw_wheel(GtkDrawingArea *area, cairo_t *cr, int width, int height, gpointer data)
{
  (void)area; (void)width; (void)height;
  AppWidgets *w = data;

  const int seg = 360;
  for (int i = 0; i < seg; i++) {
    double a0 = (double)i       / seg * 2.0 * G_PI;
    double a1 = (double)(i + 1) / seg * 2.0 * G_PI;
    double r, g, b;
    hsv_to_rgb((double)i / seg, 1.0, 1.0, &r, &g, &b);
    cairo_set_source_rgb(cr, r, g, b);
    cairo_new_path(cr);
    cairo_arc(cr, WHEEL_CX, WHEEL_CY, R_OUTER, a0, a1 + 0.012);
    cairo_arc_negative(cr, WHEEL_CX, WHEEL_CY, R_INNER, a1 + 0.012, a0);
    cairo_close_path(cr);
    cairo_fill(cr);
  }

  cairo_surface_t *surf =
      cairo_image_surface_create(CAIRO_FORMAT_RGB24, SQ_SIDE, SQ_SIDE);
  unsigned char *pix = cairo_image_surface_get_data(surf);
  int stride = cairo_image_surface_get_stride(surf);
  for (int y = 0; y < SQ_SIDE; y++) {
    guint32 *row = (guint32 *)(pix + y * stride);
    double v = 1.0 - (double)y / (SQ_SIDE - 1);
    for (int x = 0; x < SQ_SIDE; x++) {
      double s = (double)x / (SQ_SIDE - 1);
      double r, g, b;
      hsv_to_rgb(w->hue, s, v, &r, &g, &b);
      guint32 R = (guint32)(r * 255.0 + 0.5);
      guint32 G = (guint32)(g * 255.0 + 0.5);
      guint32 B = (guint32)(b * 255.0 + 0.5);
      row[x] = (R << 16) | (G << 8) | B;
    }
  }
  cairo_surface_mark_dirty(surf);
  cairo_set_source_surface(cr, surf, SQ_X0, SQ_Y0);
  cairo_paint(cr);
  cairo_surface_destroy(surf);

  cairo_set_line_width(cr, 1.0);
  cairo_set_source_rgba(cr, 1, 1, 1, 0.18);
  cairo_rectangle(cr, SQ_X0 + 0.5, SQ_Y0 + 0.5, SQ_SIDE - 1, SQ_SIDE - 1);
  cairo_stroke(cr);

  double ang = w->hue * 2.0 * G_PI;
  double mr  = (R_OUTER + R_INNER) / 2.0;
  double hx  = WHEEL_CX + cos(ang) * mr;
  double hy  = WHEEL_CY + sin(ang) * mr;
  cairo_set_line_width(cr, 3.0);
  cairo_set_source_rgb(cr, 1, 1, 1);
  cairo_arc(cr, hx, hy, 9.0, 0, 2 * G_PI);
  cairo_stroke(cr);
  cairo_set_source_rgba(cr, 0, 0, 0, 0.35);
  cairo_arc(cr, hx, hy, 11.0, 0, 2 * G_PI);
  cairo_set_line_width(cr, 1.0);
  cairo_stroke(cr);

  double sx = SQ_X0 + w->sat * (SQ_SIDE - 1);
  double sy = SQ_Y0 + (1.0 - w->val) * (SQ_SIDE - 1);
  double cr_, cg_, cb_;
  hsv_to_rgb(w->hue, w->sat, w->val, &cr_, &cg_, &cb_);
  cairo_set_source_rgb(cr, cr_, cg_, cb_);
  cairo_arc(cr, sx, sy, 7.0, 0, 2 * G_PI);
  cairo_fill(cr);
  cairo_set_line_width(cr, 2.5);
  cairo_set_source_rgb(cr, 1, 1, 1);
  cairo_arc(cr, sx, sy, 7.0, 0, 2 * G_PI);
  cairo_stroke(cr);
}

static void
wheel_set_from_point(AppWidgets *w, double x, double y)
{
  if (w->drag_region == REGION_RING) {
    double ang = atan2(y - WHEEL_CY, x - WHEEL_CX);
    if (ang < 0) ang += 2.0 * G_PI;
    w->hue = ang / (2.0 * G_PI);
  } else if (w->drag_region == REGION_SQUARE) {
    w->sat = clamp01((x - SQ_X0) / (SQ_SIDE - 1));
    w->val = clamp01(1.0 - (y - SQ_Y0) / (SQ_SIDE - 1));
  } else {
    return;
  }
  gtk_widget_queue_draw(w->wheel_area);
  apply_now(w);
}

static void
on_wheel_drag_begin(GtkGestureDrag *g, double x, double y, gpointer data)
{
  (void)g;
  AppWidgets *w = data;
  w->drag_start_x = x;
  w->drag_start_y = y;

  gboolean in_square =
      (x >= SQ_X0 && x <= SQ_X0 + SQ_SIDE &&
       y >= SQ_Y0 && y <= SQ_Y0 + SQ_SIDE);

  w->drag_region = in_square ? REGION_SQUARE : REGION_RING;
  wheel_set_from_point(w, x, y);
}

static void
on_wheel_drag_update(GtkGestureDrag *g, double ox, double oy, gpointer data)
{
  (void)g;
  AppWidgets *w = data;
  wheel_set_from_point(w, w->drag_start_x + ox, w->drag_start_y + oy);
}

static void
on_mode_changed(GObject *obj, GParamSpec *pspec, gpointer data)
{
  (void)obj; (void)pspec;
  AppWidgets *w = data;
  update_control_visibility(w);
  apply_now(w);
}

static void
on_scale_changed(GtkRange *range, gpointer data)
{
  (void)range;
  apply_now((AppWidgets *)data);
}

static gboolean
on_dir_toggled(GtkSwitch *sw, gboolean state, gpointer data)
{
  (void)sw; (void)state;
  apply_now((AppWidgets *)data);
  return FALSE;
}

static void
on_turn_off(GtkButton *btn, gpointer data)
{
  (void)btn;
  AppWidgets *w = data;
  if (device_lighting_off() == DEVICE_OK)
    set_status(w, "Подсветка выключена", FALSE);
  else
    set_status(w, "Ошибка команды", FALSE);
}

static DeviceResult
connect_device(AppWidgets *w)
{
  DeviceResult r = device_open();

  if (r == DEVICE_PERM_DENIED) {
    GError *err = NULL;
    gtk_label_set_text(GTK_LABEL(w->state_label),
                       "Установка правил udev…");
    if (udev_install_rules(&err)) {
      g_usleep(500000);
      device_close();
      r = device_open();
    } else if (err) {
      g_warning("udev: %s", err->message);
      g_error_free(err);
    }
  }

  return r;
}

static void
on_connected(AppWidgets *w)
{
  w->connected = TRUE;
  set_controls_enabled(w, TRUE);
  gtk_widget_set_visible(w->retry_button, FALSE);
  update_control_visibility(w);
  set_status(w, "Подключено", TRUE);
  apply_now(w);
}

static void
on_retry(GtkButton *btn, gpointer data)
{
  (void)btn;
  AppWidgets *w = data;
  device_close();
  DeviceResult r = connect_device(w);
  if (r == DEVICE_OK)
    on_connected(w);
  else
    set_device_error(w, r);
}

static GtkWidget *
make_row(const char *label_text, GtkWidget *control)
{
  GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_widget_add_css_class(row, "ctl-row");

  GtkWidget *label = gtk_label_new(label_text);
  gtk_widget_add_css_class(label, "ctl-label");
  gtk_widget_set_halign(label, GTK_ALIGN_START);
  gtk_widget_set_hexpand(label, TRUE);
  gtk_label_set_xalign(GTK_LABEL(label), 0.0);

  gtk_widget_set_halign(control, GTK_ALIGN_END);

  gtk_box_append(GTK_BOX(row), label);
  gtk_box_append(GTK_BOX(row), control);
  return row;
}

static void
activate(GtkApplication *app, gpointer user_data)
{
  (void)user_data;

  GtkCssProvider *provider = gtk_css_provider_new();
  gtk_css_provider_load_from_string(provider, APP_CSS);
  gtk_style_context_add_provider_for_display(
      gdk_display_get_default(),
      GTK_STYLE_PROVIDER(provider),
      GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  g_object_unref(provider);

  AppWidgets *w = g_new0(AppWidgets, 1);
  w->suppress = TRUE;
  w->hue = 0.6667;
  w->sat = 1.0;
  w->val = 1.0;

  GtkWidget *window = gtk_application_window_new(app);
  gtk_window_set_title(GTK_WINDOW(window), "Logihub for Linux");
  gtk_window_set_default_size(GTK_WINDOW(window), 460, 760);
  gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
  gtk_widget_add_css_class(window, "app-window");

  GtkWidget *outer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_set_margin_top(outer, 26);
  gtk_widget_set_margin_bottom(outer, 26);
  gtk_widget_set_margin_start(outer, 26);
  gtk_widget_set_margin_end(outer, 26);

  GtkWidget *card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_add_css_class(card, "device-card");
  gtk_widget_set_hexpand(card, TRUE);
  gtk_widget_set_vexpand(card, TRUE);

  GtkWidget *icon = gtk_image_new_from_icon_name("input-mouse-symbolic");
  gtk_image_set_pixel_size(GTK_IMAGE(icon), 44);
  gtk_widget_add_css_class(icon, "mouse-icon");
  gtk_widget_set_halign(icon, GTK_ALIGN_CENTER);

  GtkWidget *name_label = gtk_label_new("Logitech G102");
  gtk_widget_add_css_class(name_label, "device-name");
  gtk_widget_set_halign(name_label, GTK_ALIGN_CENTER);
  gtk_widget_set_margin_top(name_label, 6);

  GtkWidget *sub_label = gtk_label_new("LIGHTSYNC RGB");
  gtk_widget_add_css_class(sub_label, "device-sub");
  gtk_widget_set_halign(sub_label, GTK_ALIGN_CENTER);
  gtk_widget_set_margin_top(sub_label, 2);

  GtkWidget *status_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_add_css_class(status_row, "status-row");
  gtk_widget_set_halign(status_row, GTK_ALIGN_CENTER);

  GtkWidget *indicator = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_add_css_class(indicator, "indicator-dot");
  gtk_widget_set_valign(indicator, GTK_ALIGN_CENTER);

  GtkWidget *state_label = gtk_label_new("Подключение...");
  gtk_widget_add_css_class(state_label, "state-label");

  gtk_box_append(GTK_BOX(status_row), indicator);
  gtk_box_append(GTK_BOX(status_row), state_label);

  GtkWidget *sep = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_add_css_class(sep, "section-sep");

  const char *const modes[] = {
    "Статичный", "Дыхание", "Перелив цветов", "Волна", NULL,
  };
  GtkWidget *mode_drop = gtk_drop_down_new_from_strings(modes);
  GtkWidget *mode_row  = make_row("Эффект", mode_drop);

  GtkWidget *color_section = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  gtk_widget_add_css_class(color_section, "ctl-row");

  GtkWidget *color_label = gtk_label_new("Цвет");
  gtk_widget_add_css_class(color_label, "ctl-label");
  gtk_widget_set_halign(color_label, GTK_ALIGN_START);
  gtk_label_set_xalign(GTK_LABEL(color_label), 0.0);

  GtkWidget *wheel_area = gtk_drawing_area_new();
  gtk_widget_set_size_request(wheel_area, WHEEL_SIZE, WHEEL_SIZE);
  gtk_widget_set_halign(wheel_area, GTK_ALIGN_CENTER);
  gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(wheel_area),
                                 draw_wheel, w, NULL);

  GtkGesture *drag = gtk_gesture_drag_new();
  gtk_widget_add_controller(wheel_area, GTK_EVENT_CONTROLLER(drag));
  g_signal_connect(drag, "drag-begin",  G_CALLBACK(on_wheel_drag_begin),  w);
  g_signal_connect(drag, "drag-update", G_CALLBACK(on_wheel_drag_update), w);

  gtk_box_append(GTK_BOX(color_section), color_label);
  gtk_box_append(GTK_BOX(color_section), wheel_area);

  GtkWidget *bright_scale =
      gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
  gtk_range_set_value(GTK_RANGE(bright_scale), 100);
  gtk_widget_set_size_request(bright_scale, 180, -1);

  GtkWidget *speed_scale =
      gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
  gtk_range_set_value(GTK_RANGE(speed_scale), 50);
  gtk_widget_set_size_request(speed_scale, 180, -1);

  GtkWidget *dir_switch = gtk_switch_new();
  gtk_widget_set_valign(dir_switch, GTK_ALIGN_CENTER);

  GtkWidget *bright_row = make_row("Яркость",        bright_scale);
  GtkWidget *speed_row  = make_row("Скорость",       speed_scale);
  GtkWidget *dir_row    = make_row("Обратная волна", dir_switch);

  GtkWidget *off_button = gtk_button_new_with_label("Выключить подсветку");
  gtk_widget_add_css_class(off_button, "btn-off");
  gtk_widget_set_halign(off_button, GTK_ALIGN_CENTER);
  gtk_widget_set_margin_top(off_button, 6);

  GtkWidget *retry_button = gtk_button_new_with_label("Переподключиться");
  gtk_widget_add_css_class(retry_button, "btn-retry");
  gtk_widget_set_halign(retry_button, GTK_ALIGN_CENTER);

  gtk_box_append(GTK_BOX(card), icon);
  gtk_box_append(GTK_BOX(card), name_label);
  gtk_box_append(GTK_BOX(card), sub_label);
  gtk_box_append(GTK_BOX(card), status_row);
  gtk_box_append(GTK_BOX(card), sep);
  gtk_box_append(GTK_BOX(card), mode_row);
  gtk_box_append(GTK_BOX(card), color_section);
  gtk_box_append(GTK_BOX(card), bright_row);
  gtk_box_append(GTK_BOX(card), speed_row);
  gtk_box_append(GTK_BOX(card), dir_row);
  gtk_box_append(GTK_BOX(card), off_button);
  gtk_box_append(GTK_BOX(card), retry_button);

  gtk_box_append(GTK_BOX(outer), card);
  gtk_window_set_child(GTK_WINDOW(window), outer);

  w->indicator     = indicator;
  w->state_label   = state_label;
  w->mode_drop     = mode_drop;
  w->wheel_area    = wheel_area;
  w->bright_scale  = bright_scale;
  w->speed_scale   = speed_scale;
  w->dir_switch    = dir_switch;
  w->color_section = color_section;
  w->speed_row     = speed_row;
  w->dir_row       = dir_row;
  w->off_button    = off_button;
  w->retry_button  = retry_button;

  g_signal_connect(mode_drop,    "notify::selected", G_CALLBACK(on_mode_changed), w);
  g_signal_connect(bright_scale, "value-changed",    G_CALLBACK(on_scale_changed), w);
  g_signal_connect(speed_scale,  "value-changed",    G_CALLBACK(on_scale_changed), w);
  g_signal_connect(dir_switch,   "state-set",        G_CALLBACK(on_dir_toggled), w);
  g_signal_connect(off_button,   "clicked",          G_CALLBACK(on_turn_off), w);
  g_signal_connect(retry_button, "clicked",          G_CALLBACK(on_retry), w);

  g_object_set_data_full(G_OBJECT(window), "widgets", w, g_free);

  update_control_visibility(w);
  w->suppress = FALSE;

  DeviceResult r = connect_device(w);
  if (r == DEVICE_OK)
    on_connected(w);
  else
    set_device_error(w, r);

  gtk_window_present(GTK_WINDOW(window));
}

int
main(int argc, char *argv[])
{
  GtkApplication *app = gtk_application_new(
      "dev.attackuwu.Logihub",
      G_APPLICATION_DEFAULT_FLAGS);
  int status;

  g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
  status = g_application_run(G_APPLICATION(app), argc, argv);

  device_close();
  g_object_unref(app);
  return status;
}
