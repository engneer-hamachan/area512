def ground_line_y(game):
    return game.board_top + GROUND_Y


def draw_terrain(game):
    left = game.board_left
    ground = ground_line_y(game)
    pad_left = left + game.pad_x
    pad_right = pad_left + game.pad_width
    game.sprite.line(left, ground, left + 44, ground - 8, COLOR_GROUND)
    game.sprite.line(
        left + 44,
        ground - 8,
        left + 88,
        ground + 3,
        COLOR_GROUND
    )
    game.sprite.line(left + 88, ground + 3, pad_left, ground, COLOR_GROUND)
    game.sprite.line(pad_left, ground, pad_right, ground, COLOR_PAD)
    game.sprite.line(pad_right, ground, left + 178, ground + 4, COLOR_GROUND)
    game.sprite.line(
        left + 178,
        ground + 4,
        left + BOARD_WIDTH - 1,
        ground - 7,
        COLOR_GROUND
    )
    label_width = Widget.text_width(game.sprite, "PAD")
    label_x = pad_left + (game.pad_width - label_width) // 2
    game.sprite.text(label_x, ground + 5, "PAD", COLOR_PAD)


def draw_debris(game):
    index = 0
    while index < len(game.debris):
        rock = game.debris[index]
        rock_x = game.board_left + int(rock[0])
        rock_y = game.board_top + int(rock[1])
        game.sprite.line(rock_x - 2, rock_y, rock_x + 2, rock_y, COLOR_DEBRIS)
        game.sprite.line(rock_x, rock_y - 2, rock_x, rock_y + 2, COLOR_DEBRIS)
        index += 1
