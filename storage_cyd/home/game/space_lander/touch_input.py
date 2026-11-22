TOUCH_BUTTON_LABELS = ["LEFT", "THRUST", "RIGHT", "NEW", "QUIT"]
TOUCH_BUTTON_KEYS = ["a", "w", "d", "r", "q"]
IS_TOUCH_BUTTON_REPEATING = [True, True, True, False, False]
TOUCH_POLL_INTERVAL_MILLISECONDS = 10


def read_touch_key(game):
    held_button_index = track_held_touch_button_index(game)
    event = read_touch_event(game)

    while event is not None:
        if event[0] == "TAP":
            return "\r"

        if event[0] == "BUTTON":
            return TOUCH_BUTTON_KEYS[event[1]]

        event = read_touch_event(game)

    if held_button_index is not None:
        return TOUCH_BUTTON_KEYS[held_button_index]

    return ""
