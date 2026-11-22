def draw_status(game):
    left = game.board_left
    top = game.board_top
    game.sprite.text(left, top, "LANDER", COLOR_SHIP)
    game.sprite.text(left + 40, top, "L{}".format(game.level), COLOR_TEXT)
    game.sprite.text(
        left + 63,
        top,
        "F{}".format(int(game.fuel)),
        COLOR_TEXT
    )
    game.sprite.text(
        left + 94,
        top,
        "V{}".format(format_one_decimal(game.velocity_y)),
        COLOR_TEXT
    )
    game.sprite.text(
        left + 133,
        top,
        "H{}".format(format_one_decimal(game.velocity_x)),
        COLOR_TEXT
    )
    game.sprite.text(left, top + STATUS_Y, game.message, COLOR_TEXT)


def draw_overlay(game):
    top = game.board_top

    if is_ready(game):
        Widget.text_center(game.sprite, top + 42, "READY", COLOR_SHIP)
        Widget.text_center(game.sprite, top + 56, "RETURN TO START", COLOR_TEXT)
    elif is_landed(game):
        Widget.text_center(game.sprite, top + 42, "TOUCHDOWN", COLOR_SHIP)
        Widget.text_center(game.sprite, top + 56, "RETURN NEXT", COLOR_TEXT)
    elif is_crashed(game):
        Widget.text_center(game.sprite, top + 42, "CRASH", COLOR_CRASH)
        Widget.text_center(game.sprite, top + 56, "R TO RESET", COLOR_TEXT)
