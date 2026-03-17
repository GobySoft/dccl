#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""This module provides Python bindings for the Dynamic Compact Control Language (DCCL) library."""

__author__ = "Chris Murphy, Toby Schneider"
__copyright__ = "Copyright 2018, The DCCL Project"
__license__ = "LGPL"
try:
    from pkg_resources import get_distribution, DistributionNotFound
    __version__ = get_distribution('dccl').version
except (ImportError, DistributionNotFound):
    __version__ = "unknown"

from ._dccl import *

