class GPIO:
    IN: int
    OUT: int
    HIGH_Z: int
    PULL_UP: int
    PULL_DOWN: int
    OPEN_DRAIN: int
    ALT: int

    def __init__(self, pin: int | str, flags: int, alt_function: int = 0, /) -> None:
        """
        Create a GPIO object; flags must select one of IN, OUT, HIGH_Z and ALT.
        """
        ...

    def pin(self) -> int | str:
        """
        The pin this object was created with.
        """
        ...

    def read(self) -> int:
        """
        Read GPIO pin state.
        """
        ...

    def write(self, value: int) -> int:
        """
        Write value to GPIO pin.
        """
        ...

    def high(self) -> bool:
        """
        Check if GPIO pin is HIGH.
        """
        ...

    def low(self) -> bool:
        """
        Check if GPIO pin is LOW.
        """
        ...

    def set_dir(self, flags: int) -> int:
        """
        Set input/output direction.
        """
        ...

    def set_pull(self, flags: int) -> int:
        """
        Set pull-up or pull-down.
        """
        ...

    def open_drain(self, flags: int) -> int:
        """
        Set to open drain output mode.
        """
        ...

    def set_function(self, flags: int, alt_function: int) -> int:
        """
        Set the alternate function.
        """
        ...

    def setmode(self, flags: int, alt_function: int = 0, /) -> int:
        """
        Apply direction, pull, open drain and alternate function at once.
        """
        ...

    @staticmethod
    def read_at(pin: int | str) -> int:
        """
        Read specified pin state.
        """
        ...

    @staticmethod
    def write_at(pin: int | str, value: int) -> int:
        """
        Write value to specified pin.
        """
        ...

    @staticmethod
    def high_at(pin: int) -> bool:
        """
        Check if specified pin is HIGH; unlike its siblings this takes the pin
        number only, mirroring picoruby-gpio.
        """
        ...

    @staticmethod
    def low_at(pin: int | str) -> bool:
        """
        Check if specified pin is LOW.
        """
        ...

    @staticmethod
    def set_dir_at(pin: int | str, flags: int) -> int:
        """
        Set input/output direction of specified pin.
        """
        ...

    @staticmethod
    def set_function_at(pin: int | str, alt_function: int) -> int:
        """
        Set function of specified pin.
        """
        ...

    @staticmethod
    def pull_up_at(pin: int | str) -> int:
        """
        Set specified pin to pull-up.
        """
        ...

    @staticmethod
    def pull_down_at(pin: int | str) -> int:
        """
        Set specified pin to pull-down.
        """
        ...

    @staticmethod
    def open_drain_at(pin: int | str) -> int:
        """
        Set specified pin to open drain output mode.
        """
        ...
