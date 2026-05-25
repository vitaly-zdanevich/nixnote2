#!/bin/bash
# Run non-destructive command-line smoke tests.
set -euo pipefail

CDIR=$(pwd)

function error_exit {
    echo "$0: ***********error_exit***********"
    echo "***********" 1>&2
    echo "*********** Failed: $1" 1>&2
    echo "***********" 1>&2
    cd "${CDIR}"
    exit 1
}

if [ ! -f nixnote2.pro ]; then
  error_exit "script must be run from the project root"
fi

if [ -n "${NIXNOTE2_BINARY:-}" ]; then
    BINARY="${NIXNOTE2_BINARY}"
elif [ -x ./appdir/usr/bin/nixnote2 ]; then
    BINARY=./appdir/usr/bin/nixnote2
elif [ -x ./qmake-build-release/nixnote2 ]; then
    BINARY=./qmake-build-release/nixnote2
elif [ -x ./qmake-build-debug/nixnote2 ]; then
    BINARY=./qmake-build-debug/nixnote2
else
    error_exit "no built nixnote2 binary found"
fi

TMPDIR=$(mktemp -d)
trap 'rm -rf "${TMPDIR}"' EXIT

function assert_contains {
    local file="$1"
    local expected="$2"

    if ! grep -Fq -- "${expected}" "${file}"; then
        echo "Expected output to contain: ${expected}" 1>&2
        echo "Actual output:" 1>&2
        cat "${file}" 1>&2
        exit 1
    fi
}

function assert_not_contains {
    local file="$1"
    local unexpected="$2"

    if grep -Fq -- "${unexpected}" "${file}"; then
        echo "Expected output not to contain: ${unexpected}" 1>&2
        echo "Actual output:" 1>&2
        cat "${file}" 1>&2
        exit 1
    fi
}

function run_help_smoke {
    local name="$1"
    shift
    local output="${TMPDIR}/${name}.out"

    if ! "${BINARY}" "$@" >"${output}" 2>&1; then
        echo "Command failed: ${BINARY} $*" 1>&2
        cat "${output}" 1>&2
        exit 1
    fi

    assert_contains "${output}" "usage: nixnote2 <command>"
    assert_contains "${output}" "addNote <options>"
    assert_contains "${output}" "appendNote <options>"
    assert_contains "${output}" "alterNote <options>"
    assert_contains "${output}" "signalGui <options>"
}

echo "$0: testing ${BINARY}"

run_help_smoke help-long --help
assert_contains "${TMPDIR}/help-long.out" "--display=\"<output format>\"  Output format."
assert_contains "${TMPDIR}/help-long.out" "--delimiter=\"<delimiter>\""
assert_contains "${TMPDIR}/help-long.out" "readNote options:"
assert_contains "${TMPDIR}/help-long.out" "nixnote2 closeNotebook --notebook=\"My Notebook\""
assert_not_contains "${TMPDIR}/help-long.out" "Do not show column headings.          --display"
assert_not_contains "${TMPDIR}/help-long.out" "--id=\"<title>\""
assert_not_contains "${TMPDIR}/help-long.out" "--url=<id>"

run_help_smoke help-command help
run_help_smoke help-question "--?"
run_help_smoke help-short "-?"

echo "$0: OK"
