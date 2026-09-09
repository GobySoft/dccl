#!/bin/bash

# Configures, builds, and tests DCCL with coverage instrumentation, then writes
# an HTML report (build-coverage/coverage/index.html) plus a terminal summary.
#
# Usage: ./scripts/coverage.sh [--xml] [--no-build] [--all-tests] [-- <extra cmake flags>]
#
#   --xml        also emit Cobertura XML (coverage.xml) for CI upload
#   --no-build   reuse the existing build-coverage tree, just re-run and re-report
#   --all-tests  also run the tests excluded by default (see SLOW_TESTS below)
#
# Requires gcovr and python3-lxml (gcovr's Cobertura writer imports lxml, and
# the gcovr package does not depend on it).

set -e -u

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC_DIR="$(dirname "${SCRIPT_DIR}")"
BUILD_DIR="${SRC_DIR}/build-coverage"

# Instrumented counters are shared mutable state, so a heavily threaded test
# spends its time contending on them: dccl_test_multithread takes ~5s in a
# normal build and ~390s under --coverage, which dominates the whole job.
# Skipping it here costs about 0.3% of line coverage; it still runs in every
# other CI job, including the thread sanitizer one that exists to police it.
SLOW_TESTS='dccl_test_multithread'

# A hung test should fail the job quickly rather than sit until the CI limit.
CTEST_TIMEOUT=600

WANT_XML=false
DO_BUILD=true
RUN_ALL_TESTS=false
EXTRA_CMAKE_FLAGS=()

while [[ $# -gt 0 ]]; do
    case "$1" in
        --xml) WANT_XML=true; shift ;;
        --no-build) DO_BUILD=false; shift ;;
        --all-tests) RUN_ALL_TESTS=true; shift ;;
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

CTEST_ARGS=(--output-on-failure -j"$(nproc)" --timeout "${CTEST_TIMEOUT}")
if [ "${RUN_ALL_TESTS}" = false ]; then
    CTEST_ARGS+=(--exclude-regex "${SLOW_TESTS}")
    echo "NOTE: excluding tests matching '${SLOW_TESTS}' (very slow under coverage);"
    echo "      pass --all-tests to include them."
fi

# A failing test still leaves usable counters, so report either way.
ctest "${CTEST_ARGS[@]}" || echo "WARNING: some tests failed; coverage reported anyway" >&2

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
