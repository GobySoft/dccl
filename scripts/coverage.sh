#!/bin/bash

# Configures, builds, and tests DCCL with coverage instrumentation, then writes
# an HTML report (build-coverage/coverage/index.html) plus a terminal summary.
#
# Usage: ./scripts/coverage.sh [--xml] [--no-build] [-- <extra cmake flags>]
#
#   --xml       also emit Cobertura XML (coverage.xml) for CI upload
#   --no-build  reuse the existing build-coverage tree, just re-run and re-report
#
# Requires gcovr (apt install gcovr).

set -e -u

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC_DIR="$(dirname "${SCRIPT_DIR}")"
BUILD_DIR="${SRC_DIR}/build-coverage"

WANT_XML=false
DO_BUILD=true
EXTRA_CMAKE_FLAGS=()

while [[ $# -gt 0 ]]; do
    case "$1" in
        --xml) WANT_XML=true; shift ;;
        --no-build) DO_BUILD=false; shift ;;
        --) shift; EXTRA_CMAKE_FLAGS=("$@"); break ;;
        *) echo "unknown option: $1" >&2; exit 1 ;;
    esac
done

if ! command -v gcovr >/dev/null 2>&1; then
    echo "error: gcovr not found (apt install gcovr)" >&2
    exit 1
fi

if [ "${DO_BUILD}" = true ]; then
    mkdir -p "${BUILD_DIR}"
    cd "${BUILD_DIR}"
    cmake "${SRC_DIR}" \
          -DCMAKE_BUILD_TYPE=Debug \
          -Denable_testing=ON \
          -Denable_coverage=ON \
          -DADD_SYMLINKS=OFF \
          ${EXTRA_CMAKE_FLAGS[@]+"${EXTRA_CMAKE_FLAGS[@]}"}
    cmake --build . -- -j"$(nproc)"
fi

cd "${BUILD_DIR}"

# Stale counters from a previous run would inflate the totals.
find . -name '*.gcda' -delete

# A failing test still leaves usable counters, so report either way.
ctest --output-on-failure -j"$(nproc)" || echo "WARNING: some tests failed; coverage reported anyway" >&2

mkdir -p coverage

# Generated protobuf sources and vendored third-party code are not ours to test.
GCOVR_ARGS=(
    --root "${SRC_DIR}"
    --filter "${SRC_DIR}/src/"
    --exclude '.*/src/test/.*'
    --exclude '.*/src/thirdparty/.*'
    --exclude '.*\.pb\.(cc|h)$'
    --exclude-unreachable-branches
    --exclude-throw-branches
    # the multithread test trips a known gcov counter bug (gcc PR68080)
    --gcov-ignore-parse-errors negative_hits.warn_once_per_file
    --print-summary
)

gcovr "${GCOVR_ARGS[@]}" --html-details coverage/index.html

if [ "${WANT_XML}" = true ]; then
    gcovr "${GCOVR_ARGS[@]}" --xml-pretty --output coverage.xml
    echo "Cobertura XML: ${BUILD_DIR}/coverage.xml"
fi

echo
echo "HTML report: ${BUILD_DIR}/coverage/index.html"
