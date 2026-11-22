class Touch:

    @staticmethod
    def read() -> tuple[str, int, int] | None:
        """
        Next touch event as ("TAP" | "DRAG" | "RELEASE", x, y) in window coordinates, or None.
        """
        ...

    @staticmethod
    def open_keyboard() -> None:
        """
        Raise the window and show the software keyboard below it; redraw afterwards.
        """
        ...

    @staticmethod
    def close_keyboard() -> None:
        """
        Hide the software keyboard and put the window back; redraw afterwards.
        """
        ...

    @staticmethod
    def bluetooth_keyboard_connected() -> bool:
        """
        Whether a Bluetooth keyboard is connected.
        """
        ...
