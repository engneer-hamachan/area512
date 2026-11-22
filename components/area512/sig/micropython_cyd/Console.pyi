class Console:

    @staticmethod
    def reset() -> None:
        """
        Clear the text console and reset its output flag.
        """
        ...

    @staticmethod
    def wait_key_if_output() -> None:
        """
        Wait for one key press if anything was written to the console.
        """
        ...
