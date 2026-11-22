def apply_input(game, key):
    game.last_key = key

    if not is_active(game):
        return

    if game.fuel <= 0.0:
        return

    if key == "w" or key == " " or key == "\r" or key == "\n":
        game.velocity_y -= MAIN_THRUST
        game.fuel -= 1.2
    elif key == "a" or key == "h":
        game.velocity_x -= SIDE_THRUST
        game.fuel -= 0.5
    elif key == "d" or key == "l":
        game.velocity_x += SIDE_THRUST
        game.fuel -= 0.5

    if game.fuel < 0.0:
        game.fuel = 0.0


def handle(game, key):
    if key == "q":
        game.quit = True
    elif key == "r" or key == "n":
        start_game(game)
    elif key == "\r" or key == "\n":
        if is_ready(game):
            game.state = STATE_PLAY
            game.message = "w thrust, a/d steer"
        elif is_landed(game):
            start_next_level(game)
        else:
            apply_input(game, key)
    else:
        apply_input(game, key)
