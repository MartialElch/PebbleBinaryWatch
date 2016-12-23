#include <pebble.h>

static Window *s_main_window;
static Layer *s_main_layer;
static Layer *s_battery_layer;

static PropertyAnimation *s_animation;

static int  M;
static bool charging;
static bool animation_running;

#define FG_COLOR GColorWhite
#define BG_COLOR GColorBlack

// prototype declarations
static void animate_battery_state();

static void animation_stopped_handler(Animation *animation, bool finished, void *context) {
  // free the animation
  property_animation_destroy(s_animation);
  
  // schedule the next one, unless the app is exiting
  if (finished) {
    animation_running = false;
    if (charging) {
      if (M++ > 5) {
        M = 0;
      }
      animate_battery_state();
    }
  }
}

static void animate_battery_state() {
  if (animation_running) {
    return;
  }

  GRect from_frame = GRect(0, 0, 144, 20);
  GRect to_frame = GRect(-10, 0, 134, 20);

  // create the animation
  s_animation = property_animation_create_layer_frame(s_battery_layer, &from_frame, &to_frame);
  animation_set_duration((Animation*)s_animation, 250);
  animation_set_delay((Animation*)s_animation, 0);
  animation_set_curve((Animation*)s_animation, AnimationCurveEaseInOut);
  animation_set_handlers((Animation*)s_animation, (AnimationHandlers) {
    .stopped = animation_stopped_handler
  }, NULL);

  // schedule to occur ASAP with default settings
  animation_schedule((Animation*) s_animation);
}

static void battery_state_handler(BatteryChargeState state) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "battery event");
  if (state.is_charging) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "charging");
    if (! charging) {
      M = 1;
      charging = true;
      animate_battery_state();
    }
  } else {
    charging = false;
  }
}

static void draw_circle(int x, int y, int r, int fill, GContext *ctx) {
  if (fill == 1) {
    // draw filled circle
    graphics_fill_circle(ctx, GPoint(x, y), r);
  } else {
    // draw empty circle
    graphics_draw_circle(ctx, GPoint(x, y), r);
  }
}

static void draw_binary_number(int n, int x, int y, int N, GContext *ctx) {
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

static void draw_battery(int n, int x, int y, GContext *ctx) {
  int m;
  int N      = 7;
  int RADIUS = 8;
  int SPACE  = 4;

  // draw binary number using N circles stacked bottom to top
  // filled circle = 1; empty circle = 0
  for (int i=0; i<N; i++) {
    m = n >> i;
    draw_circle(x - i*(2*RADIUS+SPACE), y, RADIUS, m&1, ctx);
  }
}

// update process used as call back when layer is marked dirty
static void update_proc(Layer *layer, GContext *ctx) {
  time_t temp = time(NULL);
  struct tm* time = localtime(&temp);
  
  graphics_context_set_stroke_color(ctx, FG_COLOR);
  graphics_context_set_fill_color(ctx, FG_COLOR);

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
  if (!charging) {
    BatteryChargeState battery_state = battery_state_service_peek();
    draw_battery((int)battery_state.charge_percent, 132, 20, ctx);
  } else {
    draw_battery(1<<M, 132, 20, ctx);
  }
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  if (charging) {
    return;
  }

  // callback update_proc is executed when layer is marked dirty
  layer_mark_dirty(s_main_layer);
}

static void main_window_load(Window *window) {
  // create display canvas
  Layer *window_layer = window_get_root_layer(s_main_window);
  GRect bounds = layer_get_bounds(window_layer);
  s_main_layer = layer_create((GRect) { .origin = { 0, 0 }, .size = { bounds.size.w, bounds.size.h } });
  s_battery_layer = layer_create((GRect) { .origin = { 0, 0 }, .size = { bounds.size.w, 20 } });

  // register callback to execute when layer ist marked dirty
  layer_set_update_proc(s_main_layer, update_proc);
  layer_add_child(window_get_root_layer(window), s_main_layer);
  layer_add_child(window_get_root_layer(window), s_battery_layer);
}

static void main_window_unload(Window *window) {
  layer_destroy(s_main_layer);
}

static void init() {
  // create main Window element and assign to pointer
  s_main_window = window_create();
  window_set_background_color(s_main_window, BG_COLOR);
  // set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);

  // look up charging state
  BatteryChargeState battery_state = battery_state_service_peek();
  if (battery_state.is_charging) {
    charging = true;
  } else {
    charging = false;
  }

  // make sure the time is displayed from the start
  layer_mark_dirty(s_main_layer);

  // register with TickTimerService
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);

  // register with battery state service
  battery_state_service_subscribe(battery_state_handler);
}

static void deinit() {
  // destroy Window
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
