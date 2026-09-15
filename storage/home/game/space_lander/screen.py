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
        game.sprite.text(0, 0, "Space Lander", COLOR_SHIP)
        game.sprite.text(0, 2 * CHARACTER_HEIGHT, text, COLOR_TEXT)
        game.sprite.text(
            0,
            3 * CHARACTER_HEIGHT,
            "Level {}".format(game.level),
            COLOR_TEXT
        )
        game.sprite.text(
            0,
            4 * CHARACTER_HEIGHT,
            "Fuel {}".format(int(game.fuel)),
            COLOR_TEXT
        )
        game.sprite.text(
            0,
            6 * CHARACTER_HEIGHT,
            "V {} H {}".format(
                format_one_decimal(game.velocity_y),
                format_one_decimal(game.velocity_x)
            ),
            COLOR_TEXT
        )
