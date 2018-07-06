#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""Setuptools build script for DCCL Bindings."""
__author__ = "Chris Murphy, Toby Schneider"
__copyright__ = "Copyright 2018, The DCCL Project"
__license__ = "LGPL"
__version__ = "3.0.8"

from setuptools import setup, find_packages
from setuptools.extension import Extension

setup(
    name="dccl",
    version=__version__,
    description="Python Bindings for DCCL.",
    author="Chris Murphy",
    author_email="cmurphy@aphysci.com",

    packages=find_packages(),
    ext_modules = [
        Extension(
            "dccl._dccl",
            ["dccl/_dccl.cc"],
            libraries=['dccl', 'protobuf'],
            extra_compile_args = ["-Wno-write-strings"], # Hide a bunch of c++ warnings.
        )
    ]
)

