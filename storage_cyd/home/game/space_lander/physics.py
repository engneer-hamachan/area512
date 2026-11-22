def is_ready(game):
    return game.state == STATE_READY


def is_active(game):
    return game.state == STATE_PLAY


def is_landed(game):
    return game.state == STATE_LANDED


def is_crashed(game):
    return game.state == STATE_CRASHED


def is_on_pad(game):
    left = int(game.position_x) - SHIP_FOOT_X
    right = int(game.position_x) + SHIP_FOOT_X

    return left >= game.pad_x and right <= game.pad_x + game.pad_width


def is_safe_speed(game):
    maximum_vertical = 0.95 - (game.level - 1) * 0.06
    maximum_horizontal = 0.55 - (game.level - 1) * 0.035

    if maximum_vertical < 0.55:
        maximum_vertical = 0.55

    if maximum_horizontal < 0.28:
        maximum_horizontal = 0.28

    return abs(game.velocity_y) <= maximum_vertical and \
        abs(game.velocity_x) <= maximum_horizontal


def step(game):
    if not is_active(game):
        return

    update_debris(game)
    game.velocity_y += GRAVITY
    game.velocity_x *= DRAG
    game.position_x += game.velocity_x
    game.position_y += game.velocity_y

    if game.position_x < 3.0:
        game.position_x = 3.0

    if game.position_x > WORLD_WIDTH - 4.0:
        game.position_x = WORLD_WIDTH - 4.0

    if has_debris_hit(game):
        crash(game, "Debris hit. r reset")

    check_ground(game)


def check_ground(game):
    if game.position_y < GROUND_Y - SHIP_FOOT_Y:
        return

    game.position_y = GROUND_Y - SHIP_FOOT_Y

    if is_on_pad(game) and is_safe_speed(game):
        game.state = STATE_LANDED
        game.message = "Touchdown. space next"
    else:
        crash(game, "Crash. r reset")


def crash(game, message):
    game.state = STATE_CRASHED
    game.message = message
