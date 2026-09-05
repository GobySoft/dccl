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

# End-to-end tests for the 'dccl' command line tool.
#
# Usage: test.sh <path to dccl binary> <directory holding test.proto> <dccl include dir>

set -u

DCCL_BIN="$1"
PROTO_DIR="$2"
INC_DIR="$3"

PROTO="${PROTO_DIR}/test.proto"
DCCL=("${DCCL_BIN}" -I "${INC_DIR}" -I "${PROTO_DIR}")

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

# run <description> <expected exit status: ok|err> <stdin> -- <args...>
run()
{
    local desc="$1" expect="$2" input="$3"
    shift 3
    [ "${1:-}" = "--" ] && shift

    checks=$((checks + 1))
    OUT="$(printf '%s\n' "${input}" | "${DCCL[@]}" "$@" 2>"${STDERR_FILE}")"
    STATUS=$?
    ERR="$(cat "${STDERR_FILE}")"

    if [ "${expect}" = "ok" ] && [ ${STATUS} -ne 0 ]; then
        fail "${desc} (expected success, got exit ${STATUS}; stderr: ${ERR})"
        return 1
    fi
    if [ "${expect}" = "err" ] && [ ${STATUS} -eq 0 ]; then
        fail "${desc} (expected failure, got exit 0; stdout: ${OUT})"
        return 1
    fi
    pass "${desc}"
    return 0
}

# contains <description> <haystack> <needle>
contains()
{
    checks=$((checks + 1))
    case "$2" in
        *"$3"*) pass "$1" ;;
        *) fail "$1 (expected to find '$3' in: $2)" ;;
    esac
}

equals()
{
    checks=$((checks + 1))
    if [ "$2" = "$3" ]; then
        pass "$1"
    else
        fail "$1 (expected '$3', got '$2')"
    fi
}

echo "=== using dccl binary: ${DCCL_BIN}"

##
## informational actions
##
run "--version" ok "" -- --version
contains "version is non-empty" "${OUT}" "."

run "-V short form" ok "" -- -V

run "--help" ok "" -- --help
contains "help lists --encode" "${OUT}" "--encode"
contains "help lists --decode" "${OUT}" "--decode"
contains "help lists --hash_only" "${OUT}" "--hash_only"
contains "help lists --format" "${OUT}" "--format"

# no action is not an error, but it should say so
run "no action given" ok "" -- -f "${PROTO}" -m dccl.test.CommandMsg
contains "no action explains itself" "${ERR}" "No action specified"

##
## analyze
##
run "analyze single message" ok "" -- -a -f "${PROTO}" -m dccl.test.CommandMsg
contains "analyze names the message" "${OUT}" "dccl.test.CommandMsg"
contains "analyze reports field" "${OUT}" "destination"

run "analyze whole file" ok "" -- -a -f "${PROTO}"
contains "analyze covers CommandMsg" "${OUT}" "dccl.test.CommandMsg"
contains "analyze covers StatusMsg" "${OUT}" "dccl.test.StatusMsg"
contains "non-DCCL message is reported" "${ERR}" "Not a valid DCCL message"

run "analyze with console width" ok "" -- -a -w 60 -f "${PROTO}" -m dccl.test.CommandMsg
run "analyze verbose" ok "" -- -a -v -f "${PROTO}" -m dccl.test.CommandMsg

##
## hashes
##
run "hash only, single message" ok "" -- -a -H -f "${PROTO}" -m dccl.test.CommandMsg
HASH_ONE="${OUT}"
checks=$((checks + 1))
if echo "${HASH_ONE}" | grep -qE '^0x[0-9a-f]+$'; then
    pass "hash is hexadecimal"
else
    fail "hash is hexadecimal (got '${HASH_ONE}')"
fi

# the hash is a function of the definition, so it must be stable across runs
run "hash only, repeated" ok "" -- -a -H -f "${PROTO}" -m dccl.test.CommandMsg
equals "hash is stable" "${OUT}" "${HASH_ONE}"

run "hash only, two messages" ok "" -- -a -H -f "${PROTO}" -m dccl.test.CommandMsg -m dccl.test.StatusMsg
contains "multi-message hash is labelled" "${OUT}" "dccl.test.CommandMsg:"
contains "multi-message hash lists both" "${OUT}" "dccl.test.StatusMsg:"

# different definitions must not collide
CMD_HASH=$(echo "${OUT}" | grep CommandMsg | awk '{print $2}')
STATUS_HASH=$(echo "${OUT}" | grep StatusMsg | awk '{print $2}')
checks=$((checks + 1))
if [ "${CMD_HASH}" != "${STATUS_HASH}" ]; then
    pass "distinct messages hash differently"
else
    fail "distinct messages hash differently (both ${CMD_HASH})"
fi

##
## display_proto
##
run "display_proto" ok "" -- -p -f "${PROTO}" -m dccl.test.CommandMsg
contains "proto output has message keyword" "${OUT}" "message CommandMsg"
contains "proto output has field" "${OUT}" "speed"

##
## encode / decode round trips
##
MSG="destination: 3 speed: 2.5 comment: \"hi\" active: true"

for format in hex base64 textformat; do
    run "encode as ${format}" ok "${MSG}" -- -e --format ${format} -f "${PROTO}" -m dccl.test.CommandMsg
    ENCODED="${OUT}"

    checks=$((checks + 1))
    if [ -n "${ENCODED}" ]; then
        pass "encode as ${format} produced output"
    else
        fail "encode as ${format} produced output (was empty)"
    fi

    run "decode from ${format}" ok "${ENCODED}" -- -d --format ${format} -f "${PROTO}" -m dccl.test.CommandMsg
    contains "${format} round trip keeps destination" "${OUT}" "destination: 3"
    contains "${format} round trip keeps speed" "${OUT}" "speed: 2.5"
    contains "${format} round trip keeps comment" "${OUT}" "comment: \"hi\""
    contains "${format} round trip prefixes type name" "${OUT}" "|dccl.test.CommandMsg|"

    run "decode from ${format} with --omit_prefix" ok "${ENCODED}" -- -d -o --format ${format} -f "${PROTO}" -m dccl.test.CommandMsg
    checks=$((checks + 1))
    case "${OUT}" in
        *"|dccl.test.CommandMsg|"*) fail "--omit_prefix drops the type name (still present)" ;;
        *) pass "--omit_prefix drops the type name" ;;
    esac
done

# hex encoding is what the analyze/debug workflow shows, so pin its shape
run "encode as hex for shape check" ok "${MSG}" -- -e --format hex -f "${PROTO}" -m dccl.test.CommandMsg
checks=$((checks + 1))
if echo "${OUT}" | grep -qE '^[0-9a-fA-F]+$'; then
    pass "hex output is hex digits only"
else
    fail "hex output is hex digits only (got '${OUT}')"
fi

# default format is binary
run "encode with default (binary) format" ok "${MSG}" -- -e -f "${PROTO}" -m dccl.test.CommandMsg

# the |Name| prefix picks the message per input line, instead of -m
run "encode with inline message name" ok "|dccl.test.StatusMsg| state: RUNNING count: 7" \
    -- -e --format hex -f "${PROTO}"
STATUS_ENC="${OUT}"
run "decode inline-named message" ok "${STATUS_ENC}" -- -d --format hex -f "${PROTO}"
contains "inline-named round trip keeps state" "${OUT}" "state: RUNNING"
contains "inline-named round trip keeps count" "${OUT}" "count: 7"

# blank lines in the input stream are skipped, not treated as messages
run "encode skips blank input lines" ok "

${MSG}
" -- -e --format hex -f "${PROTO}" -m dccl.test.CommandMsg
checks=$((checks + 1))
if [ "$(echo "${OUT}" | grep -c .)" -eq 1 ]; then
    pass "blank lines produce no extra output"
else
    fail "blank lines produce no extra output (got '${OUT}')"
fi

# several messages in one stream decode back one after another
run "encode two messages" ok "${MSG}
${MSG}" -- -e --format hex -f "${PROTO}" -m dccl.test.CommandMsg
run "decode two messages" ok "${OUT}" -- -d --format hex -f "${PROTO}" -m dccl.test.CommandMsg
checks=$((checks + 1))
if [ "$(echo "${OUT}" | grep -c 'destination: 3')" -eq 2 ]; then
    pass "both messages decode"
else
    fail "both messages decode (got '${OUT}')"
fi

##
## error handling
##
run "invalid --format" err "" -- --format nonsense -f "${PROTO}" -m dccl.test.CommandMsg -a
contains "invalid format is explained" "${ERR}" "Invalid format"

run "nonexistent proto file" err "" -- -a -f /nonexistent/does_not_exist.proto
contains "bad proto path is explained" "${ERR}" "Invalid proto file path"

run "unknown message name" ok "" -- -a -f "${PROTO}" -m dccl.test.NoSuchMessage
contains "unknown message is explained" "${ERR}" "No descriptor with name"

run "encode with no message specified" err "${MSG}" -- -e -m dccl.test.NoSuchMessage
contains "missing encode message is explained" "${ERR}" "You must specify a DCCL message"

run "encode with two messages" err "${MSG}" -- -e -f "${PROTO}" -m dccl.test.CommandMsg -m dccl.test.StatusMsg
contains "multiple encode messages rejected" "${ERR}" "No more than one DCCL message"

run "encode with malformed inline name" err "|dccl.test.CommandMsg destination: 3" -- -e -f "${PROTO}"
contains "malformed inline name is explained" "${ERR}" "expected '|'"

run "encode with unknown inline name" err "|dccl.test.NoSuchMsg| destination: 3" -- -e -f "${PROTO}"
contains "unknown inline name is explained" "${ERR}" "Could not load descriptor"

run "negative console width" err "" -- -a -w -5 -f "${PROTO}" -m dccl.test.CommandMsg
run "non-numeric console width" err "" -- -a -w abc -f "${PROTO}" -m dccl.test.CommandMsg
contains "non-numeric width is explained" "${ERR}" "no digits found"
run "trailing-garbage console width" err "" -- -a -w 12abc -f "${PROTO}" -m dccl.test.CommandMsg
contains "trailing garbage width is explained" "${ERR}" "additional characters"
run "overflowing console width" err "" -- -a -w 99999999999999999999999 -f "${PROTO}" -m dccl.test.CommandMsg
contains "overflow width is explained" "${ERR}" "overflow"

run "unknown id codec" err "" -- -a -i no_such_id_codec -f "${PROTO}" -m dccl.test.CommandMsg

run "nonexistent shared library" err "" -- -a -l /nonexistent/libnope.so -m dccl.test.CommandMsg

##
## summary
##
echo
echo "${checks} checks run, ${failures} failure(s)"
if [ ${failures} -ne 0 ]; then
    exit 1
fi
echo "all tests passed"
exit 0
