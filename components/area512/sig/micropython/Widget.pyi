class Widget:

    @staticmethod
    def bg() -> int:
        """
        Fixed background color (0x000000).
        """
        ...

    @staticmethod
    def amber() -> int:
        """
        Fixed accent color for frames and bands (0xF5972D).
        """
        ...

    @staticmethod
    def dim() -> int:
        """
        Fixed normal-text color (0xCFA45F).
        """
        ...

    @staticmethod
    def gold() -> int:
        """
        Fixed selection/title color (0xFFD966).
        """
        ...

    @staticmethod
    def dark() -> int:
        """
        Fixed fill color for selected rows and panels (0x241604).
        """
        ...

    @staticmethod
    def theme_background() -> int:
        """
        Screen background, set in /etc/theme (default 0x000000).
        """
        ...

    @staticmethod
    def theme_text() -> int:
        """
        Body text (default 0xCFA45F).
        """
        ...

    @staticmethod
    def theme_emphasis() -> int:
        """
        Emphasized text, headings, key chips, chart values (default 0xF5972D).
        """
        ...

    @staticmethod
    def theme_border() -> int:
        """
        Frames, rules and ticks (default 0xF5972D).
        """
        ...

    @staticmethod
    def theme_selected() -> int:
        """
        Selected or focused rows and controls (default 0xFFD966).
        """
        ...

    @staticmethod
    def theme_box() -> int:
        """
        Fill inside panels, popups and table headers (default 0x241604).
        """
        ...

    @staticmethod
    def char_width() -> int:
        """
        ASCII glyph width in px at font 12 (6).
        """
        ...

    @staticmethod
    def row_height() -> int:
        """
        Standard list/table row height (16).
        """
        ...

    @staticmethod
    def header_height() -> int:
        """
        Header band height (17).
        """
        ...

    @staticmethod
    def body_top() -> int:
        """
        First y below the header band.
        """
        ...

    @staticmethod
    def body_bottom() -> int:
        """
        Last y above the footer rule.
        """
        ...

    @staticmethod
    def body_height() -> int:
        """
        Height of the content area between header and footer.
        """
        ...

    @staticmethod
    def text_width(sprite: Sprite | BandedSprite, text: str) -> int:
        """
        Measured text width in px (correct for Japanese).
        """
        ...

    @staticmethod
    def clip(sprite: Sprite | BandedSprite, text: str, width: int) -> str:
        """
        Clip text to a pixel width; ends with ">" when cut.
        """
        ...

    @staticmethod
    def read_key() -> str:
        """
        Read one key: "UP" "DOWN" "LEFT" "RIGHT" "ENTER" "ESC" "BS" or a
        one-character string.
        """
        ...

    @staticmethod
    def header(sprite: Sprite | BandedSprite, title: str, right: str = "", /) -> None:
        """
        Top band: highlight title, text-color right-aligned text.
        """
        ...

    @staticmethod
    def footer(sprite: Sprite | BandedSprite, message: str) -> None:
        """
        Bottom rule + clipped message.
        """
        ...

    @staticmethod
    def hints(sprite: Sprite | BandedSprite, hints: list[tuple[str, str]]) -> None:
        """
        Footer-position key chips from [(key, label), ...].
        """
        ...

    @staticmethod
    def separator(sprite: Sprite | BandedSprite, y: int) -> None:
        """
        Full-width horizontal rule at y.
        """
        ...

    @staticmethod
    def vseparator(sprite: Sprite | BandedSprite, x: int, y: int, height: int) -> None:
        """
        Vertical rule.
        """
        ...

    @staticmethod
    def tabs(sprite: Sprite | BandedSprite, y: int, labels: list[str], active: int) -> None:
        """
        Tab bar at y; active tab highlighted.
        """
        ...

    @staticmethod
    def battery(sprite: Sprite | BandedSprite, x: int, y: int) -> None:
        """
        Battery icon with charge level at (x, y).
        """
        ...

    @staticmethod
    def splash(sprite: Sprite | BandedSprite, title: str, subtitle: str = "", /) -> None:
        """
        Full-screen title in the 24px font.
        """
        ...

    @staticmethod
    def big_text(sprite: Sprite | BandedSprite, x: int, y: int, text: str, color: int = Widget.theme_emphasis(), /) -> None:
        """
        Draw text in the 24px font; the default color is the theme highlight.
        """
        ...

    @staticmethod
    def toast(sprite: Sprite | BandedSprite, message: str) -> None:
        """
        Bottom-center message chip; the app decides how long to show it.
        """
        ...

    @staticmethod
    def panel(sprite: Sprite | BandedSprite, x: int, y: int, width: int, height: int) -> None:
        """
        Theme fill + accent frame.
        """
        ...

    @staticmethod
    def titled_panel(sprite: Sprite | BandedSprite, x: int, y: int, width: int, height: int, title: str) -> None:
        """
        Panel with a highlight title row.
        """
        ...

    @staticmethod
    def text_center(sprite: Sprite | BandedSprite, y: int, text: str, color: int = Widget.theme_text(), /) -> None:
        """
        Horizontally centered text; the default color is the theme text color.
        """
        ...

    @staticmethod
    def text_right(sprite: Sprite | BandedSprite, right_x: int, y: int, text: str, color: int = Widget.theme_text(), /) -> None:
        """
        Right-aligned text; the default color is the theme text color.
        """
        ...

    @staticmethod
    def center_lines(sprite: Sprite | BandedSprite, lines: list[str | tuple[str, int]]) -> None:
        """
        Vertically centered lines from [text or (text, color), ...]; ""
        rows are spacers.
        """
        ...

    @staticmethod
    def wrap_text(sprite: Sprite | BandedSprite, x: int, y: int, width: int, height: int, text: str, color: int = Widget.theme_text(), /) -> int:
        """
        Word-wrapped text in a box; returns lines drawn.
        """
        ...

    @staticmethod
    def marquee(sprite: Sprite | BandedSprite, x: int, y: int, width: int, text: str, offset: int) -> int:
        """
        Horizontally scrolling text; returns text px width.
        """
        ...

    @staticmethod
    def cell(sprite: Sprite | BandedSprite, x: int, y: int, width: int, height: int, text: str, selected: bool) -> None:
        """
        Table cell; selected = theme fill + highlight frame.
        """
        ...

    @staticmethod
    def table_header(sprite: Sprite | BandedSprite, x: int, y: int, widths: list[int], labels: list[str]) -> None:
        """
        Column header row.
        """
        ...

    @staticmethod
    def table_row(sprite: Sprite | BandedSprite, x: int, y: int, widths: list[int], texts: list[str], selected: bool) -> None:
        """
        Row of cells.
        """
        ...

    @staticmethod
    def field(sprite: Sprite | BandedSprite, x: int, y: int, width: int, label: str, value: str, focused: bool) -> None:
        """
        Settings row: text-color label left, highlight value right.
        """
        ...

    @staticmethod
    def gauge(sprite: Sprite | BandedSprite, x: int, y: int, width: int, value: int, max: int) -> None:
        """
        Read-only progress bar.
        """
        ...

    @staticmethod
    def slider(sprite: Sprite | BandedSprite, x: int, y: int, width: int, value: int, max: int, focused: bool) -> None:
        """
        Track + knob.
        """
        ...

    @staticmethod
    def scrollbar(sprite: Sprite | BandedSprite, x: int, y: int, height: int, top: int, visible: int, total: int) -> None:
        """
        Vertical scrollbar; hidden when total <= visible.
        """
        ...

    @staticmethod
    def hscrollbar(sprite: Sprite | BandedSprite, x: int, y: int, width: int, left: int, visible: int, total: int) -> None:
        """
        Horizontal scrollbar.
        """
        ...

    @staticmethod
    def badge(sprite: Sprite | BandedSprite, x: int, y: int, text: str) -> None:
        """
        Inverted chip: accent fill, background-color text.
        """
        ...

    @staticmethod
    def busy(sprite: Sprite | BandedSprite, x: int, y: int, frame: int) -> None:
        """
        4-phase busy marker; pass a frame counter.
        """
        ...

    @staticmethod
    def page_dots(sprite: Sprite | BandedSprite, y: int, count: int, active: int) -> None:
        """
        Centered page-indicator dots.
        """
        ...

    @staticmethod
    def bar_chart(sprite: Sprite | BandedSprite, x: int, y: int, width: int, height: int, values: list[int], max: int) -> None:
        """
        Bar chart; max <= 0 auto-scales to the data.
        """
        ...

    @staticmethod
    def line_chart(sprite: Sprite | BandedSprite, x: int, y: int, width: int, height: int, values: list[int], max: int) -> None:
        """
        Polyline chart; max <= 0 auto-scales to the data.
        """
        ...

    @staticmethod
    def button(sprite: Sprite | BandedSprite, x: int, y: int, width: int, label: str, focused: bool) -> None:
        """
        Button with centered label.
        """
        ...

    @staticmethod
    def checkbox(sprite: Sprite | BandedSprite, x: int, y: int, label: str, checked: bool, focused: bool) -> None:
        """
        Checkbox with label.
        """
        ...

    @staticmethod
    def radio(sprite: Sprite | BandedSprite, x: int, y: int, label: str, selected: bool, focused: bool) -> None:
        """
        Radio button with label.
        """
        ...

    @staticmethod
    def toggle(sprite: Sprite | BandedSprite, x: int, y: int, on: bool, focused: bool) -> None:
        """
        ON/OFF toggle pill.
        """
        ...

    @staticmethod
    def spinner(sprite: Sprite | BandedSprite, x: int, y: int, width: int, text: str, focused: bool) -> None:
        """
        "< text >" value chooser; the app handles left/right keys.
        """
        ...

    @staticmethod
    def input(sprite: Sprite | BandedSprite, label: str, initial: str = "", /) -> str | None:
        """
        Modal one-line text input; str on Enter, None on ESC.
        """
        ...

    @staticmethod
    def input_number(sprite: Sprite | BandedSprite, label: str, initial: int, min: int, max: int) -> int | None:
        """
        Modal number input clamped to min..max; None on ESC.
        """
        ...

    @staticmethod
    def confirm(sprite: Sprite | BandedSprite, question: str) -> bool:
        """
        Modal y/n question; ESC = False.
        """
        ...

    @staticmethod
    def dialog(sprite: Sprite | BandedSprite, message: str, buttons: list[str]) -> int | None:
        """
        Modal message with a button row; button index or None on ESC.
        """
        ...

    @staticmethod
    def menu(sprite: Sprite | BandedSprite, title: str, items: list[str], initial: int = 0, /) -> int | None:
        """
        Modal item picker; item index or None on ESC.
        """
        ...

    @staticmethod
    def alert(sprite: Sprite | BandedSprite, message: str) -> None:
        """
        Modal message; returns on any key.
        """
        ...
