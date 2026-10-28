class Sprite:

    def __init__(self, width: int, height: int, font_size: int | None = None, /) -> None:
        """
        Create an off-screen sprite of the given width and height.
        """
        ...

    def delete(self) -> None:
        """
        Free the sprite buffer immediately.
        """
        ...

    def width(self) -> int:
        """
        Sprite width in pixels.
        """
        ...

    def height(self) -> int:
        """
        Sprite height in pixels.
        """
        ...

    def fill(self, color: int) -> None:
        """
        Fill the whole sprite with an RGB888 color.
        """
        ...

    def pixel(self, x: int, y: int, color: int) -> None:
        """
        Draw a single pixel.
        """
        ...

    def line(self, x0: int, y0: int, x1: int, y1: int, color: int) -> None:
        """
        Draw a line.
        """
        ...

    def rect(self, x: int, y: int, width: int, height: int, color: int) -> None:
        """
        Draw a rectangle outline.
        """
        ...

    def fill_rect(self, x: int, y: int, width: int, height: int, color: int) -> None:
        """
        Draw a filled rectangle.
        """
        ...

    def circle(self, x: int, y: int, radius: int, color: int) -> None:
        """
        Draw a circle outline.
        """
        ...

    def fill_circle(self, x: int, y: int, radius: int, color: int) -> None:
        """
        Draw a filled circle.
        """
        ...

    def text(self, x: int, y: int, string: str, color: int) -> None:
        """
        Draw text at (x, y); efont supports Japanese.
        """
        ...

    def push(self, x: int, y: int, transparent: int | None = None, /) -> None:
        """
        Transfer the sprite to the screen at (x, y); the optional 3rd argument
        is a transparent color.
        """
        ...

class Display:

    @staticmethod
    def width() -> int:
        """
        Screen width in pixels.
        """
        ...

    @staticmethod
    def height() -> int:
        """
        Screen height in pixels.
        """
        ...

    @staticmethod
    def screen_width() -> int:
        """
        Physical display width in pixels, even while the app runs in a window.
        """
        ...

    @staticmethod
    def screen_height() -> int:
        """
        Physical display height in pixels, even while the app runs in a window.
        """
        ...

    @staticmethod
    def full_screen() -> None:
        """
        Draw on the whole display instead of the window; does nothing when
        there is no window. width() and height() then report the display.
        """
        ...

    @staticmethod
    def fill_screen(color: int) -> None:
        """
        Fill the whole screen with an RGB888 color.
        """
        ...

    @staticmethod
    def set_brightness(brightness: int) -> None:
        """
        Set backlight brightness (0..255).
        """
        ...

    @staticmethod
    def show_header_image(path: str, hold_milliseconds: int = 1000, /) -> bool:
        """
        Show an image header file for the given time.
        """
        ...
