#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Copyright 2026:
#   GobySoft, LLC (2013-)
#   Community contributors (see AUTHORS file)
#
# This file is part of the Dynamic Compact Control Language Library ("DCCL").
#
# DCCL is free software: you can redistribute it and/or modify it under the
# terms of the GNU Lesser General Public License as published by the Free
# Software Foundation, either version 2.1 of the License, or (at your option)
# any later version.
#
# DCCL is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
# A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
# details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with DCCL. If not, see <http://www.gnu.org/licenses/>.

"""Unit tests for the DCCL Python bindings.

These tests cover:
  - Basic encode/decode round-trip for a message with an ID field.
  - decode_with_full_name for a normal (ID-bearing) message.
  - decode_with_full_name for a message with omit_id=true.
  - Error cases: unknown type, mismatched type.

The tests require the following environment variables (set by CMake):
  DCCL_TEST_PROTO_PATH  - directory containing test.proto
  DCCL_INC_PATH         - directory containing dccl/option_extensions.proto
"""

import os
import sys
import unittest

# Allow running from the source tree without installation: if the DCCL Python
# source directory is on PYTHONPATH the module can be imported directly.
try:
    import dccl
except ImportError as exc:
    sys.exit("Could not import dccl Python module: {}".format(exc))

# Resolve paths from environment (set by CTest / CMake) or fall back to
# guessing relative to this file for developer convenience.
_this_dir = os.path.dirname(os.path.abspath(__file__))
PROTO_PATH = os.environ.get("DCCL_TEST_PROTO_PATH", _this_dir)
INC_PATH = os.environ.get("DCCL_INC_PATH", "")


def _load_proto_and_pb2():
    """Load test.proto into DCCL's DynamicProtobufManager and import the
    generated *_pb2 module.  Returns the pb2 module."""
    if INC_PATH:
        dccl.addProtoIncludePath(INC_PATH)
    dccl.addProtoIncludePath(PROTO_PATH)
    dccl.loadProtoFile(os.path.join(PROTO_PATH, "test.proto"))

    # The pb2 file is generated alongside the test script by CMake.
    try:
        import test_pb2  # noqa: PLC0415  (import not at top level is intentional)
        return test_pb2
    except ImportError:
        return None


# Load the proto once for the whole test module.
_pb2 = _load_proto_and_pb2()


def _make_codec():
    """Return a freshly constructed dccl.Codec with all test types loaded."""
    c = dccl.Codec()
    c.load("dccl.test.python.NormalMsg")
    c.load("dccl.test.python.AnotherMsg")
    c.load("dccl.test.python.OmitIdMsg")
    return c


@unittest.skipIf(_pb2 is None, "test_pb2 not available (run via CMake)")
class TestEncodeDecodeNormalMsg(unittest.TestCase):
    """Tests for basic encode/decode of a message that carries a DCCL ID."""

    def setUp(self):
        self.codec = _make_codec()

    def _make_msg(self, d=3.14, i=42):
        msg = _pb2.NormalMsg()
        msg.d = d
        msg.i = i
        return msg

    def test_encode_returns_bytes(self):
        msg = self._make_msg()
        encoded = self.codec.encode(msg)
        self.assertIsInstance(encoded, bytes)
        self.assertGreater(len(encoded), 0)

    def test_decode_round_trip(self):
        original = self._make_msg(d=1.23, i=100)
        encoded = self.codec.encode(original)
        decoded = self.codec.decode(encoded)
        self.assertAlmostEqual(decoded.d, 1.23, places=2)
        self.assertEqual(decoded.i, 100)

    def test_encode_decode_boundary_values(self):
        msg = self._make_msg(d=-100.0, i=-20)
        encoded = self.codec.encode(msg)
        decoded = self.codec.decode(encoded)
        self.assertAlmostEqual(decoded.d, -100.0, places=2)
        self.assertEqual(decoded.i, -20)

    def test_id_returns_correct_id(self):
        msg = self._make_msg()
        encoded = self.codec.encode(msg)
        self.assertEqual(self.codec.id(encoded), 201)

    def test_size_returns_positive_integer(self):
        msg = self._make_msg()
        self.assertGreater(self.codec.size(msg), 0)


@unittest.skipIf(_pb2 is None, "test_pb2 not available (run via CMake)")
class TestDecodeWithFullNameNormalMsg(unittest.TestCase):
    """Tests for decode_with_full_name on a message that carries a DCCL ID."""

    def setUp(self):
        self.codec = _make_codec()

    def _make_msg(self, d=7.5, i=10):
        msg = _pb2.NormalMsg()
        msg.d = d
        msg.i = i
        return msg

    def test_decode_with_full_name_round_trip(self):
        original = self._make_msg(d=7.5, i=10)
        encoded = self.codec.encode(original)
        decoded = self.codec.decode_with_full_name(
            encoded, "dccl.test.python.NormalMsg"
        )
        self.assertAlmostEqual(decoded.d, 7.5, places=2)
        self.assertEqual(decoded.i, 10)

    def test_decode_with_full_name_matches_decode(self):
        original = self._make_msg(d=50.0, i=500)
        encoded = self.codec.encode(original)
        via_decode = self.codec.decode(encoded)
        via_full_name = self.codec.decode_with_full_name(
            encoded, "dccl.test.python.NormalMsg"
        )
        self.assertAlmostEqual(via_decode.d, via_full_name.d, places=5)
        self.assertEqual(via_decode.i, via_full_name.i)

    def test_decode_with_full_name_unloaded_type_raises(self):
        original = self._make_msg()
        encoded = self.codec.encode(original)
        with self.assertRaises(dccl.DcclException):
            self.codec.decode_with_full_name(encoded, "dccl.test.python.NoSuchMsg")

    def test_decode_with_full_name_mismatched_type_raises(self):
        """Encoding NormalMsg (ID=201) then requesting decode as AnotherMsg
        (ID=202) should raise because the embedded ID does not match."""
        original = self._make_msg()
        encoded = self.codec.encode(original)
        with self.assertRaises(dccl.DcclException):
            self.codec.decode_with_full_name(encoded, "dccl.test.python.AnotherMsg")


@unittest.skipIf(_pb2 is None, "test_pb2 not available (run via CMake)")
class TestDecodeWithFullNameOmitId(unittest.TestCase):
    """Tests for decode_with_full_name on a message with omit_id=true."""

    def setUp(self):
        self.codec = _make_codec()

    def _make_omit_msg(self, d=5.0, i=1):
        msg = _pb2.OmitIdMsg()
        msg.d = d
        msg.i = i
        return msg

    def test_encode_omit_id_returns_bytes(self):
        msg = self._make_omit_msg()
        encoded = self.codec.encode(msg)
        self.assertIsInstance(encoded, bytes)
        self.assertGreater(len(encoded), 0)

    def test_decode_with_full_name_omit_id_round_trip(self):
        original = self._make_omit_msg(d=5.0, i=1)
        encoded = self.codec.encode(original)
        decoded = self.codec.decode_with_full_name(
            encoded, "dccl.test.python.OmitIdMsg"
        )
        self.assertAlmostEqual(decoded.d, 5.0, places=2)
        self.assertEqual(decoded.i, 1)

    def test_decode_with_full_name_omit_id_various_values(self):
        for d_val, i_val in [(-99.99, -20), (0.0, 0), (99.99, 3000)]:
            with self.subTest(d=d_val, i=i_val):
                original = self._make_omit_msg(d=d_val, i=i_val)
                encoded = self.codec.encode(original)
                decoded = self.codec.decode_with_full_name(
                    encoded, "dccl.test.python.OmitIdMsg"
                )
                self.assertAlmostEqual(decoded.d, d_val, places=2)
                self.assertEqual(decoded.i, i_val)

    def test_decode_omit_id_raises_without_full_name(self):
        """codec.decode() should raise for an omit_id message because there is
        no embedded ID to look up."""
        msg = self._make_omit_msg()
        encoded = self.codec.encode(msg)
        with self.assertRaises(dccl.DcclException):
            self.codec.decode(encoded)


@unittest.skipIf(_pb2 is None, "test_pb2 not available (run via CMake)")
class TestMiscCodecMethods(unittest.TestCase):
    """Tests for miscellaneous Codec methods."""

    def setUp(self):
        self.codec = _make_codec()

    def test_id_from_descriptor_string(self):
        self.assertEqual(self.codec.id("dccl.test.python.NormalMsg"), 201)

    def test_set_strict_does_not_raise(self):
        self.codec.set_strict(1)
        self.codec.set_strict(0)

    def test_encode_out_of_range_raises_with_strict(self):
        self.codec.set_strict(1)
        msg = _pb2.NormalMsg()
        msg.d = 200.0  # out of [-100, 100]
        msg.i = 0
        with self.assertRaises(Exception):
            self.codec.encode(msg)


if __name__ == "__main__":
    unittest.main()
