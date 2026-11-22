# Generated from builtins.pyi.

class object:
    __doc__: str | None
    __dict__: dict[str, Any]
    __module__: str
    __annotations__: dict[str, Any]

class staticmethod(Generic[_P, _R_co]):
    pass

class classmethod(Generic[_T, _P, _R_co]):
    pass

class type:
    __bases__: tuple[type, ...]
    __module__: str
    __name__: str
    __qualname__: str

class super:
    pass

class int:

    def to_bytes(self, length: SupportsIndex=1, byteorder: Literal['little', 'big']='big', *, signed: bool=False) -> bytes:
        ...

    @classmethod
    def from_bytes(cls, bytes: Iterable[SupportsIndex] | SupportsBytes | ReadableBuffer, byteorder: Literal['little', 'big']='big', *, signed: bool=False) -> Self:
        ...

    def to_bytes(self, length: SupportsIndex, byteorder: Literal['little', 'big'], *, signed: bool=False) -> bytes:
        ...

    @classmethod
    def from_bytes(cls, bytes: Iterable[SupportsIndex] | SupportsBytes | ReadableBuffer, byteorder: Literal['little', 'big'], *, signed: bool=False) -> Self:
        ...

class str(Sequence[str]):

    def endswith(self, suffix: str | tuple[str, ...], start: SupportsIndex | None=..., end: SupportsIndex | None=..., /) -> bool:
        ...

    def find(self, sub: str, start: SupportsIndex | None=..., end: SupportsIndex | None=..., /) -> int:
        ...

    @overload
    def format(self: LiteralString, *args: LiteralString, **kwargs: LiteralString) -> LiteralString:
        ...

    @overload
    def format(self, *args: object, **kwargs: object) -> str:
        ...

    def index(self, sub: str, start: SupportsIndex | None=..., end: SupportsIndex | None=..., /) -> int:
        ...

    def isalpha(self) -> bool:
        ...

    def isdigit(self) -> bool:
        ...

    def islower(self) -> bool:
        ...

    def isspace(self) -> bool:
        ...

    def isupper(self) -> bool:
        ...

    @overload
    def join(self: LiteralString, iterable: Iterable[LiteralString], /) -> LiteralString:
        ...

    @overload
    def join(self, iterable: Iterable[str], /) -> str:
        ...

    @overload
    def lower(self: LiteralString) -> LiteralString:
        ...

    @overload
    def lower(self) -> str:
        ...

    @overload
    def lstrip(self: LiteralString, chars: LiteralString | None=None, /) -> LiteralString:
        ...

    @overload
    def lstrip(self, chars: str | None=None, /) -> str:
        ...

    def rfind(self, sub: str, start: SupportsIndex | None=..., end: SupportsIndex | None=..., /) -> int:
        ...

    def rindex(self, sub: str, start: SupportsIndex | None=..., end: SupportsIndex | None=..., /) -> int:
        ...

    @overload
    def rsplit(self: LiteralString, sep: LiteralString | None=None, maxsplit: SupportsIndex=-1) -> list[LiteralString]:
        ...

    @overload
    def rsplit(self, sep: str | None=None, maxsplit: SupportsIndex=-1) -> list[str]:
        ...

    @overload
    def rstrip(self: LiteralString, chars: LiteralString | None=None, /) -> LiteralString:
        ...

    @overload
    def rstrip(self, chars: str | None=None, /) -> str:
        ...

    @overload
    def split(self: LiteralString, sep: LiteralString | None=None, maxsplit: SupportsIndex=-1) -> list[LiteralString]:
        ...

    @overload
    def split(self, sep: str | None=None, maxsplit: SupportsIndex=-1) -> list[str]:
        ...

    def startswith(self, prefix: str | tuple[str, ...], start: SupportsIndex | None=..., end: SupportsIndex | None=..., /) -> bool:
        ...

    @overload
    def strip(self: LiteralString, chars: LiteralString | None=None, /) -> LiteralString:
        ...

    @overload
    def strip(self, chars: str | None=None, /) -> str:
        ...

    @overload
    def upper(self: LiteralString) -> LiteralString:
        ...

    @overload
    def upper(self) -> str:
        ...

    @overload
    def replace(self: LiteralString, old: LiteralString, new: LiteralString, /, count: SupportsIndex=-1) -> LiteralString:
        ...

    @overload
    def replace(self, old: str, new: str, /, count: SupportsIndex=-1) -> str:
        ...

    @overload
    def replace(self: LiteralString, old: LiteralString, new: LiteralString, count: SupportsIndex=-1, /) -> LiteralString:
        ...

    @overload
    def replace(self, old: str, new: str, count: SupportsIndex=-1, /) -> str:
        ...

class bytes(Sequence[int]):

    def endswith(self, suffix: ReadableBuffer | tuple[ReadableBuffer, ...], start: SupportsIndex | None=..., end: SupportsIndex | None=..., /) -> bool:
        ...

    def find(self, sub: ReadableBuffer | SupportsIndex, start: SupportsIndex | None=..., end: SupportsIndex | None=..., /) -> int:
        ...

    def index(self, sub: ReadableBuffer | SupportsIndex, start: SupportsIndex | None=..., end: SupportsIndex | None=..., /) -> int:
        ...

    def isalpha(self) -> bool:
        ...

    def isdigit(self) -> bool:
        ...

    def islower(self) -> bool:
        ...

    def isspace(self) -> bool:
        ...

    def isupper(self) -> bool:
        ...

    def join(self, iterable_of_bytes: Iterable[ReadableBuffer], /) -> bytes:
        ...

    def lower(self) -> bytes:
        ...

    def lstrip(self, bytes: ReadableBuffer | None=None, /) -> bytes:
        ...

    def replace(self, old: ReadableBuffer, new: ReadableBuffer, count: SupportsIndex=-1, /) -> bytes:
        ...

    def rfind(self, sub: ReadableBuffer | SupportsIndex, start: SupportsIndex | None=..., end: SupportsIndex | None=..., /) -> int:
        ...

    def rindex(self, sub: ReadableBuffer | SupportsIndex, start: SupportsIndex | None=..., end: SupportsIndex | None=..., /) -> int:
        ...

    def rsplit(self, sep: ReadableBuffer | None=None, maxsplit: SupportsIndex=-1) -> list[bytes]:
        ...

    def rstrip(self, bytes: ReadableBuffer | None=None, /) -> bytes:
        ...

    def split(self, sep: ReadableBuffer | None=None, maxsplit: SupportsIndex=-1) -> list[bytes]:
        ...

    def startswith(self, prefix: ReadableBuffer | tuple[ReadableBuffer, ...], start: SupportsIndex | None=..., end: SupportsIndex | None=..., /) -> bool:
        ...

    def strip(self, bytes: ReadableBuffer | None=None, /) -> bytes:
        ...

    def upper(self) -> bytes:
        ...

@final
class bool(int):
    pass

class tuple(Sequence[_T_co]):

    def count(self, value: Any, /) -> int:
        ...

    def index(self, value: Any, start: SupportsIndex=0, stop: SupportsIndex=sys.maxsize, /) -> int:
        ...

class list(MutableSequence[_T]):
    __hash__: ClassVar[None]

    def copy(self) -> list[_T]:
        ...

    def append(self, object: _T, /) -> None:
        ...

    def extend(self, iterable: Iterable[_T], /) -> None:
        ...

    def pop(self, index: SupportsIndex=-1, /) -> _T:
        ...

    def index(self, value: _T, start: SupportsIndex=0, stop: SupportsIndex=sys.maxsize, /) -> int:
        ...

    def count(self, value: _T, /) -> int:
        ...

    def insert(self, index: SupportsIndex, object: _T, /) -> None:
        ...

    def remove(self, value: _T, /) -> None:
        ...

    @overload
    def sort(self: list[SupportsRichComparisonT], *, key: None=None, reverse: bool=False) -> None:
        ...

    @overload
    def sort(self, *, key: Callable[[_T], SupportsRichComparison], reverse: bool=False) -> None:
        ...

class dict(MutableMapping[_KT, _VT]):
    __hash__: ClassVar[None]

    def copy(self) -> dict[_KT, _VT]:
        ...

    def keys(self) -> dict_keys[_KT, _VT]:
        ...

    def values(self) -> dict_values[_KT, _VT]:
        ...

    def items(self) -> dict_items[_KT, _VT]:
        ...

    @overload
    def get(self, key: _KT, /) -> _VT | None:
        ...

    @overload
    def get(self, key: _KT, default: _VT, /) -> _VT:
        ...

    @overload
    def get(self, key: _KT, default: _T, /) -> _VT | _T:
        ...

    @overload
    def pop(self, key: _KT, /) -> _VT:
        ...

    @overload
    def pop(self, key: _KT, default: _VT, /) -> _VT:
        ...

    @overload
    def pop(self, key: _KT, default: _T, /) -> _VT | _T:
        ...

@final
class range(Sequence[int]):
    pass

def abs(x: SupportsAbs[_T], /) -> _T:
    ...

def all(iterable: Iterable[object], /) -> bool:
    ...

def any(iterable: Iterable[object], /) -> bool:
    ...

def bin(number: int | SupportsIndex, /) -> str:
    ...

def callable(obj: object, /) -> TypeIs[Callable[..., object]]:
    ...

def chr(i: int, /) -> str:
    ...

def dir(o: object=..., /) -> list[str]:
    ...

@overload
def divmod(x: SupportsDivMod[_T_contra, _T_co], y: _T_contra, /) -> _T_co:
    ...

@overload
def divmod(x: _T_contra, y: SupportsRDivMod[_T_contra, _T_co], /) -> _T_co:
    ...

@overload
def getattr(o: object, name: str, /) -> Any:
    ...

@overload
def getattr(o: object, name: str, default: None, /) -> Any | None:
    ...

@overload
def getattr(o: object, name: str, default: bool, /) -> Any | bool:
    ...

@overload
def getattr(o: object, name: str, default: list[Any], /) -> Any | list[Any]:
    ...

@overload
def getattr(o: object, name: str, default: dict[Any, Any], /) -> Any | dict[Any, Any]:
    ...

@overload
def getattr(o: object, name: str, default: _T, /) -> Any | _T:
    ...

def globals() -> dict[str, Any]:
    ...

def hasattr(obj: object, name: str, /) -> bool:
    ...

def hash(obj: object, /) -> int:
    ...

def hex(number: int | SupportsIndex, /) -> str:
    ...

def id(obj: object, /) -> int:
    ...

@overload
def iter(object: SupportsIter[_SupportsNextT], /) -> _SupportsNextT:
    ...

@overload
def iter(object: _GetItemIterable[_T], /) -> Iterator[_T]:
    ...

@overload
def iter(object: Callable[[], _T | None], sentinel: None, /) -> Iterator[_T]:
    ...

@overload
def iter(object: Callable[[], _T], sentinel: object, /) -> Iterator[_T]:
    ...

def isinstance(obj: object, class_or_tuple: _ClassInfo, /) -> bool:
    ...

def issubclass(cls: type, class_or_tuple: _ClassInfo, /) -> bool:
    ...

def len(obj: Sized, /) -> int:
    ...

def locals() -> dict[str, Any]:
    ...

class map(Generic[_S]):
    pass

@overload
def next(i: SupportsNext[_T], /) -> _T:
    ...

@overload
def next(i: SupportsNext[_T], default: _VT, /) -> _T | _VT:
    ...

def oct(number: int | SupportsIndex, /) -> str:
    ...

def ord(c: str | bytes | bytearray, /) -> int:
    ...

@overload
def print(*values: object, sep: str | None=' ', end: str | None='\n', file: SupportsWrite[str] | None=None, flush: Literal[False]=False) -> None:
    ...

@overload
def print(*values: object, sep: str | None=' ', end: str | None='\n', file: _SupportsWriteAndFlush[str] | None=None, flush: bool) -> None:
    ...

@overload
def pow(base: int, exp: int, mod: int) -> int:
    ...

@overload
def pow(base: int, exp: Literal[0], mod: None=None) -> Literal[1]:
    ...

@overload
def pow(base: int, exp: _PositiveInteger, mod: None=None) -> int:
    ...

@overload
def pow(base: int, exp: _NegativeInteger, mod: None=None) -> float:
    ...

@overload
def pow(base: int, exp: int, mod: None=None) -> Any:
    ...

@overload
def pow(base: _PositiveInteger, exp: float, mod: None=None) -> float:
    ...

@overload
def pow(base: _NegativeInteger, exp: float, mod: None=None) -> complex:
    ...

@overload
def pow(base: float, exp: int, mod: None=None) -> float:
    ...

@overload
def pow(base: float, exp: complex | _SupportsSomeKindOfPow, mod: None=None) -> Any:
    ...

@overload
def pow(base: complex, exp: complex | _SupportsSomeKindOfPow, mod: None=None) -> complex:
    ...

@overload
def pow(base: _SupportsPow2[_E, _T_co], exp: _E, mod: None=None) -> _T_co:
    ...

@overload
def pow(base: _SupportsPow3NoneOnly[_E, _T_co], exp: _E, mod: None=None) -> _T_co:
    ...

@overload
def pow(base: _SupportsPow3[_E, _M, _T_co], exp: _E, mod: _M) -> _T_co:
    ...

@overload
def pow(base: _SupportsSomeKindOfPow, exp: float, mod: None=None) -> Any:
    ...

@overload
def pow(base: _SupportsSomeKindOfPow, exp: complex, mod: None=None) -> complex:
    ...

def repr(obj: object, /) -> str:
    ...

@overload
def round(number: _SupportsRound1[_T], ndigits: None=None) -> _T:
    ...

@overload
def round(number: _SupportsRound2[_T], ndigits: SupportsIndex) -> _T:
    ...

def setattr(obj: object, name: str, value: Any, /) -> None:
    ...

@overload
def sorted(iterable: Iterable[SupportsRichComparisonT], /, *, key: None=None, reverse: bool=False) -> list[SupportsRichComparisonT]:
    ...

@overload
def sorted(iterable: Iterable[_T], /, *, key: Callable[[_T], SupportsRichComparison], reverse: bool=False) -> list[_T]:
    ...

@overload
def sum(iterable: Iterable[bool | _LiteralInteger], /, start: int=0) -> int:
    ...

@overload
def sum(iterable: Iterable[_SupportsSumNoDefaultT], /) -> _SupportsSumNoDefaultT | Literal[0]:
    ...

@overload
def sum(iterable: Iterable[_AddableT1], /, start: _AddableT2) -> _AddableT1 | _AddableT2:
    ...

class zip(Generic[_T_co]):
    pass

class BaseException:
    args: tuple[Any, ...]
    __cause__: BaseException | None
    __context__: BaseException | None
    __suppress_context__: bool
    __traceback__: TracebackType | None

class GeneratorExit(BaseException):
    ...

class KeyboardInterrupt(BaseException):
    ...

class SystemExit(BaseException):
    code: sys._ExitCode

class Exception(BaseException):
    ...

class StopIteration(Exception):
    value: Any

class OSError(Exception):
    errno: int | None
    strerror: str
    filename: Any
    filename2: Any

class ArithmeticError(Exception):
    ...

class AssertionError(Exception):
    ...

class AttributeError(Exception):
    pass

class EOFError(Exception):
    ...

class ImportError(Exception):
    name: str | None
    path: str | None
    msg: str

class LookupError(Exception):
    ...

class MemoryError(Exception):
    ...

class NameError(Exception):
    pass

class RuntimeError(Exception):
    ...

class SyntaxError(Exception):
    msg: str
    lineno: int | None
    offset: int | None
    text: str | None
    filename: str | None

class TypeError(Exception):
    ...

class ValueError(Exception):
    ...

class OverflowError(ArithmeticError):
    ...

class ZeroDivisionError(ArithmeticError):
    ...

class IndexError(LookupError):
    ...

class KeyError(LookupError):
    ...

class NotImplementedError(RuntimeError):
    ...

class IndentationError(SyntaxError):
    ...

def const(expr: Const_T) -> Const_T:
    """
    Used to declare that the expression is a constant so that the compiler can
    optimise it.  The use of this function should be as follows::

     from micropython import const

     CONST_X = const(123)
     CONST_Y = const(2 * CONST_X + 1)

    Constants declared this way are still accessible as global variables from
    outside the module they are declared in.  On the other hand, if a constant
    begins with an underscore then it is hidden, it is not available as a global
    variable, and does not take up any memory during execution.

    This `const` function is recognised directly by the MicroPython parser and is
    provided as part of the :mod:`micropython` module mainly so that scripts can be
    written which run under both CPython and MicroPython, by following the above
    pattern.
    """
    ...

def eval(source: str, globals: dict = ..., locals: dict = ...) -> object: ...

def exec(source: str, globals: dict = ..., locals: dict = ...) -> None: ...

def sleep(seconds: float, /) -> None: ...

def sleep_ms(milliseconds: int, /) -> None: ...
