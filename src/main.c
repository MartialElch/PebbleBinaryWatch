#include <pebble.h>

static Window *s_main_window;
static Layer *s_main_layer;

void draw_circle(int x, int y, int r, int fill, GContext *ctx) {
  if (fill == 1) {
    // draw filled circle
    graphics_fill_circle(ctx, GPoint(x, y), r);
  } else {
    // draw empty circle
    graphics_draw_circle(ctx, GPoint(x, y), r);
  }
}

void draw_binary_number(int n, int x, int y, int N, GContext *ctx) {
  int m;

  int RADIUS = 10;
  int SPACE = 5;
  
  // draw binary number using N circles side by side
  // filled circle = 1; empty circle = 0
  for (int i=0; i<N; i++) {
    m = n >> i;
    draw_circle(x, y - i*(2*RADIUS+SPACE), RADIUS, m&1, ctx);
  }
}

void draw_binary_batt(int n, int x, int y, int N, GContext *ctx) {
  int m;

  int RADIUS = 8;
  int SPACE = 4;

  // draw binary number using N circles stacked bottom to top
  // filled circle = 1; empty circle = 0
  for (int i=0; i<N; i++) {
    m = n >> i;
    draw_circle(x - i*(2*RADIUS+SPACE), y, RADIUS, m&1, ctx);
  }
}

// update process used as call back when layer is marked dirty
void update_proc(Layer *layer, GContext *ctx) {
  time_t temp = time(NULL); 
  struct tm* time = localtime(&temp);

  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_fill_color(ctx, GColorBlack);

  // draw seconds
  draw_binary_number(time->tm_sec%10, 132, 146, 4, ctx);
  draw_binary_number((int)time->tm_sec/10, 108, 146, 3, ctx);
  // draw minutes
  draw_binary_number(time->tm_min%10, 84, 146, 4, ctx);
  draw_binary_number((int)time->tm_min/10, 60, 146, 3, ctx);
  // draw hours
  draw_binary_number(time->tm_hour%10, 36, 146, 4, ctx);
  draw_binary_number((int)time->tm_hour/10, 12, 146, 2, ctx);

  // draw battery
  BatteryChargeState battery_state = battery_state_service_peek();
  draw_binary_batt((int)battery_state.charge_percent, 132, 20, 7, ctx);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  // callback update_proc is executed when layer is marked dirty
  layer_mark_dirty(s_main_layer);
}

static void main_window_load(Window *window) {
  // create display canvas
  Layer *window_layer = window_get_root_layer(s_main_window);
  GRect bounds = layer_get_bounds(window_layer);
  s_main_layer = layer_create((GRect) { .origin = { 0, 0 }, .size = { bounds.size.w, bounds.size.h } });
  // register callback to execute when layer ist marked dirty
  layer_set_update_proc(s_main_layer, update_proc);
  layer_add_child(window_get_root_layer(window), s_main_layer);
}

static void main_window_unload(Window *window) {
  layer_destroy(s_main_layer);
}

static void init() {
  // Create main Window element and assign to pointer
  s_main_window = window_create();

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true); 

  // Make sure the time is displayed from the start
  layer_mark_dirty(s_main_layer);

  // Register with TickTimerService
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
}

static void deinit() {
  // Destroy Window
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}