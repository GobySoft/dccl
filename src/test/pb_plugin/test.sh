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

# Tests protoc-gen-dccl, the protoc plugin that adds boost::units accessors to
# generated protobuf classes and (optionally) writes a loader .cpp.
#
# Usage: test.sh <protoc> <protoc-gen-dccl> <proto source dir> <dccl include dir> <work dir>

set -u

PROTOC="$1"
PLUGIN="$2"
PROTO_DIR="$3"
INC_DIR="$4"
WORK_DIR="$5"

rm -rf "${WORK_DIR}"
mkdir -p "${WORK_DIR}"

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

# generate <output subdir> <extra --dccl_out prefix> <proto file>
generate()
{
    local outdir="${WORK_DIR}/$1" prefix="$2" proto="$3"
    mkdir -p "${outdir}"
    "${PROTOC}" \
        --cpp_out="${outdir}" \
        --dccl_out="${prefix}${outdir}" \
        --plugin=protoc-gen-dccl="${PLUGIN}" \
        -I "${PROTO_DIR}" -I "${INC_DIR}" -I /usr/include \
        "${PROTO_DIR}/${proto}" 2>&1
}

contains()
{
    checks=$((checks + 1))
    if grep -qF -- "$3" "$2"; then
        pass "$1"
    else
        fail "$1 (expected '$3' in $2)"
    fi
}

##
## the happy path: units accessors are added to the generated header
##
checks=$((checks + 1))
OUT="$(generate units "" units.proto)"
if [ $? -eq 0 ]; then
    pass "generating units.proto succeeds"
else
    fail "generating units.proto succeeds (${OUT})"
fi

HDR="${WORK_DIR}/units/units.pb.h"

checks=$((checks + 1))
if [ -f "${HDR}" ]; then
    pass "the generated header exists"
else
    fail "the generated header exists"
    echo "${checks} checks run, ${failures} failure(s)"
    exit 1
fi

contains "boost units headers are included" "${HDR}" "#include <boost/units/quantity.hpp>"
contains "absolute.hpp is included for temperatures" "${HDR}" "#include <boost/units/absolute.hpp>"

# every units field gets a dimension typedef, a unit typedef, and a
# _with_units getter/setter pair
for field in distance speed depth temperature conductivity salinity; do
    contains "${field} gets a unit typedef" "${HDR}" "${field}_unit"
    contains "${field} gets a with_units setter" "${HDR}" "set_${field}_with_units"
    contains "${field} gets a with_units getter" "${HDR}" "${field}_with_units"
done

contains "base_dimensions produces a derived_dimension" "${HDR}" "distance_dimension"
contains "the message unit_system is applied" "${HDR}" "boost::units::si::system"
contains "a prefixed unit is scaled" "${HDR}" "make_scaled_unit"
contains "a relative temperature is generated" "${HDR}" "temperature_unit"
contains "a custom unit pulls in its own header" "${HDR}" "dccl/units/conductivity.h"
contains "a custom unit is used directly" "${HDR}" "dccl::units::microsiemens_per_cm_unit"

# a per-field system overrides the message-level one
contains "a per-field system is applied" "${HDR}" "degree"

# a field with no units must not gain accessors
checks=$((checks + 1))
if grep -q "plain_with_units" "${HDR}"; then
    fail "a field without units gets no units accessors"
else
    pass "a field without units gets no units accessors"
fi

##
## the loader file
##
LOAD_CPP="${WORK_DIR}/dccl_load.cpp"
: > "${LOAD_CPP}"

checks=$((checks + 1))
OUT="$(generate loadfile "dccl3_load_file=${LOAD_CPP}:" units.proto)"
if [ $? -eq 0 ]; then
    pass "generating with dccl3_load_file succeeds"
else
    fail "generating with dccl3_load_file succeeds (${OUT})"
fi

contains "the loader defines dccl3_load" "${LOAD_CPP}" "void dccl3_load(dccl::Codec* dccl)"
contains "the loader defines dccl3_unload" "${LOAD_CPP}" "void dccl3_unload(dccl::Codec* dccl)"
contains "the loader includes the generated header" "${LOAD_CPP}" "units.pb.h"
contains "the loader registers a DCCL message" "${LOAD_CPP}" "DCCLLoader"
contains "the loader registers UnitsMsg" "${LOAD_CPP}" "UnitsMsg"

##
## error handling
##
# Pinning current behavior, not desired behavior: DCCLGenerator has a
# check_field_type() that raises "Can only use (dccl.field).base_dimensions on
# numeric fields", but nothing calls it, and get_field_type_name() falls back
# to "double" for any other type. So units on a string field generate a
# set_<field>_with_units() that calls set_<field>(double) and the resulting
# header does not compile. If check_field_type() is ever wired up, this test
# should flip to expecting a clean rejection.
checks=$((checks + 1))
OUT="$(generate bad "" bad_units.proto)"
if [ $? -eq 0 ]; then
    pass "units on a non-numeric field are currently accepted (see comment)"
else
    fail "units on a non-numeric field are currently accepted (generation failed: ${OUT})"
fi

BAD_HDR="${WORK_DIR}/bad/bad_units.pb.h"
contains "the string field gets units accessors it cannot support" \
         "${BAD_HDR}" "set_name_with_units"
contains "and they pass a double to a string setter" \
         "${BAD_HDR}" "boost::units::quantity<name_unit,double >"

checks=$((checks + 1))
OUT="$(generate badparam "not_a_real_parameter=1:" units.proto)"
if [ $? -ne 0 ]; then
    pass "an unknown plugin parameter is rejected"
else
    fail "an unknown plugin parameter is rejected (generation succeeded)"
fi
checks=$((checks + 1))
case "${OUT}" in
    *"Unknown parameter"*) pass "the unknown parameter error names the parameter" ;;
    *) fail "the unknown parameter error names the parameter (got: ${OUT})" ;;
esac

checks=$((checks + 1))
OUT="$(generate badload "dccl3_load_file=/nonexistent/dir/load.cpp:" units.proto)"
if [ $? -ne 0 ]; then
    pass "an unopenable load file is rejected"
else
    fail "an unopenable load file is rejected (generation succeeded)"
fi

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
