# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
"""
mantid.py3compat
------------------

Transitional module providing compatibility functions intended to help
migrating from Python 2 to Python 3. Mostly just wraps six but allowing
for additional functionality of our own.

This module should be fully compatible with:
    * Python >=v2.7
    * Python 3
"""

import inspect
import six
import sys
from types import *

# Enumerations are built in with Python 3
try:
    from enum import Enum
except ImportError:
    # use a compatability layer
    from enum import Enum  # noqa

# -----------------------------------------------------------------------------
# Globals and constants
# -----------------------------------------------------------------------------
__all__ = dir(six)


# -----------------------------------------------------------------------------
# Library functions
# -----------------------------------------------------------------------------
if sys.version_info[0:2] < (3, 2):
    setswitchinterval = sys.setcheckinterval
else:
    setswitchinterval = sys.setswitchinterval

if sys.version_info[0:2] < (3, 5):
    # getfullargspec deprecated up until python 3.5, so use getargspec
    getfullargspec = inspect.getargspec
else:
    getfullargspec = inspect.getfullargspec


# -----------------------------------------------------------------------------
# File manipulation
# -----------------------------------------------------------------------------
csv_open_type = 'w'


# -----------------------------------------------------------------------------
# Strings
# -----------------------------------------------------------------------------
if not hasattr(six, "ensure_str"):
    # Ubuntu 16.04, Windows, and OSX Mantid builds have a version of six which
    # doesn't include ensure_str
    # ensure_str was added in six 1.12.0
    def ensure_str(s, encoding='utf-8', errors='strict'):
        """Coerce *s* to `str`.
        For Python 2:
          - `unicode` -> encoded to `str`
          - `str` -> `str`
        For Python 3:
          - `str` -> `str`
          - `bytes` -> decoded to `str`
        """
        if not isinstance(s, (str, bytes)):
            raise TypeError("not expecting type '%s'" % type(s))
        if isinstance(s, str):
            s = s.encode(encoding, errors)
        elif isinstance(s, bytes):
            s = s.decode(encoding, errors)
        return s


def is_text_string(obj):
    """Return True if `obj` is a text string, False if it is anything else,
    like binary data (Python 3) or QString (Python 2, PyQt API #1)"""
    return isinstance(obj, str)


def to_text_string(obj, encoding=None):
    """Convert `obj` to (unicode) text string"""

    if encoding is None:
        return str(obj)
    elif isinstance(obj, str):
        # In case this function is not used properly, this could happen
        return obj
    else:
        return str(obj, encoding)


def qbytearray_to_str(qba):
    """Convert QByteArray object to str in a way compatible with Python 2/3"""
    return str(bytes(qba.toHex().data()).decode())
