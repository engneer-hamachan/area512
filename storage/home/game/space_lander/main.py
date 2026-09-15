class SpaceLander:
    def __init__(self):
        self.console = IO()
        self.sprite = None
        self.board_left = 0
        self.board_top = 0
        self.quit = False
        self.state = STATE_READY
        self.level = 1
        self.position_x = 0.0
        self.position_y = 0.0
        self.velocity_x = 0.0
        self.velocity_y = 0.0
        self.fuel = 0.0
        self.pad_x = 0
        self.pad_width = 0
        self.debris = []
        self.last_key = ""
        self.message = ""

    def read_key(self):
        key = self.console.read_nonblock(1)

        return "" if key is None else key

    def wait_key(self):
        self.console.raw()
        self.console.getch()
        self.console._restore_termios()

    def run(self):
        screen_width = Display.width()
        screen_height = Display.height()
        self.board_left = (screen_width - BOARD_WIDTH) // 2
        self.board_top = (screen_height - BOARD_HEIGHT) // 2
        Display.fill_screen(COLOR_BACKGROUND)
        self.sprite = BandedSprite(FONT_SIZE)

        try:
            start_game(self)

            while not self.quit:
                handle(self, self.read_key())
                step(self)
                draw(self)
                sleep_ms(FRAME_MILLISECONDS)

            draw_end(self)
            self.wait_key()
        finally:
            self.sprite.delete()
            self.sprite = None
            Display.fill_screen(COLOR_BACKGROUND)


SpaceLander().run()
