# Generated from gc.pyi.

def mem_alloc() -> int:
    """
    Return the number of bytes of heap RAM that are allocated by Python code.

    Admonition:Difference to CPython
       :class: attention

       This function is MicroPython extension.
    """
    ...

def isenabled(*args, **kwargs) -> Incomplete:
    ...

def mem_free() -> int:
    """
    Return the number of bytes of heap RAM that is available for Python
    code to allocate, or -1 if this amount is not known.

    Admonition:Difference to CPython
       :class: attention

       This function is MicroPython extension.
    """
    ...

def collect() -> int | None:
    """
    Run a garbage collection.
    """
    ...

def enable() -> None:
    """
    Enable automatic garbage collection.
    """
    ...

def disable() -> None:
    """
    Disable automatic garbage collection.  Heap memory can still be allocated,
    and garbage collection can still be initiated manually using :meth:`gc.collect`.
    """
    ...
