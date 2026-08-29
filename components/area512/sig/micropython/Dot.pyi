class Dot:

    @staticmethod
    def load(path: str) -> Dot:
        """
        Load a dot image (.a5d); the path is rooted at the card's data
        directory.
        """
        ...

    def width(self) -> int:
        """
        Dot image width in dots.
        """
        ...

    def height(self) -> int:
        """
        Dot image height in dots.
        """
        ...

    def painted_left(self) -> int:
        """
        Left edge of the painted dots; 0 when nothing is painted.
        """
        ...

    def painted_top(self) -> int:
        """
        Top edge of the painted dots; 0 when nothing is painted.
        """
        ...

    def painted_right(self) -> int:
        """
        Right edge of the painted dots; -1 when nothing is painted.
        """
        ...

    def painted_bottom(self) -> int:
        """
        Bottom edge of the painted dots; -1 when nothing is painted.
        """
        ...

    def push(self, sprite: Sprite, x: int, y: int) -> None:
        """
        Draw the dot image onto a sprite at (x, y); palette index 0 is skipped.
        """
        ...

    @staticmethod
    def overlap(first: Dot, first_x: int, first_y: int, second: Dot, second_x: int, second_y: int, use_hitbox: bool = False, /) -> bool:
        """
        True when a dot overlaps between the two images placed at the given
        positions; compares dots with a color index other than 0, or dots
        marked in the editor with the x tile when use_hitbox is true.
        """
        ...

    @staticmethod
    def edit(path: str) -> None:
        """
        Open the dot editor; an empty file starts as a 32x32 blank image.
        """
        ...
