#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""This module provides Python bindings for the Dynamic Compact Control Language (DCCL) library."""

import pkg_resources

__author__ = "Chris Murphy, Toby Schneider"
__copyright__ = "Copyright 2018, The DCCL Project"
__license__ = "LGPL"
__version__ = pkg_resources.get_distribution('dccl').version

from ._dccl import *

