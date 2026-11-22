class RNG:

    @staticmethod
    def random_int() -> int:
        """
        A random integer from the hardware generator.
        """
        ...

    @staticmethod
    def random_string(length: int) -> bytes:
        """
        Random bytes of the given length.
        """
        ...

    @staticmethod
    def uuid() -> str:
        """
        A random RFC 4122 version 4 UUID.
        """
        ...
