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

function assert_empty {
    local file="$1"

    if [ -s "${file}" ]; then
        echo "Expected output to be empty, got:" 1>&2
        cat "${file}" 1>&2
        exit 1
    fi
}

function assert_file_equals {
    local expected="$1"
    local actual="$2"

    if ! cmp -s "${expected}" "${actual}"; then
        echo "Expected output:" 1>&2
        cat "${expected}" 1>&2
        echo "Actual output:" 1>&2
        cat "${actual}" 1>&2
        diff -u "${expected}" "${actual}" 1>&2 || true
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

PROGRAM_DATA_DIR="${CDIR}/appdir/usr/share/nixnote2"
if [ -d "${PROGRAM_DATA_DIR}" ]; then
    if ! "${BINARY}" query \
        --configDir="${TMPDIR}/quiet-config" \
        --userDataDir="${TMPDIR}/quiet-data" \
        --programDataDir="${PROGRAM_DATA_DIR}" \
        --search='intitle:"NixNote smoke test"' \
        --display='%i|%t' \
        --noHeaders \
        >"${TMPDIR}/query-quiet.out" 2>"${TMPDIR}/query-quiet.err"; then
        echo "Command failed: ${BINARY} query" 1>&2
        cat "${TMPDIR}/query-quiet.err" 1>&2
        exit 1
    fi

    assert_empty "${TMPDIR}/query-quiet.err"

    if command -v sqlite3 >/dev/null; then
        READNOTE_CONFIG="${TMPDIR}/readnote-config"
        READNOTE_DATA="${TMPDIR}/readnote-data"
        mkdir -p "${READNOTE_CONFIG}" "${READNOTE_DATA}/db-2"
        printf '[SaveState]\nlastAccessedAccount=2\ndatabaseVersion=2\n' \
            >"${READNOTE_CONFIG}/nixnote.conf"

        sqlite3 "${READNOTE_DATA}/db-2/nixnote.db" <<'SQL'
CREATE TABLE NoteTable (lid INTEGER PRIMARY KEY, title TEXT);
CREATE TABLE DataStore (lid integer, key integer, data blob default null collate nocase);
INSERT INTO NoteTable (lid, title) VALUES (1, 'CLI text smoke');
INSERT INTO DataStore (lid, key, data)
VALUES (1, 5000, '00000000-0000-4000-8000-000000000001');
INSERT INTO DataStore (lid, key, data)
VALUES (1, 5002, '<!DOCTYPE en-note SYSTEM "http://xml.evernote.com/pub/enml2.dtd"><en-note><div>First line</div><div><br/></div><div>Second line</div><div>Third line</div></en-note>');
SQL

        if ! "${BINARY}" readNote \
            --id=1 \
            --accountId=2 \
            --configDir="${READNOTE_CONFIG}" \
            --userDataDir="${READNOTE_DATA}" \
            --programDataDir="${PROGRAM_DATA_DIR}" \
            >"${TMPDIR}/readnote-format.out" 2>"${TMPDIR}/readnote-format.err"; then
            echo "Command failed: ${BINARY} readNote" 1>&2
            cat "${TMPDIR}/readnote-format.err" 1>&2
            exit 1
        fi

        assert_empty "${TMPDIR}/readnote-format.err"
        printf 'First line\n\nSecond line\nThird line\n' >"${TMPDIR}/readnote-format.expected"
        assert_file_equals "${TMPDIR}/readnote-format.expected" "${TMPDIR}/readnote-format.out"
    fi
fi

echo "$0: OK"
