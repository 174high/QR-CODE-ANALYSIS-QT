"""Low-level wrapper around zbar's interface
"""
from ctypes import (
    c_ubyte, c_char_p, c_int, c_uint, c_ulong, c_void_p, Structure,
    CFUNCTYPE, POINTER
)

from . import zbar_library

__all__ = [
    'EXTERNAL_DEPENDENCIES', 'LIBZBAR', 'ZBarConfig', 'ZBarSymbol',
    'zbar_image_create', 'zbar_image_destroy', 'zbar_image_first_symbol',
    'zbar_image_scanner_create', 'zbar_image_scanner_destroy',
    'zbar_image_set_data',
    'zbar_image_set_format', 'zbar_image_set_size', 'zbar_scan_image',
    'zbar_symbol_get_data', 'zbar_symbol_get_loc_size',
    'zbar_symbol_get_loc_x', 'zbar_symbol_get_loc_y', 'zbar_symbol_next'
]

# Globals populated in load_libzbar
LIBZBAR = None
"""ctypes.CDLL
"""

EXTERNAL_DEPENDENCIES = []
"""List of instances of ctypes.CDLL. Helpful when freezing.
"""


# Structs
class zbar_image_scanner(Structure):
    """Opaque C++ class with private implementation
    """
    pass


def load_libzbar():
    """Loads the zbar shared library and its dependencies.

    Populates the globals LIBZBAR and EXTERNAL_DEPENDENCIES.
    """
    global LIBZBAR
    global EXTERNAL_DEPENDENCIES
    if not LIBZBAR:
        libzbar, dependencies = zbar_library.load()
        LIBZBAR = libzbar
        EXTERNAL_DEPENDENCIES = [LIBZBAR] + dependencies

    return LIBZBAR

# Function signatures
def zbar_function(fname, restype, *args):
    """Returns a foreign function exported by `zbar`.

    Args:
        fname (:obj:`str`): Name of the exported function as string.
        restype (:obj:): Return type - one of the `ctypes` primitive C data
        types.
        *args: Arguments - a sequence of `ctypes` primitive C data types.

    Returns:
        cddl.CFunctionType: A wrapper around the function.
    """
    prototype = CFUNCTYPE(restype, *args)
    return prototype((fname, load_libzbar()))


zbar_image_scanner_create = zbar_function(
    'zbar_image_scanner_create',
    POINTER(zbar_image_scanner)
)

zbar_image_scanner_destroy = zbar_function(
    'zbar_image_scanner_destroy',
    None,
    POINTER(zbar_image_scanner)
)

