class BandedSprite:

    def __init__(self, font_size: int, /) -> None:
        """
        Create a sprite that draws the screen one band of rows at a time.
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

    def draw(self) -> bool:
        """
        Transfer the previous region and prepare the next; False after the final transfer.
        """
        ...

    def region_top(self) -> int:
        """
        First screen row held by the current region.
        """
        ...

    def region_bottom(self) -> int:
        """
        One past the last screen row held by the current region.
        """
        ...

