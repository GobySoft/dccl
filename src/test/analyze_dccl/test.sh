#!/bin/bash

# Copyright 2026:
#   GobySoft, LLC (2013-)
#   Community contributors (see AUTHORS file)
# File authors:
#   Toby Schneider <toby@gobysoft.org>
#
#
# This file is part of the Dynamic Compact Control Language Library
# ("DCCL").
#
# DCCL is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 2.1 of the License, or
# (at your option) any later version.
#
# DCCL is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with DCCL.  If not, see <http://www.gnu.org/licenses/>.

# Tests analyze_dccl, the deprecated predecessor of `dccl --analyze`.
#
# Usage: test.sh <analyze_dccl binary> <directory holding test.proto> <dccl include dir>

set -u

BIN="$1"
PROTO_DIR="$2"
INC_DIR="$3"
PROTO="${PROTO_DIR}/test.proto"

STDERR_FILE="$(mktemp)"
trap 'rm -f "${STDERR_FILE}"' EXIT

failures=0
checks=0

fail()
{
    echo "FAIL: $1"
    failures=$((failures + 1))
}

pass()
{
    echo "ok: $1"
}

contains()
{
    checks=$((checks + 1))
    case "$2" in
        *"$3"*) pass "$1" ;;
        *) fail "$1 (expected '$3' in: $2)" ;;
    esac
}

# no arguments at all is a usage error
checks=$((checks + 1))
OUT="$("${BIN}" 2>"${STDERR_FILE}")"
STATUS=$?
ERR="$(cat "${STDERR_FILE}")"
if [ ${STATUS} -ne 0 ]; then
    pass "running with no arguments fails"
else
    fail "running with no arguments fails (exit 0)"
fi
contains "the usage message is printed" "${ERR}" "usage: analyze_dccl"
contains "the deprecation notice is printed" "${ERR}" "deprecated"

# A proto that cannot be read. The loader throws for a missing file, so this
# only reports cleanly because the tool catches it; uncaught, the process
# terminates instead.
checks=$((checks + 1))
OUT="$("${BIN}" /nonexistent/nope.proto "${INC_DIR}" 2>"${STDERR_FILE}")"
STATUS=$?
ERR="$(cat "${STDERR_FILE}")"
if [ ${STATUS} -eq 1 ]; then
    pass "an unreadable proto exits 1"
else
    fail "an unreadable proto exits 1 (got ${STATUS})"
fi
contains "the read failure is reported" "${ERR}" "failed to read in"
contains "the failure names the file" "${ERR}" "/nonexistent/nope.proto"
contains "the failure gives the reason" "${ERR}" "File not found"
# terminate() would print this instead of the message above
checks=$((checks + 1))
case "${ERR}" in
    *"terminate called"*) fail "the tool reports rather than aborting" ;;
    *) pass "the tool reports rather than aborting" ;;
esac

# the normal path
checks=$((checks + 1))
OUT="$("${BIN}" "${PROTO}" "${INC_DIR}" "${PROTO_DIR}" 2>"${STDERR_FILE}")"
STATUS=$?
ERR="$(cat "${STDERR_FILE}")"
if [ ${STATUS} -eq 0 ]; then
    pass "analyzing a valid proto succeeds"
else
    fail "analyzing a valid proto succeeds (exit ${STATUS}; stderr: ${ERR})"
fi
contains "the file it read is reported" "${OUT}" "read in:"
contains "the DCCL message is analyzed" "${OUT}" "dccl.test.CommandMsg"
contains "field names appear in the analysis" "${OUT}" "destination"
contains "the codec banner is printed" "${OUT}" "Dynamic Compact Control Language"
# test.proto also holds a plain protobuf message, which is not loadable
contains "a non-DCCL message is reported" "${ERR}" "Not a valid DCCL message"

echo
echo "${checks} checks run, ${failures} failure(s)"
if [ ${failures} -ne 0 ]; then
    exit 1
fi
echo "all tests passed"
exit 0
