class ADC:

    def __init__(self, pin: int | str) -> None:
        """
        Create an ADC object on the given pin.
        """
        ...

    def input(self) -> int:
        """
        The pin number ADC_init accepted.
        """
        ...

    def read(self) -> float:
        """
        Read the pin voltage in volts.
        """
        ...

    def read_voltage(self) -> float:
        """
        Read the pin voltage in volts.
        """
        ...

    def read_raw(self) -> int:
        """
        Read the raw converter value.
        """
        ...
