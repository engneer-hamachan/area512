def start_game(game):
    game.quit = False
    reset_campaign(game)


def reset_campaign(game):
    game.level = 1
    start_level(game, False)


def start_level(game, play):
    game.position_x = 120.0
    game.position_y = 13.0
    game.velocity_x = 0.0
    game.velocity_y = 0.0
    game.last_key = ""
    game.pad_width = 38 - (game.level - 1) * 3

    if game.pad_width < 18:
        game.pad_width = 18

    game.pad_x = 12 + pick_random_below(BOARD_WIDTH - game.pad_width - 24)
    game.fuel = 105.0 - (game.level - 1) * 7.0

    if game.fuel < 48.0:
        game.fuel = 48.0

    build_debris(game)
    game.state = STATE_PLAY if play else STATE_READY

    if play:
        game.message = "Level {}".format(game.level)
    else:
        game.message = "return start, q quit"


def start_next_level(game):
    game.level += 1
    start_level(game, False)
    game.message = "Level {}. space start".format(game.level)


def build_debris(game):
    game.debris = []
    count = game.level - 1

    if count > 5:
        count = 5

    index = 0
    while index < count:
        debris_x = 12.0 + pick_random_below(BOARD_WIDTH - 24)
        debris_y = 24.0 + pick_random_below(GROUND_Y - 45)
        debris_velocity_x = 0.45 + pick_random_below(30) / 100.0

        if pick_random_below(2) == 0:
            debris_velocity_x = -debris_velocity_x

        game.debris.append([debris_x, debris_y, debris_velocity_x])
        index += 1


def update_debris(game):
    index = 0
    while index < len(game.debris):
        rock = game.debris[index]
        rock[0] += rock[2]

        if rock[0] > WORLD_WIDTH - 3.0:
            rock[0] = 2.0

        if rock[0] < 2.0:
            rock[0] = WORLD_WIDTH - 3.0

        index += 1


def has_debris_hit(game):
    index = 0
    while index < len(game.debris):
        rock = game.debris[index]

        if abs(game.position_x - rock[0]) < 7.0 and \
           abs(game.position_y - rock[1]) < 7.0:
            return True

        index += 1

    return False
