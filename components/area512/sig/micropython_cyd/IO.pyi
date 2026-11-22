class IO:
    """
    The console. IO() and the builtin global STDIN are the same single
    instance. IO().raw() / IO().getch() / IO()._restore_termios() is the
    Python equivalent of Ruby's STDIN.raw { STDIN.getch }.
    """

    def __init__(self) -> None:
        """
        Take the console; every call returns the same instance.
        """
        ...

    def raw(self) -> None:
        """
        Switch the terminal to raw mode.
        """
        ...

    def cooked(self) -> None:
        """
        Switch the terminal to cooked mode.
        """
        ...

    def _restore_termios(self) -> None:
        """
        Restore the terminal settings saved before the last mode change.
        """
        ...

    def echo(self) -> bool:
        """
        Whether the terminal echoes input.
        """
        ...

    def set_echo(self, flag: bool) -> None:
        """
        Turn terminal echo on or off.
        """
        ...

    def read_nonblock(self, maxlen: int) -> str | None:
        """
        Read up to maxlen buffered bytes without waiting; None if none are
        available.
        """
        ...

    def getch(self) -> str:
        """
        Wait for one key press and return it as a one-character string.
        """
        ...
