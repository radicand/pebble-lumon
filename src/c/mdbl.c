#include <pebble.h>

extern uint32_t MESSAGE_KEY_ANIM_MODE;

#define PERSIST_KEY_ANIM_MODE 1
#define BINS_H 38
#define GRID_TOP 40
#define CELL_W 18
#define CELL_H 18
#define BIN_COUNT 3
#define ANIM_FRAMES 16
#define ANIM_MS 60
#define STEPS_GOAL 10000

typedef struct {
  int16_t x;
  int16_t y;
  int16_t w;
  int16_t h;
} LogoRect;

typedef struct {
  char time[6];
  char date[9];
  int16_t time_row;
  int16_t time_col;
  int16_t date_row;
  int16_t date_col;
} DateTimeLayout;

typedef struct {
  int16_t grid_top;
  int16_t grid_bottom;
  int16_t grid_left;
  int16_t grid_cols;
  int16_t grid_rows;
  int16_t cell_w;
  int16_t cell_h;
  int16_t bins_top;
  int16_t bins_h;
} GridGeometry;

typedef enum {
  ANIM_OFF = 0,
  ANIM_HOURLY = 1,
  ANIM_EVERY_MIN = 2
} AnimMode;

typedef enum {
  ANIM_TYPE_NONE = 0,
  ANIM_TYPE_TIME = 1,
  ANIM_TYPE_DATE = 2
} AnimType;

typedef struct {
  bool active;
  uint8_t frame;
  uint8_t total_frames;
  uint8_t target_bin;
  AnimType type;
  int16_t grid_left;
  int16_t grid_top;
  int16_t cell_w;
  int16_t cell_h;
  int16_t cell_count;
  int16_t cols[8];
  int16_t rows[8];
  char chars[8];
} CollectAnimation;

static Window *s_window;
static Layer *s_canvas_layer;
static GFont s_font_small;
static GFont s_font_time;
static GFont s_font_date;
static GFont s_font_bin;
static GFont s_font_bin_label;
static int32_t s_steps = -1;
static uint8_t s_battery_percent;
static AnimMode s_anim_mode = ANIM_HOURLY;
static CollectAnimation s_anim;
static AppTimer *s_anim_timer;
static struct tm s_prev_time;
static bool s_have_prev_time;
static bool s_pending_date_anim;
static bool s_hide_date_overlay;
static DateTimeLayout s_pending_date_layout;
static int16_t s_screen_w;
static int16_t s_screen_h;
static bool s_boot_demo_done;

static const char *DAYS[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
static const char *MONTHS[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
static const char *MDR_FILES[] = {"TUMWATER", "CAIRNS", "SIENA", "ALLENTOWN", "WELLINGTON", "PACIFICA", "BELLEFONTE", "NANTUCKET", "COLDHARBOR", "KIER"};

static const int16_t LOGO_W = 164;
static const LogoRect LOGO_RECTS[] = {
  {0,6,6,18},{30,6,5,16},{55,6,6,17},{64,6,6,2},{92,6,6,2},{105,6,24,1},{135,6,6,1},{158,6,6,14},
  {103,7,28,1},{135,7,7,1},{64,8,7,1},{91,8,7,1},{102,8,30,2},{135,8,8,1},{64,9,8,2},{90,9,8,1},{135,9,9,1},
  {89,10,9,2},{101,10,15,2},{118,10,14,2},{135,10,11,1},{64,11,9,1},{135,11,12,1},{64,12,10,1},{88,12,10,1},
  {101,12,14,2},{119,12,13,2},{135,12,13,1},{64,13,11,2},{87,13,11,2},{135,13,14,1},{101,14,13,1},
  {120,14,12,2},{135,14,6,14},{142,14,8,1},{64,15,12,1},{86,15,12,1},{101,15,12,2},{143,15,9,1},
  {64,16,13,1},{85,16,13,1},{121,16,11,2},{144,16,9,1},{64,17,6,11},{71,17,7,1},{84,17,7,1},{92,17,6,11},
  {101,17,11,5},{146,17,8,1},{72,18,6,1},{84,18,6,1},{122,18,10,3},{147,18,9,1},{73,19,6,1},{83,19,6,1},
  {149,19,8,1},{73,20,7,1},{82,20,7,1},{150,20,14,1},{74,21,6,1},{81,21,7,1},{121,21,11,1},{151,21,13,1},
  {30,22,6,1},{75,22,12,2},{101,22,12,1},{120,22,12,2},{152,22,12,1},{30,23,7,1},{54,23,7,1},{101,23,13,1},
  {153,23,11,1},{0,24,26,4},{30,24,30,2},{76,24,10,1},{102,24,30,2},{155,24,9,1},{77,25,8,1},{156,25,8,1},
  {31,26,28,1},{77,26,7,1},{103,26,28,1},{157,26,7,1},{33,27,25,1},{78,27,5,1},{104,27,25,1},{158,27,6,1}
};

static GColor col_bg(void) {
  return PBL_IF_COLOR_ELSE(GColorFromRGB(0, 29, 47), GColorBlack);
}

static GColor col_fg(void) {
  return PBL_IF_COLOR_ELSE(GColorFromRGB(225, 255, 255), GColorWhite);
}

static GColor col_dim(void) {
  return PBL_IF_COLOR_ELSE(GColorFromRGB(95, 170, 185), GColorLightGray);
}

static void mark_dirty(void);

static uint32_t hash_cell(int16_t col, int16_t row, int16_t seed) {
  uint32_t h = ((uint32_t)col * 73856093u) ^ ((uint32_t)row * 19349663u) ^ ((uint32_t)seed * 83492791u);
  h = (h ^ (h >> 13)) * 1540483477u;
  return h ^ (h >> 15);
}

static int16_t cell_digit(int16_t col, int16_t row, int16_t seed) {
  return hash_cell(col, row, seed) % 10;
}

static void two_digits(char *out, int value) {
  out[0] = '0' + ((value / 10) % 10);
  out[1] = '0' + (value % 10);
}

static int clamp_pct(int value) {
  if (value < 0) {
    return 0;
  }
  if (value > 100) {
    return 100;
  }
  return value;
}

static void fmt_steps(char *buffer, size_t buffer_size, int32_t steps) {
  if (steps < 0) {
    snprintf(buffer, buffer_size, "--");
  } else if (steps < 1000) {
    snprintf(buffer, buffer_size, "%ld", (long)steps);
  } else if (steps < 10000) {
    snprintf(buffer, buffer_size, "%ld.%ldk", (long)(steps / 1000), (long)((steps % 1000) / 100));
  } else {
    snprintf(buffer, buffer_size, "%ldk", (long)(steps / 1000));
  }
}

static void compute_grid_geometry(int16_t width, int16_t height, GridGeometry *geom) {
  geom->cell_w = CELL_W;
  geom->cell_h = CELL_H;
  geom->grid_top = GRID_TOP;
  geom->bins_h = BINS_H;
  geom->bins_top = height - geom->bins_h;
  geom->grid_bottom = geom->bins_top;
  geom->grid_cols = width / geom->cell_w;
  geom->grid_rows = (geom->grid_bottom - geom->grid_top) / geom->cell_h;
  geom->grid_left = (width - geom->grid_cols * geom->cell_w) / 2;
}

static int16_t random_grid_row(int16_t seed, uint32_t salt, int16_t max_row) {
  if (max_row <= 0) {
    return 0;
  }

  uint32_t h = ((uint32_t)seed ^ salt) * 2654435761u;
  return h % max_row;
}

static void make_date_time_layout(DateTimeLayout *layout, const struct tm *tick_time, int16_t grid_rows, int16_t grid_cols) {
  const int16_t seed = tick_time->tm_hour * 60 + tick_time->tm_min;
  two_digits(&layout->time[0], tick_time->tm_hour);
  layout->time[2] = ':';
  two_digits(&layout->time[3], tick_time->tm_min);
  layout->time[5] = '\0';

  snprintf(layout->date, sizeof(layout->date), "%s%s%02d", DAYS[tick_time->tm_wday], MONTHS[tick_time->tm_mon], tick_time->tm_mday);

  const int16_t time_len = strlen(layout->time);
  const int16_t date_len = strlen(layout->date);
  layout->time_row = random_grid_row(seed, 0x12345678u, grid_rows);
  layout->time_col = (grid_cols - time_len) / 2;
  layout->date_row = random_grid_row(seed, 0xABCDEF00u, grid_rows - 1);
  if (layout->date_row >= layout->time_row) {
    layout->date_row++;
  }
  layout->date_col = (grid_cols - date_len) / 2;
}

static bool is_occupied(const DateTimeLayout *layout, int16_t row, int16_t col) {
  if (row == layout->time_row && col >= layout->time_col && col < layout->time_col + (int16_t)strlen(layout->time)) {
    return true;
  }

  return row == layout->date_row && col >= layout->date_col && col < layout->date_col + (int16_t)strlen(layout->date);
}

static bool is_animating_cell(int16_t row, int16_t col) {
  if (!s_anim.active) {
    return false;
  }

  for (int16_t i = 0; i < s_anim.cell_count; i++) {
    if (s_anim.rows[i] == row && s_anim.cols[i] == col) {
      return true;
    }
  }
  return false;
}

static void draw_centered_cell_text(GContext *ctx, const char *text, GFont font, GColor color, int16_t cell_x, int16_t cell_y, int16_t cell_w, int16_t cell_h, int16_t y_offset) {
  const GRect bounds = GRect(cell_x, cell_y, cell_w, cell_h);
  const GSize text_size = graphics_text_layout_get_content_size(text, font, bounds, GTextOverflowModeFill, GTextAlignmentCenter);
  const GRect draw_rect = GRect(
    cell_x + (cell_w - text_size.w) / 2,
    cell_y + (cell_h - text_size.h) / 2 + y_offset,
    text_size.w,
    text_size.h
  );

  graphics_context_set_text_color(ctx, color);
  graphics_draw_text(ctx, text, font, draw_rect, GTextOverflowModeFill, GTextAlignmentCenter, NULL);
}

static void draw_grid_text(GContext *ctx, const char *text, int16_t row, int16_t col, GFont font, GColor color, const GridGeometry *geom, int16_t y_offset) {
  char ch[2] = {'\0', '\0'};
  for (int16_t i = 0; text[i] != '\0'; i++) {
    ch[0] = text[i];
    draw_centered_cell_text(ctx, ch, font, color, geom->grid_left + (col + i) * geom->cell_w, geom->grid_top + row * geom->cell_h, geom->cell_w, geom->cell_h, y_offset);
  }
}

static void update_steps(void) {
  s_steps = -1;

  if (!PBL_IF_HEALTH_ELSE(true, false)) {
    return;
  }

  const time_t start = time_start_of_today();
  const time_t end = time(NULL);
  HealthServiceAccessibilityMask mask = health_service_metric_accessible(HealthMetricStepCount, start, end);
  if (mask & HealthServiceAccessibilityMaskAvailable) {
    s_steps = health_service_sum_today(HealthMetricStepCount);
  }
}

static void update_battery(void) {
  BatteryChargeState charge = battery_state_service_peek();
  s_battery_percent = charge.charge_percent;
}

static void draw_lumon_logo(GContext *ctx, int16_t cx, int16_t y) {
  const int16_t x = cx - (LOGO_W / 2);
  graphics_context_set_fill_color(ctx, col_fg());
  for (uint16_t i = 0; i < ARRAY_LENGTH(LOGO_RECTS); i++) {
    graphics_fill_rect(ctx, GRect(x + LOGO_RECTS[i].x, y + LOGO_RECTS[i].y, LOGO_RECTS[i].w, LOGO_RECTS[i].h), 0, GCornerNone);
  }
}

static void draw_dividers(GContext *ctx, int16_t width, const GridGeometry *geom) {
  graphics_context_set_fill_color(ctx, col_dim());
  graphics_fill_rect(ctx, GRect(8, 40, width - 16, 1), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(8, geom->grid_bottom, width - 16, 1), 0, GCornerNone);
}

static void draw_mdr_grid(GContext *ctx, const DateTimeLayout *layout, const struct tm *tick_time, const GridGeometry *geom) {
  const int16_t seed = tick_time->tm_hour * 60 + tick_time->tm_min;
  char digit[2] = {'0', '\0'};

  for (int16_t row = 0; row < geom->grid_rows; row++) {
    for (int16_t col = 0; col < geom->grid_cols; col++) {
      if (is_occupied(layout, row, col) || is_animating_cell(row, col)) {
        continue;
      }

      digit[0] = '0' + cell_digit(col, row, seed);
      draw_centered_cell_text(ctx, digit, s_font_small, col_dim(), geom->grid_left + col * geom->cell_w, geom->grid_top + row * geom->cell_h, geom->cell_w, geom->cell_h, 0);
    }
  }
}

static int16_t bin_rect_x(int16_t width, int16_t index) {
  const int16_t gap = 4;
  const int16_t margin = 8;
  const int16_t bin_w = (width - 2 * margin - (BIN_COUNT - 1) * gap) / BIN_COUNT;
  return margin + index * (bin_w + gap);
}

static int16_t bin_rect_w(int16_t width) {
  const int16_t gap = 4;
  const int16_t margin = 8;
  return (width - 2 * margin - (BIN_COUNT - 1) * gap) / BIN_COUNT;
}

static void bin_center(int16_t width, int16_t bins_top, int16_t index, int16_t *cx, int16_t *cy) {
  const int16_t bin_w = bin_rect_w(width);
  *cx = bin_rect_x(width, index) + bin_w / 2;
  *cy = bins_top + BINS_H / 2;
}

static const char *mdr_file_for_time(const struct tm *tick_time) {
  if (tick_time->tm_min == 0) {
    return "KIER";
  }
  return MDR_FILES[(tick_time->tm_hour * 60 + tick_time->tm_min) % ARRAY_LENGTH(MDR_FILES)];
}

static void draw_bins(GContext *ctx, const struct tm *tick_time, int16_t width, const GridGeometry *geom) {
  const int16_t bin_w = bin_rect_w(width);
  const int16_t bin_h = geom->bins_h - 4;
  const int16_t y = geom->bins_top + 2;
  const int16_t bar_h = 5;
  const int16_t bar_y = y + bin_h - bar_h - 3;
  const int16_t label_h = 14;

  char steps_value[12];
  fmt_steps(steps_value, sizeof(steps_value), s_steps);

  int steps_pct = -1;
  if (s_steps >= 0) {
    steps_pct = clamp_pct((int)(s_steps * 100 / STEPS_GOAL));
  }
  const int bat_pct = clamp_pct(s_battery_percent);
  const int file_pct = tick_time->tm_min == 0 ? 100 : clamp_pct(tick_time->tm_min * 100 / 59);
  const char *file_name = mdr_file_for_time(tick_time);
  const bool file_highlight = tick_time->tm_min == 0;
  char file_label[12];
  snprintf(file_label, sizeof(file_label), "%.8s", file_name);

  const int pcts[BIN_COUNT] = {steps_pct, bat_pct, file_pct};
  const char *labels[BIN_COUNT] = {"ST", "BT", NULL};
  char bat_value[8];
  snprintf(bat_value, sizeof(bat_value), "%d%%", s_battery_percent);
  const char *values[BIN_COUNT] = {steps_value, bat_value, file_label};

  for (int i = 0; i < BIN_COUNT; i++) {
    const int16_t x = bin_rect_x(width, i);
    const int pct = pcts[i];
    const int16_t pulse = (s_anim.active && s_anim.target_bin == i && s_anim.frame >= s_anim.total_frames - 2) ? 3 : 0;
    const GColor value_color = (i == 2 && file_highlight) ? col_fg() : col_dim();

    graphics_context_set_stroke_color(ctx, col_dim());
    graphics_draw_rect(ctx, GRect(x, y, bin_w, bin_h));

    if (i == 2) {
      graphics_context_set_text_color(ctx, value_color);
      graphics_draw_text(ctx, values[i], s_font_bin_label, GRect(x + 2, y + 1, bin_w - 4, label_h), GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
    } else {
      graphics_context_set_text_color(ctx, col_dim());
      graphics_draw_text(ctx, labels[i], s_font_bin, GRect(x + 2, y + 1, 16, label_h), GTextOverflowModeFill, GTextAlignmentLeft, NULL);
      graphics_context_set_text_color(ctx, value_color);
      graphics_draw_text(ctx, values[i], s_font_bin, GRect(x + 18, y + 1, bin_w - 20, label_h), GTextOverflowModeFill, GTextAlignmentRight, NULL);
    }

    const int16_t bar_inner_w = bin_w - 6;
    graphics_context_set_fill_color(ctx, col_dim());
    graphics_fill_rect(ctx, GRect(x + 3, bar_y, bar_inner_w, bar_h), 0, GCornerNone);

    if (pct >= 0) {
      const int16_t fill_w = (int16_t)((bar_inner_w * pct) / 100) + pulse;
      graphics_context_set_fill_color(ctx, col_fg());
      if (fill_w >= bar_inner_w) {
        graphics_fill_rect(ctx, GRect(x + 3, bar_y, bar_inner_w, bar_h), 0, GCornerNone);
      } else if (fill_w > 0) {
        graphics_fill_rect(ctx, GRect(x + 3, bar_y, fill_w, bar_h), 0, GCornerNone);
      }
    }
  }
}

static int16_t ease_out(int16_t t, int16_t max_t) {
  if (max_t <= 0) {
    return 256;
  }
  const int16_t progress = t * 256 / max_t;
  return 256 - ((256 - progress) * (256 - progress) / 256);
}

static void draw_collection_animation(GContext *ctx, int16_t width, const GridGeometry *geom) {
  if (!s_anim.active || s_anim.cell_count <= 0) {
    return;
  }

  int32_t sum_x = 0;
  int32_t sum_y = 0;
  int16_t target_cx;
  int16_t target_cy;
  bin_center(width, geom->bins_top, s_anim.target_bin, &target_cx, &target_cy);

  const int16_t eased = ease_out(s_anim.frame, s_anim.total_frames - 1);

  for (int16_t i = 0; i < s_anim.cell_count; i++) {
    const int16_t start_x = s_anim.grid_left + s_anim.cols[i] * s_anim.cell_w + s_anim.cell_w / 2;
    const int16_t start_y = s_anim.grid_top + s_anim.rows[i] * s_anim.cell_h + s_anim.cell_h / 2;
    const int16_t end_x = target_cx;
    const int16_t end_y = target_cy;
    const int16_t x = start_x + ((end_x - start_x) * eased) / 256;
    const int16_t y = start_y + ((end_y - start_y) * eased) / 256;
    sum_x += x;
    sum_y += y;

    char ch[2] = {s_anim.chars[i], '\0'};
    draw_centered_cell_text(ctx, ch, s_font_time, col_fg(), x - s_anim.cell_w / 2, y - s_anim.cell_h / 2, s_anim.cell_w, s_anim.cell_h, -2);
  }

  const int16_t centroid_x = (int16_t)(sum_x / s_anim.cell_count);
  const int16_t centroid_y = (int16_t)(sum_y / s_anim.cell_count);
  const int16_t funnel_top_y = centroid_y - 4;

  graphics_context_set_stroke_color(ctx, col_fg());
  graphics_draw_line(ctx, GPoint(centroid_x - 8, funnel_top_y), GPoint(target_cx, geom->bins_top));
  graphics_draw_line(ctx, GPoint(centroid_x + 8, funnel_top_y), GPoint(target_cx, geom->bins_top));
}

static void start_collection_from_layout(const DateTimeLayout *layout, AnimType type, uint8_t target_bin, const GridGeometry *geom);
static void finish_animation(void);

static void anim_timer_callback(void *data) {
  if (!s_anim.active) {
    return;
  }

  s_anim.frame++;
  if (s_anim.frame >= s_anim.total_frames) {
    finish_animation();
    return;
  }

  mark_dirty();
  s_anim_timer = app_timer_register(ANIM_MS, anim_timer_callback, NULL);
}

static void finish_animation(void) {
  if (s_anim_timer) {
    app_timer_cancel(s_anim_timer);
    s_anim_timer = NULL;
  }

  s_anim.active = false;

  if (s_pending_date_anim) {
    s_pending_date_anim = false;
    s_hide_date_overlay = false;
    GridGeometry geom;
    compute_grid_geometry(s_screen_w, s_screen_h, &geom);
    start_collection_from_layout(&s_pending_date_layout, ANIM_TYPE_DATE, 2, &geom);
    return;
  }

  s_hide_date_overlay = false;

  mark_dirty();
}

static void populate_anim_cells(const DateTimeLayout *layout, const char *text, int16_t row, int16_t col) {
  for (int16_t i = 0; text[i] != '\0' && s_anim.cell_count < 8; i++) {
    s_anim.cols[s_anim.cell_count] = col + i;
    s_anim.rows[s_anim.cell_count] = row;
    s_anim.chars[s_anim.cell_count] = text[i];
    s_anim.cell_count++;
  }
}

static void start_collection_from_layout(const DateTimeLayout *layout, AnimType type, uint8_t target_bin, const GridGeometry *geom) {
  if (s_anim_timer) {
    app_timer_cancel(s_anim_timer);
    s_anim_timer = NULL;
  }

  s_anim.active = true;
  s_anim.frame = 0;
  s_anim.total_frames = ANIM_FRAMES;
  s_anim.target_bin = target_bin;
  s_anim.type = type;
  s_anim.grid_left = geom->grid_left;
  s_anim.grid_top = geom->grid_top;
  s_anim.cell_w = geom->cell_w;
  s_anim.cell_h = geom->cell_h;
  s_anim.cell_count = 0;

  if (type == ANIM_TYPE_TIME) {
    populate_anim_cells(layout, layout->time, layout->time_row, layout->time_col);
  } else if (type == ANIM_TYPE_DATE) {
    populate_anim_cells(layout, layout->date, layout->date_row, layout->date_col);
  }

  if (s_anim.cell_count <= 0) {
    s_anim.active = false;
    return;
  }

  s_anim_timer = app_timer_register(ANIM_MS, anim_timer_callback, NULL);
  mark_dirty();
}

static bool should_animate_time_change(const struct tm *prev, const struct tm *now) {
  if (s_anim_mode == ANIM_OFF) {
    return false;
  }
  if (s_anim_mode == ANIM_EVERY_MIN) {
    return prev->tm_hour != now->tm_hour || prev->tm_min != now->tm_min;
  }
  return prev->tm_hour != now->tm_hour;
}

static bool should_animate_date_change(const struct tm *prev, const struct tm *now) {
  if (s_anim_mode == ANIM_OFF) {
    return false;
  }
  return prev->tm_yday != now->tm_yday || prev->tm_year != now->tm_year;
}

static void handle_time_transitions(struct tm *tick_time) {
  GridGeometry geom;
  compute_grid_geometry(s_screen_w, s_screen_h, &geom);

  if (!s_have_prev_time) {
    s_prev_time = *tick_time;
    s_have_prev_time = true;
    return;
  }

  if (s_anim.active) {
    s_prev_time = *tick_time;
    return;
  }

  const bool time_changed = tick_time->tm_hour != s_prev_time.tm_hour || tick_time->tm_min != s_prev_time.tm_min;
  const bool date_changed = tick_time->tm_yday != s_prev_time.tm_yday || tick_time->tm_year != s_prev_time.tm_year;

  if (!time_changed && !date_changed) {
    return;
  }

  DateTimeLayout old_layout;
  make_date_time_layout(&old_layout, &s_prev_time, geom.grid_rows, geom.grid_cols);

  if (time_changed && should_animate_time_change(&s_prev_time, tick_time)) {
    const uint8_t target_bin = (uint8_t)(s_prev_time.tm_hour % BIN_COUNT);
    start_collection_from_layout(&old_layout, ANIM_TYPE_TIME, target_bin, &geom);
    if (date_changed && should_animate_date_change(&s_prev_time, tick_time)) {
      s_pending_date_layout = old_layout;
      s_pending_date_anim = true;
      s_hide_date_overlay = true;
    }
  } else if (date_changed && should_animate_date_change(&s_prev_time, tick_time)) {
    start_collection_from_layout(&old_layout, ANIM_TYPE_DATE, 2, &geom);
  }

  s_prev_time = *tick_time;
}

static void canvas_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  const int16_t width = bounds.size.w;
  const int16_t height = bounds.size.h;
  GridGeometry geom;
  compute_grid_geometry(width, height, &geom);

  time_t now = time(NULL);
  struct tm *tick_time = localtime(&now);
  DateTimeLayout layout;
  make_date_time_layout(&layout, tick_time, geom.grid_rows, geom.grid_cols);

  graphics_context_set_fill_color(ctx, col_bg());
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  draw_lumon_logo(ctx, width / 2, 2);
  draw_dividers(ctx, width, &geom);
  draw_mdr_grid(ctx, &layout, tick_time, &geom);
  draw_bins(ctx, tick_time, width, &geom);

  const bool hide_time = s_anim.active && s_anim.type == ANIM_TYPE_TIME;
  const bool hide_date = s_hide_date_overlay || (s_anim.active && s_anim.type == ANIM_TYPE_DATE);
  if (!hide_time) {
    draw_grid_text(ctx, layout.time, layout.time_row, layout.time_col, s_font_time, col_fg(), &geom, -2);
  }
  if (!hide_date) {
    draw_grid_text(ctx, layout.date, layout.date_row, layout.date_col, s_font_date, col_fg(), &geom, -2);
  }

  draw_collection_animation(ctx, width, &geom);
}

static void mark_dirty(void) {
  if (s_canvas_layer) {
    layer_mark_dirty(s_canvas_layer);
  }
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_steps();
  handle_time_transitions(tick_time);
  mark_dirty();
}

static void battery_handler(BatteryChargeState charge) {
  s_battery_percent = charge.charge_percent;
  mark_dirty();
}

static void health_handler(HealthEventType event, void *context) {
  update_steps();
  mark_dirty();
}

static void load_settings(void) {
  if (persist_exists(PERSIST_KEY_ANIM_MODE)) {
    const int32_t stored = persist_read_int(PERSIST_KEY_ANIM_MODE);
    if (stored >= ANIM_OFF && stored <= ANIM_EVERY_MIN) {
      s_anim_mode = (AnimMode)stored;
    }
  }
}

static void apply_anim_mode(int32_t mode) {
  if (mode < ANIM_OFF || mode > ANIM_EVERY_MIN) {
    return;
  }
  s_anim_mode = (AnimMode)mode;
  persist_write_int(PERSIST_KEY_ANIM_MODE, mode);
}

static void inbox_received_handler(DictionaryIterator *iter, void *context) {
  Tuple *mode = dict_find(iter, MESSAGE_KEY_ANIM_MODE);
  if (mode) {
    apply_anim_mode(mode->value->int32);
  }
}

static void outbox_failed_handler(DictionaryIterator *iter, AppMessageResult reason, void *context) {
}

static void outbox_sent_handler(DictionaryIterator *iter, void *context) {
}

static void app_message_init(void) {
  app_message_register_inbox_received(inbox_received_handler);
  app_message_register_outbox_failed(outbox_failed_handler);
  app_message_register_outbox_sent(outbox_sent_handler);
  app_message_open(64, 64);
}

static void trigger_collection_demo(void) {
  if (s_anim.active || s_anim_mode == ANIM_OFF) {
    return;
  }

  time_t now = time(NULL);
  struct tm *tick_time = localtime(&now);
  GridGeometry geom;
  DateTimeLayout layout;
  compute_grid_geometry(s_screen_w, s_screen_h, &geom);
  make_date_time_layout(&layout, tick_time, geom.grid_rows, geom.grid_cols);
  start_collection_from_layout(&layout, ANIM_TYPE_TIME, (uint8_t)(tick_time->tm_hour % BIN_COUNT), &geom);
}

static void boot_demo_callback(void *data) {
  if (s_boot_demo_done) {
    return;
  }
  s_boot_demo_done = true;
  trigger_collection_demo();
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  s_screen_w = bounds.size.w;
  s_screen_h = bounds.size.h;
  s_canvas_layer = layer_create(bounds);
  layer_set_update_proc(s_canvas_layer, canvas_update_proc);
  layer_add_child(window_layer, s_canvas_layer);
  app_timer_register(1500, boot_demo_callback, NULL);
}

static void window_unload(Window *window) {
  layer_destroy(s_canvas_layer);
  s_canvas_layer = NULL;
}

static void init(void) {
  s_font_small = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
  s_font_time = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
  s_font_date = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
  s_font_bin = fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD);
  s_font_bin_label = fonts_get_system_font(FONT_KEY_GOTHIC_14);

  load_settings();
  update_battery();
  update_steps();

  s_window = window_create();
  window_set_background_color(s_window, col_bg());
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);

  app_message_init();
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  battery_state_service_subscribe(battery_handler);
  health_service_events_subscribe(health_handler, NULL);
}

static void deinit(void) {
  if (s_anim_timer) {
    app_timer_cancel(s_anim_timer);
    s_anim_timer = NULL;
  }
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  health_service_events_unsubscribe();
  app_message_deregister_callbacks();
  window_destroy(s_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
