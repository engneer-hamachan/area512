class QuaternarySprite:

    BACKGROUND_COLOR_INDEX: int
    TEXT_COLOR_INDEX: int
    EMPHASIS_COLOR_INDEX: int
    BORDER_COLOR_INDEX: int
    SELECTED_COLOR_INDEX: int
    BOX_COLOR_INDEX: int

    def __init__(self, width: int, height: int, font_size: int | None = None, /) -> None:
        """
        Create an off-screen sprite of 4-bit palette indices in the theme
        colors.
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

    def fill(self, color_index: int) -> None:
        """
        Fill the whole sprite with a *_COLOR_INDEX constant.
        """
        ...

    def pixel(self, x: int, y: int, color_index: int) -> None:
        """
        Draw a single pixel.
        """
        ...

    def line(self, x0: int, y0: int, x1: int, y1: int, color_index: int) -> None:
        """
        Draw a line.
        """
        ...

    def rect(self, x: int, y: int, width: int, height: int, color_index: int) -> None:
        """
        Draw a rectangle outline.
        """
        ...

    def fill_rect(self, x: int, y: int, width: int, height: int, color_index: int) -> None:
        """
        Draw a filled rectangle.
        """
        ...

    def circle(self, x: int, y: int, radius: int, color_index: int) -> None:
        """
        Draw a circle outline.
        """
        ...

    def fill_circle(self, x: int, y: int, radius: int, color_index: int) -> None:
        """
        Draw a filled circle.
        """
        ...

    def text(self, x: int, y: int, string: str, color_index: int) -> None:
        """
        Draw text at (x, y); efont supports Japanese.
        """
        ...

    def push(self, x: int, y: int, transparent_color_index: int | None = None, /) -> None:
        """
        Transfer the sprite to the screen at (x, y); the optional 3rd argument
        is a transparent *_COLOR_INDEX constant.
        """
        ...
