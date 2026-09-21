class WidgetList:

    def __init__(self) -> None:
        """
        Create an empty list (capacity 128 rows).
        """
        ...

    def area(self, x: int, y: int, width: int, height: int) -> None:
        """
        Set the drawing area; default is the whole body area.
        """
        ...

    def clear(self) -> None:
        """
        Remove all rows.
        """
        ...

    def add(self, text: str, tag: str = "", /) -> None:
        """
        Append a row; the optional tag is drawn amber on the left.
        """
        ...

    def set_empty_text(self, text: str) -> None:
        """
        Text shown centered when the list is empty.
        """
        ...

    def set_show_marks(self, show_marks: bool) -> None:
        """
        Show the multi-select mark column.
        """
        ...

    def toggle_mark(self) -> None:
        """
        Toggle the mark on the cursor row.
        """
        ...

    def mark(self, index: int, marked: bool) -> None:
        """
        Set the mark on row index.
        """
        ...

    def marked(self, index: int) -> bool:
        """
        Whether row index is marked.
        """
        ...

    def count(self) -> int:
        """
        Number of rows.
        """
        ...

    def index(self) -> int:
        """
        Cursor row.
        """
        ...

    def set_index(self, index: int) -> None:
        """
        Move the cursor (clamped; scroll follows).
        """
        ...

    def top(self) -> int:
        """
        First visible row.
        """
        ...

    def handle(self, key: str) -> bool:
        """
        Consume UP/DOWN, k/j, ;/. cursor keys; True when consumed.
        """
        ...

    def draw(self, sprite: Sprite | BandedSprite) -> None:
        """
        Draw rows, selection, marks, and the scrollbar when needed.
        """
        ...
