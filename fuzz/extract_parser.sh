#!/usr/bin/env bash
#
# Slice the verbatim body of ngx_http_error_abuse_validate_snapshot() out
# of the shipped ../ngx_http_error_abuse_module.c into
# generated_parser.inc. That function is the security gate that walks the
# untrusted on-disk snapshot with pointer arithmetic against p/last before
# load() consumes it.
#
# This keeps the fuzz target locked to production code: there is no
# hand-maintained copy of the validator. If the signature or body changes
# upstream, the next fuzz build picks it up automatically. If the function
# can no longer be found, we fail loudly rather than fuzz nothing.

set -euo pipefail

FUZZ_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC="$FUZZ_DIR/../ngx_http_error_abuse_module.c"
OUT="$FUZZ_DIR/generated_parser.inc"

if [ ! -f "$SRC" ]; then
    echo "✗ cannot find $SRC" >&2
    exit 1
fi

# Capture from the `static ngx_int_t` return-type line whose following
# definition line is ngx_http_error_abuse_validate_snapshot(, through the
# matching closing brace in column 1 (nginx style: definitions close with
# a bare `}`).
awk '
    /^static ngx_int_t$/ { pending = 1; buf = $0 ORS; next }
    pending && /^ngx_http_error_abuse_validate_snapshot\(/ {
        capture = 1; pending = 0; print buf; print; next
    }
    pending { pending = 0; buf = "" }
    capture {
        print
        if ($0 == "}") { capture = 0 }
    }
' "$SRC" > "$OUT"

if ! grep -q 'ngx_http_error_abuse_validate_snapshot' "$OUT" \
   || [ "$(tail -n1 "$OUT")" != "}" ]; then
    echo "✗ failed to extract validate_snapshot() from $SRC" >&2
    echo "  (source layout changed? update extract_parser.sh)" >&2
    rm -f "$OUT"
    exit 1
fi

LINES=$(wc -l < "$OUT")
echo "✓ extracted ngx_http_error_abuse_validate_snapshot() — $LINES lines -> $OUT"
