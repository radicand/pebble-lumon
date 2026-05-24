#include <pebble.h>

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

static Window *s_window;
static Layer *s_canvas_layer;
static GFont s_font_small;
static GFont s_font_time;
static GFont s_font_status;
static int32_t s_steps = -1;
static uint8_t s_battery_percent;

static const char *DAYS[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
static const char *MONTHS[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
static const char *MDR_FILES[] = {"TUMWATER", "CAIRNS", "SIENA", "ALLENTOWN", "WELLINGTON", "PACIFICA", "BELLEFONTE", "NANTUCKET", "KIER"};

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

static int16_t random_grid_pos(int16_t seed, uint32_t salt, int16_t max_row, int16_t max_col, int16_t *row, int16_t *col) {
  if (max_row <= 0 || max_col <= 0) {
    *row = 0;
    *col = 0;
    return 0;
  }

  uint32_t h = ((uint32_t)seed ^ salt) * 2654435761u;
  *row = h % max_row;
  *col = (h / max_row) % max_col;
  return 1;
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
  random_grid_pos(seed, 0x12345678u, grid_rows, grid_cols - time_len + 1, &layout->time_row, &layout->time_col);
  random_grid_pos(seed, 0xABCDEF00u, grid_rows - 1, grid_cols - date_len + 1, &layout->date_row, &layout->date_col);
  if (layout->date_row >= layout->time_row) {
    layout->date_row++;
  }
}

static bool is_occupied(const DateTimeLayout *layout, int16_t row, int16_t col) {
  if (row == layout->time_row && col >= layout->time_col && col < layout->time_col + (int16_t)strlen(layout->time)) {
    return true;
  }

  return row == layout->date_row && col >= layout->date_col && col < layout->date_col + (int16_t)strlen(layout->date);
}

static void draw_centered_cell_text(GContext *ctx, const char *text, GFont font, GColor color, int16_t cell_x, int16_t cell_y, int16_t cell_w, int16_t cell_h) {
  graphics_context_set_text_color(ctx, color);
  graphics_draw_text(ctx, text, font, GRect(cell_x, cell_y - 1, cell_w, cell_h + 4), GTextOverflowModeFill, GTextAlignmentCenter, NULL);
}

static void draw_grid_text(GContext *ctx, const char *text, int16_t row, int16_t col, GFont font, int16_t grid_left, int16_t grid_top, int16_t cell_w, int16_t cell_h) {
  char ch[2] = {'\0', '\0'};
  for (int16_t i = 0; text[i] != '\0'; i++) {
    ch[0] = text[i];
    draw_centered_cell_text(ctx, ch, font, col_fg(), grid_left + (col + i) * cell_w, grid_top + row * cell_h, cell_w, cell_h);
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

static void draw_dividers(GContext *ctx, int16_t width, int16_t height) {
  graphics_context_set_fill_color(ctx, col_dim());
  graphics_fill_rect(ctx, GRect(8, 40, width - 16, 1), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(8, height - 52, width - 16, 1), 0, GCornerNone);
}

static void draw_mdr_grid(GContext *ctx, const DateTimeLayout *layout, const struct tm *tick_time, int16_t grid_left, int16_t grid_top, int16_t grid_cols, int16_t grid_rows, int16_t cell_w, int16_t cell_h) {
  const int16_t seed = tick_time->tm_hour * 60 + tick_time->tm_min;
  char digit[2] = {'0', '\0'};

  for (int16_t row = 0; row < grid_rows; row++) {
    for (int16_t col = 0; col < grid_cols; col++) {
      if (is_occupied(layout, row, col)) {
        continue;
      }

      digit[0] = '0' + cell_digit(col, row, seed);
      draw_centered_cell_text(ctx, digit, s_font_small, col_dim(), grid_left + col * cell_w, grid_top + row * cell_h, cell_w, cell_h);
    }
  }
}

static void draw_status_rows(GContext *ctx, const struct tm *tick_time, int16_t width, int16_t height) {
  char steps_value[12];
  char steps_text[18];
  char battery_text[12];
  fmt_steps(steps_value, sizeof(steps_value), s_steps);
  snprintf(steps_text, sizeof(steps_text), "STEPS %s", steps_value);
  snprintf(battery_text, sizeof(battery_text), "BAT %d%%", s_battery_percent);

  const char *right;
  GColor right_color = col_dim();
  if (tick_time->tm_min == 0) {
    right = "PRAISE KIER";
    right_color = col_fg();
  } else {
    right = MDR_FILES[(tick_time->tm_hour * 60 + tick_time->tm_min) % ARRAY_LENGTH(MDR_FILES)];
  }

  graphics_context_set_text_color(ctx, col_dim());
  graphics_draw_text(ctx, steps_text, s_font_status, GRect(10, height - 51, width / 2, 22), GTextOverflowModeFill, GTextAlignmentLeft, NULL);
  graphics_context_set_text_color(ctx, right_color);
  graphics_draw_text(ctx, right, s_font_status, GRect(width / 2, height - 51, (width / 2) - 10, 22), GTextOverflowModeFill, GTextAlignmentRight, NULL);

  graphics_context_set_text_color(ctx, col_dim());
  graphics_draw_text(ctx, battery_text, s_font_status, GRect(10, height - 28, width / 2, 22), GTextOverflowModeFill, GTextAlignmentLeft, NULL);
  graphics_draw_text(ctx, "DEPT MDR", s_font_status, GRect(width / 2, height - 28, (width / 2) - 10, 22), GTextOverflowModeFill, GTextAlignmentRight, NULL);
}

static void canvas_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  const int16_t width = bounds.size.w;
  const int16_t height = bounds.size.h;
  const int16_t grid_top = 44;
  const int16_t grid_bottom = height - 54;
  const int16_t cell_w = 22;
  const int16_t cell_h = 22;
  const int16_t grid_cols = width / cell_w;
  const int16_t grid_rows = (grid_bottom - grid_top) / cell_h;
  const int16_t grid_left = (width - grid_cols * cell_w) / 2;

  time_t now = time(NULL);
  struct tm *tick_time = localtime(&now);
  DateTimeLayout layout;
  make_date_time_layout(&layout, tick_time, grid_rows, grid_cols);

  graphics_context_set_fill_color(ctx, col_bg());
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  draw_lumon_logo(ctx, width / 2, 2);
  draw_dividers(ctx, width, height);
  draw_mdr_grid(ctx, &layout, tick_time, grid_left, grid_top, grid_cols, grid_rows, cell_w, cell_h);
  draw_grid_text(ctx, layout.time, layout.time_row, layout.time_col, s_font_time, grid_left, grid_top, cell_w, cell_h);
  draw_grid_text(ctx, layout.date, layout.date_row, layout.date_col, s_font_status, grid_left, grid_top, cell_w, cell_h);
  draw_status_rows(ctx, tick_time, width, height);
}

static void mark_dirty(void) {
  if (s_canvas_layer) {
    layer_mark_dirty(s_canvas_layer);
  }
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_steps();
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

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  s_canvas_layer = layer_create(bounds);
  layer_set_update_proc(s_canvas_layer, canvas_update_proc);
  layer_add_child(window_layer, s_canvas_layer);
}

static void window_unload(Window *window) {
  layer_destroy(s_canvas_layer);
  s_canvas_layer = NULL;
}

static void init(void) {
  s_font_small = fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD);
  s_font_time = fonts_get_system_font(FONT_KEY_LECO_20_BOLD_NUMBERS);
  s_font_status = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);

  update_battery();
  update_steps();

  s_window = window_create();
  window_set_background_color(s_window, col_bg());
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);

  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  battery_state_service_subscribe(battery_handler);
  health_service_events_subscribe(health_handler, NULL);
}

static void deinit(void) {
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  health_service_events_unsubscribe();
  window_destroy(s_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
