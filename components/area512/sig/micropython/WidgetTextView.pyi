class WidgetTextView:

    def __init__(self) -> None:
        """
        Create an empty text view.
        """
        ...

    def area(self, x: int, y: int, width: int, height: int) -> None:
        """
        Set the drawing area; default is the whole body area.
        """
        ...

    def set_text(self, text: str) -> None:
        """
        Set the text (copied, up to 2048 bytes).
        """
        ...

    def scroll(self) -> int:
        """
        First visible wrapped line.
        """
        ...

    def set_scroll(self, scroll: int) -> None:
        """
        Scroll to a wrapped line (clamped).
        """
        ...

    def handle(self, key: str) -> bool:
        """
        Consume UP/DOWN, k/j, ;/. scroll keys; True when consumed.
        """
        ...

    def draw(self, sprite: Sprite | BandedSprite) -> None:
        """
        Draw word-wrapped text and the scrollbar when needed.
        """
        ...
