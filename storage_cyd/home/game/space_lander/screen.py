def draw(game):
    while game.sprite.draw():
        game.sprite.fill(COLOR_BACKGROUND)
        draw_status(game)
        draw_terrain(game)
        draw_debris(game)
        draw_ship(game)
        draw_overlay(game)


def draw_end(game):
    text = "Bye."

    if is_landed(game):
        text = "Landed."
    elif is_crashed(game):
        text = "Crashed."

    while game.sprite.draw():
        game.sprite.fill(COLOR_BACKGROUND)
        Widget.center_lines(game.sprite, [
            ("Space Lander", COLOR_SHIP),
            ("", COLOR_TEXT),
            (text, COLOR_TEXT),
            ("Level {}".format(game.level), COLOR_TEXT),
            ("Fuel {}".format(int(game.fuel)), COLOR_TEXT),
            ("", COLOR_TEXT),
            (
                "V {} H {}".format(
                    format_one_decimal(game.velocity_y),
                    format_one_decimal(game.velocity_x)
                ),
                COLOR_TEXT
            )
        ])
