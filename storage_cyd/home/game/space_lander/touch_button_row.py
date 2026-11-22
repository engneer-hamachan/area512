TOUCH_BUTTON_ROW_LEFT = -1
TOUCH_BUTTON_ROW_TOP = 136
TOUCH_BUTTON_ROW_WIDTH = 242
TOUCH_BUTTON_ROW_HEIGHT = 51
TOUCH_BUTTON_ROW_GAP = 2
TOUCH_BUTTON_FONT_SIZE = 8
TOUCH_BUTTON_CHARACTER_WIDTH = 6
TOUCH_BUTTON_LETTER_HEIGHT = 7
TOUCH_BUTTON_PRESSED_MILLISECONDS = 80


def show_touch_button_row(game, labels, is_touch_button_repeating):
    game.touch_button_labels = labels
    game.is_touch_button_repeating = is_touch_button_repeating
    push_touch_button_row(game, -1)


def find_touch_button_left(game, button_index):
    return (
        TOUCH_BUTTON_ROW_WIDTH * button_index // len(game.touch_button_labels)
    )


def push_touch_button_row(game, pressed_button_index):
    if game.touch_button_labels is None:
        return

    sprite = QuaternarySprite(
        TOUCH_BUTTON_ROW_WIDTH,
        TOUCH_BUTTON_ROW_HEIGHT,
        TOUCH_BUTTON_FONT_SIZE
    )
    button_count = len(game.touch_button_labels)
    row_border_height = TOUCH_BUTTON_ROW_HEIGHT - TOUCH_BUTTON_ROW_GAP
    sprite.fill(QuaternarySprite.BACKGROUND_COLOR_INDEX)

    for button_index in range(button_count):
        button_left = find_touch_button_left(game, button_index)
        button_right = find_touch_button_left(game, button_index + 1)
        label_color_index = QuaternarySprite.TEXT_COLOR_INDEX
        fill_color_index = QuaternarySprite.BACKGROUND_COLOR_INDEX

        if button_index == pressed_button_index:
            fill_color_index = QuaternarySprite.SELECTED_COLOR_INDEX
            label_color_index = QuaternarySprite.BACKGROUND_COLOR_INDEX

        sprite.fill_rect(
            button_left,
            TOUCH_BUTTON_ROW_GAP,
            button_right - button_left,
            row_border_height,
            fill_color_index
        )

        if button_index > 0:
            sprite.line(
                button_left,
                TOUCH_BUTTON_ROW_GAP,
                button_left,
                TOUCH_BUTTON_ROW_HEIGHT - 1,
                QuaternarySprite.BORDER_COLOR_INDEX
            )

        label = game.touch_button_labels[button_index]
        label_letter_width = len(label) * TOUCH_BUTTON_CHARACTER_WIDTH - 1
        inside_left = button_left + 1
        inside_right = (
            button_right - 1
            if button_index == button_count - 1
            else button_right
        )
        inside_width = inside_right - inside_left
        inside_top = TOUCH_BUTTON_ROW_GAP + 1
        inside_height = row_border_height - 2
        sprite.text(
            inside_left + (inside_width - label_letter_width) // 2,
            inside_top + (inside_height - TOUCH_BUTTON_LETTER_HEIGHT) // 2,
            label,
            label_color_index
        )

    sprite.rect(
        0,
        TOUCH_BUTTON_ROW_GAP,
        TOUCH_BUTTON_ROW_WIDTH,
        row_border_height,
        QuaternarySprite.BORDER_COLOR_INDEX
    )
    Display.subwindow(
        TOUCH_BUTTON_ROW_LEFT,
        TOUCH_BUTTON_ROW_TOP,
        TOUCH_BUTTON_ROW_WIDTH,
        TOUCH_BUTTON_ROW_HEIGHT
    )
    Display.push_subwindow(sprite)
    sprite.delete()


def find_touch_button_index_at(game, window_x, window_y):
    if game.touch_button_labels is None:
        return None

    if (
        window_x < TOUCH_BUTTON_ROW_LEFT
        or window_x >= TOUCH_BUTTON_ROW_LEFT + TOUCH_BUTTON_ROW_WIDTH
    ):
        return None

    if (
        window_y < TOUCH_BUTTON_ROW_TOP
        or window_y >= TOUCH_BUTTON_ROW_TOP + TOUCH_BUTTON_ROW_HEIGHT
    ):
        return None

    button_index = 0
    row_x = window_x - TOUCH_BUTTON_ROW_LEFT

    while (
        button_index < len(game.touch_button_labels) - 1
        and row_x >= find_touch_button_left(game, button_index + 1)
    ):
        button_index += 1

    return button_index


def track_held_touch_button_index(game):
    current_point = Touch.read_current_point()
    held_button_index = None

    if current_point is not None:
        held_button_index = find_touch_button_index_at(
            game,
            current_point[0],
            current_point[1]
        )

    if (
        held_button_index is not None
        and not game.is_touch_button_repeating[held_button_index]
    ):
        held_button_index = None

    if held_button_index is not None:
        game.last_held_touch_button_index = held_button_index

    pressed_button_index = (
        -1 if held_button_index is None else held_button_index
    )

    if pressed_button_index != game.pressed_touch_button_index:
        game.pressed_touch_button_index = pressed_button_index
        push_touch_button_row(game, pressed_button_index)

    return held_button_index


def read_touch_event(game):
    while True:
        event = Touch.read()

        if event is None:
            return None

        kind = event[0]

        if kind == "DRAG":
            if game.is_touch_button_row_dragged is None:
                game.is_touch_button_row_dragged = (
                    find_touch_button_index_at(game, event[1], event[2])
                    is not None
                )

            if not game.is_touch_button_row_dragged:
                return event
        elif kind == "RELEASE":
            is_touch_button_row_dragged = game.is_touch_button_row_dragged
            game.is_touch_button_row_dragged = None
            game.last_held_touch_button_index = None

            if not is_touch_button_row_dragged:
                return event
        else:
            button_index = find_touch_button_index_at(game, event[1], event[2])
            last_held_button_index = game.last_held_touch_button_index
            game.last_held_touch_button_index = None

            if button_index is None:
                return event

            if button_index == last_held_button_index:
                continue

            push_touch_button_row(game, button_index)
            sleep_ms(TOUCH_BUTTON_PRESSED_MILLISECONDS)
            push_touch_button_row(game, -1)

            return ("BUTTON", button_index, 0)
