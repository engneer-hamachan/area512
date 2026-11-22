class SD:

    @staticmethod
    def mount() -> bool:
        """
        Mount the microSD card; raises OSError when no card is found.
        """
        ...

    @staticmethod
    def unmount() -> None:
        """
        Unmount the microSD card.
        """
        ...

    @staticmethod
    def exist(path: str) -> bool:
        """
        Check whether a file exists on the card.
        """
        ...

    @staticmethod
    def mkdir(path: str) -> bool:
        """
        Create a directory on the card; an existing one counts as success.
        """
        ...

    @staticmethod
    def read(path: str) -> str:
        """
        Read a whole file from the card; raises OSError on failure.
        """
        ...

    @staticmethod
    def write(path: str, content: str) -> int:
        """
        Write content to a file on the card and return bytes written.
        """
        ...

    @staticmethod
    def restore_seed() -> bool:
        """
        Restore the random seed file from the card.
        """
        ...

class File:

    def __init__(self, path: str, mode: str | None = None, /) -> None:
        """
        Open a file; mode is "r" (default) or "w".
        """
        ...

    def read(self) -> str:
        """
        Read the remaining file contents.
        """
        ...

    def close(self) -> None:
        """
        Close the file.
        """
        ...

    @staticmethod
    def exist(path: str) -> bool:
        """
        Check if a file exists.
        """
        ...

    @staticmethod
    def file(path: str) -> bool:
        """
        Check if the path is a regular file.
        """
        ...

    @staticmethod
    def directory(path: str) -> bool:
        """
        Check if the path is a directory.
        """
        ...

    @staticmethod
    def unlink(path: str) -> int:
        """
        Delete a file.
        """
        ...

    @staticmethod
    def rename(source_path: str, destination_path: str) -> int:
        """
        Rename a file.
        """
        ...

class Dir:

    def __init__(self, path: str) -> None:
        """
        Open a directory for reading.
        """
        ...

    def read(self) -> str | None:
        """
        Read the next entry name; None when there are no more.
        """
        ...

    def close(self) -> None:
        """
        Close the directory.
        """
        ...

    @staticmethod
    def exist(path: str) -> bool:
        """
        Check if a directory exists.
        """
        ...

    @staticmethod
    def mkdir(path: str) -> int:
        """
        Create a directory.
        """
        ...

    @staticmethod
    def rmdir(path: str) -> int:
        """
        Remove an empty directory.
        """
        ...
