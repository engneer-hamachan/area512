#include "area512_display.h"
#include "area512_hal.h"
#include "console_internal.h"
#include "soft_keyboard.h"

#include <lgfx/v1/LGFXBase.hpp>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

static constexpr int TOUCH_EVENT_QUEUE_LENGTH = 16;
static constexpr int DEFAULT_DRAG_START_DISTANCE = 6;
static constexpr int DEFAULT_DRAG_AXIS_DECISION_DISTANCE = 8;
static constexpr int DEFAULT_SWIPE_DISTANCE = 24;

namespace {
struct TouchEvent {
  int kind;
  int screen_x;
  int screen_y;
  int64_t uptime_ms;
};
}

static QueueHandle_t s_touch_event_queue = nullptr;
static SemaphoreHandle_t s_touch_panel_mutex = nullptr;
static bool s_is_touching = false;
static bool s_is_touch_started_on_soft_keyboard = false;
static bool s_is_dragging = false;
static int s_touch_start_screen_x = 0;
static int s_touch_start_screen_y = 0;
static int64_t s_touch_start_uptime_ms = 0;
static int s_last_drag_screen_x = 0;
static int s_last_drag_screen_y = 0;
static int s_drag_start_distance = DEFAULT_DRAG_START_DISTANCE;
static int s_drag_axis_decision_distance = DEFAULT_DRAG_AXIS_DECISION_DISTANCE;
static int s_swipe_distance = DEFAULT_SWIPE_DISTANCE;

static bool
read_touch_point(int *touch_screen_x, int *touch_screen_y) {
  auto *device =
    static_cast<lgfx::v1::LGFX_Device *>(area512_display_device());

  if (device == nullptr)
    return false;

  lgfx::v1::touch_point_t touch_point;

  if (device->getTouch(&touch_point, 1) == 0)
    return false;

  *touch_screen_x = touch_point.x;
  *touch_screen_y = touch_point.y;

  return true;
}

static void
queue_touch_event(
  int kind,
  int touch_screen_x,
  int touch_screen_y,
  int64_t event_uptime_ms
) {

  TouchEvent event = {
    kind,
    touch_screen_x,
    touch_screen_y,
    event_uptime_ms
  };

  xQueueSend(s_touch_event_queue, &event, 0);
}

static void
handle_touch_start(int touch_screen_x, int touch_screen_y) {
  s_is_touching = true;
  s_is_touch_started_on_soft_keyboard =
    area512_is_soft_keyboard_open() &&
    touch_screen_y >= AREA512_SOFT_KEYBOARD_TOP;

  if (s_is_touch_started_on_soft_keyboard) {
    area512_press_soft_keyboard_key(touch_screen_x, touch_screen_y);

    return;
  }

  s_is_dragging = false;
  s_touch_start_screen_x = touch_screen_x;
  s_touch_start_screen_y = touch_screen_y;
  s_touch_start_uptime_ms = area512_read_uptime_ms();
}

static void
handle_touch_move(int touch_screen_x, int touch_screen_y) {
  if (s_is_touch_started_on_soft_keyboard)
    return;

  if (!s_is_dragging) {
    if (
      abs(touch_screen_x - s_touch_start_screen_x) < s_drag_start_distance &&
      abs(touch_screen_y - s_touch_start_screen_y) < s_drag_start_distance
    )
      return;

    s_is_dragging = true;

  } else if (
    touch_screen_x == s_last_drag_screen_x &&
    touch_screen_y == s_last_drag_screen_y
  ) {

    return;
  }

  s_last_drag_screen_x = touch_screen_x;
  s_last_drag_screen_y = touch_screen_y;

  queue_touch_event(
    AREA512_TOUCH_DRAG,
    touch_screen_x,
    touch_screen_y,
    area512_read_uptime_ms()
  );
}

static void
handle_touch_end(void) {
  s_is_touching = false;

  if (s_is_touch_started_on_soft_keyboard) {
    s_is_touch_started_on_soft_keyboard = false;

    area512_release_soft_keyboard_key();

    return;
  }

  if (s_is_dragging)
    queue_touch_event(
      AREA512_TOUCH_RELEASE,
      s_last_drag_screen_x,
      s_last_drag_screen_y,
      area512_read_uptime_ms()
    );

  else
    queue_touch_event(
      AREA512_TOUCH_TAP,
      s_touch_start_screen_x,
      s_touch_start_screen_y,
      s_touch_start_uptime_ms
    );
}

bool
keyboard_init(void) {
  s_touch_event_queue =
    xQueueCreate(TOUCH_EVENT_QUEUE_LENGTH, sizeof(TouchEvent));

  s_touch_panel_mutex = xSemaphoreCreateMutex();

  return s_touch_event_queue != nullptr && s_touch_panel_mutex != nullptr;
}

void
keyboard_poll(void) {
  int touch_screen_x;
  int touch_screen_y;

  if (xSemaphoreTake(s_touch_panel_mutex, 0) != pdTRUE)
    return;

  bool has_touch_point =
    read_touch_point(
      &touch_screen_x,
      &touch_screen_y
    );

  xSemaphoreGive(s_touch_panel_mutex);

  if (!has_touch_point) {
    if (s_is_touching)
      handle_touch_end();

    return;
  }

  if (s_is_touching)
    handle_touch_move(touch_screen_x, touch_screen_y);
  else
    handle_touch_start(touch_screen_x, touch_screen_y);
}

extern "C" bool
area512_read_touch_event(
  int *kind,
  int *touch_screen_x,
  int *touch_screen_y,
  int64_t *event_uptime_ms
) {

  TouchEvent event;

  if (
    s_touch_event_queue == nullptr ||
    xQueueReceive(
      s_touch_event_queue,
      &event,
      0
    ) != pdTRUE
  )
    return false;

  *kind = event.kind;
  *touch_screen_x = event.screen_x;
  *touch_screen_y = event.screen_y;
  *event_uptime_ms = event.uptime_ms;

  return true;
}

extern "C" bool
area512_read_current_touch_point(int *touch_screen_x, int *touch_screen_y) {
  if (!s_is_touching || s_is_touch_started_on_soft_keyboard)
    return false;

  if (s_is_dragging) {
    *touch_screen_x = s_last_drag_screen_x;
    *touch_screen_y = s_last_drag_screen_y;

    return true;
  }

  *touch_screen_x = s_touch_start_screen_x;
  *touch_screen_y = s_touch_start_screen_y;

  return true;
}

extern "C" bool
area512_read_touch_start_point(int *touch_screen_x, int *touch_screen_y) {
  if (
    !s_is_touching || s_is_touch_started_on_soft_keyboard || s_is_dragging
  )
    return false;

  *touch_screen_x = s_touch_start_screen_x;
  *touch_screen_y = s_touch_start_screen_y;

  return true;
}

extern "C" int
area512_read_touch_drag_start_distance(void) {
  return s_drag_start_distance;
}

extern "C" void
area512_set_touch_drag_start_distance(int drag_start_distance) {
  s_drag_start_distance = drag_start_distance;
}

extern "C" int
area512_read_touch_drag_axis_decision_distance(void) {
  return s_drag_axis_decision_distance;
}

extern "C" void
area512_set_touch_drag_axis_decision_distance(int drag_axis_decision_distance) {
  s_drag_axis_decision_distance = drag_axis_decision_distance;
}

extern "C" int
area512_read_touch_swipe_distance(void) {
  return s_swipe_distance;
}

extern "C" void
area512_set_touch_swipe_distance(int swipe_distance) {
  s_swipe_distance = swipe_distance;
}

extern "C" int
area512_find_touch_drag_axis(int horizontal_distance, int vertical_distance) {
  if (
    abs(horizontal_distance) < s_drag_axis_decision_distance &&
    abs(vertical_distance) < s_drag_axis_decision_distance
  )
    return AREA512_TOUCH_DRAG_AXIS_NONE;

  return abs(horizontal_distance) > abs(vertical_distance)
           ? AREA512_TOUCH_DRAG_AXIS_HORIZONTAL
           : AREA512_TOUCH_DRAG_AXIS_VERTICAL;
}

extern "C" int
area512_find_touch_swipe_direction(
  int drag_axis,
  int horizontal_distance,
  int vertical_distance
) {

  if (
    drag_axis != AREA512_TOUCH_DRAG_AXIS_HORIZONTAL ||
    abs(horizontal_distance) < s_swipe_distance ||
    abs(horizontal_distance) <= abs(vertical_distance)
  )
    return AREA512_TOUCH_SWIPE_NONE;

  return horizontal_distance > 0 ? AREA512_TOUCH_SWIPE_RIGHT
                                 : AREA512_TOUCH_SWIPE_LEFT;
}

extern "C" void
area512_lock_touch_panel(void) {
  if (s_touch_panel_mutex != nullptr)
    xSemaphoreTake(s_touch_panel_mutex, portMAX_DELAY);
}

extern "C" void
area512_unlock_touch_panel(void) {
  if (s_touch_panel_mutex != nullptr)
    xSemaphoreGive(s_touch_panel_mutex);
}

extern "C" void
area512_clear_touch_events(void) {
  if (s_touch_event_queue != nullptr)
    xQueueReset(s_touch_event_queue);
}

void
area512_console_feed_ime(uint8_t byte) {
  area512_console_stdin_push(byte);
}

void
area512_console_flush_ime(void) {
}

void
area512_console_toggle_ime(void) {
}

void
area512_keymap_load(void) {
}
