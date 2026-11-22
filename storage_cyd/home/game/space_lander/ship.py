def draw_ship(game):
    ship_x = game.board_left + int(game.position_x)
    ship_y = game.board_top + int(game.position_y)
    color = COLOR_CRASH if is_crashed(game) else COLOR_SHIP
    game.sprite.line(ship_x, ship_y - 5, ship_x - 5, ship_y + 5, color)
    game.sprite.line(ship_x, ship_y - 5, ship_x + 5, ship_y + 5, color)
    game.sprite.line(ship_x - 5, ship_y + 5, ship_x + 5, ship_y + 5, color)
    game.sprite.line(
        ship_x - 3,
        ship_y + 6,
        ship_x - SHIP_FOOT_X,
        ship_y + SHIP_FOOT_Y,
        color
    )
    game.sprite.line(
        ship_x + 3,
        ship_y + 6,
        ship_x + SHIP_FOOT_X,
        ship_y + SHIP_FOOT_Y,
        color
    )
    draw_flame(game, ship_x, ship_y)


def draw_flame(game, ship_x, ship_y):
    if not is_active(game):
        return

    key = game.last_key

    if key == "w" or key == " " or key == "\r" or key == "\n":
        game.sprite.line(
            ship_x - 2,
            ship_y + 7,
            ship_x,
            ship_y + 14,
            COLOR_FLAME
        )
        game.sprite.line(
            ship_x + 2,
            ship_y + 7,
            ship_x,
            ship_y + 14,
            COLOR_FLAME
        )
    elif key == "a" or key == "h":
        game.sprite.line(
            ship_x + 5,
            ship_y + 1,
            ship_x + 11,
            ship_y + 1,
            COLOR_FLAME
        )
    elif key == "d" or key == "l":
        game.sprite.line(
            ship_x - 5,
            ship_y + 1,
            ship_x - 11,
            ship_y + 1,
            COLOR_FLAME
        )
