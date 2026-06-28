#include <pebble.h>

extern uint32_t MESSAGE_KEY_ANIM_MODE;

#define PERSIST_KEY_ANIM_MODE 1
#define BINS_H 38
#define FILE_DIVIDER_Y 38
#define GRID_TOP 44
#define CELL_W 20
#define CELL_H 20
#define BIN_COUNT 2
#define ANIM_OPEN_FRAMES 6
#define ANIM_COLLECT_FRAMES 18
#define ANIM_CLOSE_FRAMES 6
#define ANIM_TOTAL_FRAMES (ANIM_OPEN_FRAMES + ANIM_COLLECT_FRAMES + ANIM_CLOSE_FRAMES)
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
  int16_t x;
  int16_t y;
  int16_t w;
  int16_t h;
} BinGeom;

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
static GFont s_font_bin_value;
static GFont s_font_file;
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
static const char *MDR_FILES[] = {
  "ALLENTOWN", "TRINITY", "TODOS SANTOS", "ASTORIA", "LUCKNOW", "ST. PIERRE", "COLEMAN",
  "WAYNESBORO", "CORK", "MOLDE", "CAIRNS", "BODO", "ZURICH", "CULPEPPER", "BELLINGHAM",
  "BILLINGS", "YAKIMA", "LOVELAND", "MERIDA", "SOPCHOPPY", "VILNIUS", "RHODES",
  "WELLINGTON", "DRANESVILLE", "COLD HARBOR"
};

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

static uint32_t mdr_hour_slot(const struct tm *tick_time) {
  return (((uint32_t)tick_time->tm_year + 1900u) * 366u + (uint32_t)tick_time->tm_yday) * 24u + (uint32_t)tick_time->tm_hour;
}

static int mdr_file_index_for_slot(uint32_t slot) {
  const uint32_t h = slot * 2654435761u ^ 0x4D445246u;
  return (int)(h % ARRAY_LENGTH(MDR_FILES));
}

static int mdr_file_index_for_hour(const struct tm *tick_time) {
  const uint32_t slot = mdr_hour_slot(tick_time);
  int idx = mdr_file_index_for_slot(slot);
  if (idx == mdr_file_index_for_slot(slot - 1)) {
    idx = (idx + 1) % ARRAY_LENGTH(MDR_FILES);
  }
  return idx;
}

static const char *mdr_file_for_time(const struct tm *tick_time) {
  if (tick_time->tm_min == 0) {
    return "PRAISE KIER";
  }
  return MDR_FILES[mdr_file_index_for_hour(tick_time)];
}

static void draw_top_divider_with_file(GContext *ctx, int16_t width, const struct tm *tick_time) {
  const char *file_name = mdr_file_for_time(tick_time);
  const bool file_highlight = tick_time->tm_min == 0;
  const GRect text_rect = GRect(8, FILE_DIVIDER_Y - 9, width - 16, 18);
  const GSize text_size = graphics_text_layout_get_content_size(
    file_name, s_font_file, text_rect, GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter);
  const int16_t divider_y = text_rect.origin.y + text_size.h / 2 + 2;
  const int16_t text_left = 8 + (width - 16 - text_size.w) / 2;
  const int16_t gap_start = text_left - 4;
  const int16_t gap_end = text_left + text_size.w + 4;

  graphics_context_set_fill_color(ctx, col_dim());
  if (gap_start > 8) {
    graphics_fill_rect(ctx, GRect(8, divider_y, gap_start - 8, 1), 0, GCornerNone);
  }
  if (gap_end < width - 8) {
    graphics_fill_rect(ctx, GRect(gap_end, divider_y, width - 8 - gap_end, 1), 0, GCornerNone);
  }

  graphics_context_set_text_color(ctx, file_highlight ? col_fg() : col_dim());
  graphics_draw_text(ctx, file_name, s_font_file, text_rect, GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
}

static void draw_dividers(GContext *ctx, int16_t width, const GridGeometry *geom) {
  graphics_context_set_fill_color(ctx, col_dim());
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

static void compute_bin_geom(int16_t x, int16_t y, int16_t bin_w, int16_t bin_h, BinGeom *geom) {
  geom->x = x;
  geom->y = y;
  geom->w = bin_w;
  geom->h = bin_h;
}

static void bin_geom_for_index(int16_t width, const GridGeometry *grid_geom, uint8_t index, BinGeom *bin_geom) {
  const int16_t x = bin_rect_x(width, index);
  const int16_t y = grid_geom->bins_top + 2;
  const int16_t bin_w = bin_rect_w(width);
  const int16_t bin_h = grid_geom->bins_h - 4;
  compute_bin_geom(x, y, bin_w, bin_h, bin_geom);
}

static void bin_mouth_center(const BinGeom *geom, int16_t *cx, int16_t *cy) {
  *cx = geom->x + geom->w / 2;
  *cy = geom->y + 2;
}

static void draw_doubled_hline(GContext *ctx, int16_t x1, int16_t y, int16_t x2) {
  graphics_draw_line(ctx, GPoint(x1, y), GPoint(x2, y));
  graphics_draw_line(ctx, GPoint(x1, y + 2), GPoint(x2, y + 2));
}

static void draw_bin_funnel(GContext *ctx, const BinGeom *geom, int16_t open_amount) {
  if (open_amount <= 0) {
    return;
  }

  const int16_t top_y = geom->y;
  const int16_t left_x = geom->x;
  const int16_t right_x = geom->x + geom->w;
  const int16_t open_h = (open_amount * 36) / 256;
  const int16_t diag_spread = (open_amount * 12) / 256;
  const int16_t lid_len = (open_amount * (geom->w / 2)) / 256;

  const int16_t left_diag_x = left_x - diag_spread;
  const int16_t left_diag_y = top_y - open_h;
  const int16_t right_diag_x = right_x + diag_spread;
  const int16_t right_diag_y = top_y - open_h;

  graphics_context_set_stroke_color(ctx, col_fg());
  graphics_draw_line(ctx, GPoint(left_x, top_y), GPoint(left_diag_x, left_diag_y));
  graphics_draw_line(ctx, GPoint(right_x, top_y), GPoint(right_diag_x, right_diag_y));
  draw_doubled_hline(ctx, left_diag_x, left_diag_y, left_diag_x + lid_len);
  draw_doubled_hline(ctx, right_diag_x - lid_len, right_diag_y, right_diag_x);
}

static void draw_single_bin(GContext *ctx, const BinGeom *geom, const char *label, const char *value, int pct, bool highlight, int16_t pulse) {
  const int16_t bar_h = 5;
  const int16_t bar_y = geom->y + geom->h - bar_h - 3;
  const int16_t label_h = 14;
  const int16_t bar_inner_w = geom->w - 6;
  const GColor stroke = highlight ? col_fg() : col_dim();

  graphics_context_set_stroke_color(ctx, stroke);
  graphics_draw_rect(ctx, GRect(geom->x, geom->y, geom->w, geom->h));

  graphics_context_set_text_color(ctx, col_dim());
  graphics_draw_text(ctx, label, s_font_bin, GRect(geom->x + 2, geom->y + 1, 24, label_h), GTextOverflowModeFill, GTextAlignmentLeft, NULL);
  graphics_draw_text(ctx, value, s_font_bin_value, GRect(geom->x + 18, geom->y + 1, geom->w - 20, label_h), GTextOverflowModeFill, GTextAlignmentRight, NULL);

  graphics_context_set_fill_color(ctx, col_dim());
  graphics_fill_rect(ctx, GRect(geom->x + 3, bar_y, bar_inner_w, bar_h), 0, GCornerNone);

  if (pct >= 0) {
    const int16_t fill_w = (int16_t)((bar_inner_w * pct) / 100) + pulse;
    graphics_context_set_fill_color(ctx, col_fg());
    if (fill_w >= bar_inner_w) {
      graphics_fill_rect(ctx, GRect(geom->x + 3, bar_y, bar_inner_w, bar_h), 0, GCornerNone);
    } else if (fill_w > 0) {
      graphics_fill_rect(ctx, GRect(geom->x + 3, bar_y, fill_w, bar_h), 0, GCornerNone);
    }
  }
}

static void draw_bins(GContext *ctx, int16_t width, const GridGeometry *geom) {
  char steps_value[12];
  fmt_steps(steps_value, sizeof(steps_value), s_steps);

  int steps_pct = -1;
  if (s_steps >= 0) {
    steps_pct = clamp_pct((int)(s_steps * 100 / STEPS_GOAL));
  }
  const int bat_pct = clamp_pct(s_battery_percent);

  const int pcts[BIN_COUNT] = {steps_pct, bat_pct};
  const char *labels[BIN_COUNT] = {"STP", "BAT"};
  char bat_value[8];
  snprintf(bat_value, sizeof(bat_value), "%d%%", s_battery_percent);
  const char *values[BIN_COUNT] = {steps_value, bat_value};

  for (int i = 0; i < BIN_COUNT; i++) {
    BinGeom bin_geom;
    bin_geom_for_index(width, geom, i, &bin_geom);
    const bool highlight = s_anim.active && s_anim.target_bin == (uint8_t)i;
    const int16_t pulse = (highlight && s_anim.frame >= s_anim.total_frames - 2) ? 3 : 0;
    draw_single_bin(ctx, &bin_geom, labels[i], values[i], pcts[i], highlight, pulse);
  }
}

static int16_t ease_out(int16_t t, int16_t max_t) {
  if (max_t <= 0) {
    return 256;
  }
  const int16_t progress = t * 256 / max_t;
  return 256 - ((256 - progress) * (256 - progress) / 256);
}

static int16_t ease_in(int16_t t, int16_t max_t) {
  if (max_t <= 0) {
    return 256;
  }
  const int16_t progress = t * 256 / max_t;
  return (progress * progress) / 256;
}

static int16_t anim_open_amount(void) {
  if (s_anim.frame < ANIM_OPEN_FRAMES) {
    return ease_out(s_anim.frame, ANIM_OPEN_FRAMES - 1);
  }
  if (s_anim.frame < ANIM_OPEN_FRAMES + ANIM_COLLECT_FRAMES) {
    return 256;
  }

  const int16_t close_frame = s_anim.frame - ANIM_OPEN_FRAMES - ANIM_COLLECT_FRAMES;
  return 256 - ease_out(close_frame, ANIM_CLOSE_FRAMES - 1);
}

static int16_t anim_collect_progress(void) {
  if (s_anim.frame < ANIM_OPEN_FRAMES) {
    return 0;
  }
  if (s_anim.frame >= ANIM_OPEN_FRAMES + ANIM_COLLECT_FRAMES) {
    return 256;
  }

  return ease_in(s_anim.frame - ANIM_OPEN_FRAMES, ANIM_COLLECT_FRAMES - 1);
}

static GFont font_for_anim_scale(int16_t scale) {
  if (scale >= 200) {
    return s_font_time;
  }
  if (scale >= 140) {
    return s_font_small;
  }
  if (scale >= 80) {
    return s_font_bin_value;
  }
  return s_font_bin;
}

static void draw_anim_digit(GContext *ctx, char ch, int16_t x, int16_t y, int16_t scale) {
  if (scale < 24) {
    return;
  }

  char str[2] = {ch, '\0'};
  const int16_t size = (s_anim.cell_w * scale) / 256;
  if (size < 6) {
    return;
  }

  const GFont font = font_for_anim_scale(scale);
  draw_centered_cell_text(ctx, str, font, col_fg(), x - size / 2, y - size / 2, size, size, 0);
}

static void draw_collection_animation(GContext *ctx, int16_t width, const GridGeometry *geom) {
  if (!s_anim.active || s_anim.cell_count <= 0) {
    return;
  }

  BinGeom target_bin;
  bin_geom_for_index(width, geom, s_anim.target_bin, &target_bin);
  const int16_t open_amount = anim_open_amount();
  draw_bin_funnel(ctx, &target_bin, open_amount);

  const int16_t collect = anim_collect_progress();
  if (collect <= 0) {
    return;
  }

  int16_t target_cx;
  int16_t target_cy;
  bin_mouth_center(&target_bin, &target_cx, &target_cy);
  const int16_t scale = 256 - (collect * 224 / 256);

  for (int16_t i = 0; i < s_anim.cell_count; i++) {
    const int16_t start_x = s_anim.grid_left + s_anim.cols[i] * s_anim.cell_w + s_anim.cell_w / 2;
    const int16_t start_y = s_anim.grid_top + s_anim.rows[i] * s_anim.cell_h + s_anim.cell_h / 2;
    const int16_t x = start_x + ((target_cx - start_x) * collect) / 256;
    const int16_t y = start_y + ((target_cy - start_y) * collect) / 256;
    draw_anim_digit(ctx, s_anim.chars[i], x, y, scale);
  }
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
    start_collection_from_layout(&s_pending_date_layout, ANIM_TYPE_DATE, 1, &geom);
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
  s_anim.total_frames = ANIM_TOTAL_FRAMES;
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
    start_collection_from_layout(&old_layout, ANIM_TYPE_DATE, 1, &geom);
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
  draw_top_divider_with_file(ctx, width, tick_time);
  draw_mdr_grid(ctx, &layout, tick_time, &geom);
  draw_bins(ctx, width, &geom);
  draw_dividers(ctx, width, &geom);

  const bool hide_time = s_anim.active && s_anim.type == ANIM_TYPE_TIME;
  const bool hide_date = s_hide_date_overlay || (s_anim.active && s_anim.type == ANIM_TYPE_DATE);
  if (!hide_time) {
    draw_grid_text(ctx, layout.time, layout.time_row, layout.time_col, s_font_time, col_fg(), &geom, 0);
  }
  if (!hide_date) {
    draw_grid_text(ctx, layout.date, layout.date_row, layout.date_col, s_font_date, col_fg(), &geom, 0);
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
  s_font_small = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
  s_font_time = fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD);
  s_font_date = fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD);
  s_font_bin = fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD);
  s_font_bin_value = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
  s_font_file = fonts_get_system_font(FONT_KEY_GOTHIC_18);

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
