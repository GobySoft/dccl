#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""Setuptools build script for DCCL Bindings."""
__author__ = "Chris Murphy, Toby Schneider"
__copyright__ = "Copyright 2018, The DCCL Project"
__license__ = "LGPL"
__version__ = "3.0.8"

from setuptools import setup, find_packages
from setuptools.extension import Extension
from distutils.command.clean import clean as _clean
from distutils.command.build_py import build_py as _build_py
import subprocess

class clean(_clean):
  def run(self):
    # Delete generated files in the code tree.
    for (dirpath, dirnames, filenames) in os.walk("."):
      for filename in filenames:
        filepath = os.path.join(dirpath, filename)
        if filepath.endswith("_pb2.py") or filepath.endswith(".pyc") or \
          filepath.endswith(".so") or filepath.endswith(".o"):
          os.remove(filepath)
    # _clean is an old-style class, so super() doesn't work.
    _clean.run(self)

class build_py(_build_py):
  def run(self):
    # Generate option_extension.proto file.
    protoc_command = ['protoc', '-I../build/include/', '--python_out=.', '../build/include/dccl/option_extensions.proto']
    if subprocess.call(protoc_command) != 0:
      sys.exit(-1)

    # _build_py is an old-style class, so super() doesn't work.
    _build_py.run(self)

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
    ],

    # Override clean + build to support protobuf.
    cmdclass={
        'clean': clean,
        'build_py': build_py,
    }

)

