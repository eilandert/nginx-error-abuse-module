/*
 * Minimal nginx shim for fuzzing ngx_http_error_abuse_validate_snapshot().
 *
 * The real ngx_http_error_abuse_module.c pulls in <ngx_config.h>/
 * <ngx_core.h>/<ngx_http.h> plus hiredis — the whole nginx tree. The
 * snapshot validator only touches a tiny, well-defined slice of that
 * surface (a couple of integer typedefs, the two on-disk struct layouts,
 * zone->threshold and ngx_memcpy), so we reproduce just that slice here
 * with the EXACT upstream semantics. The fuzz target then includes the
 * verbatim function body sliced out of the shipped .c, so we fuzz the
 * SHIPPED code — not a re-implementation.
 *
 * If the module ever changes these typedefs or the two on-disk struct
 * layouts, this shim must be updated to match (the .inc compile will fail
 * loudly if a referenced name disappears).
 */

#ifndef NGX_ERROR_ABUSE_FUZZ_SHIM_H
#define NGX_ERROR_ABUSE_FUZZ_SHIM_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef intptr_t   ngx_int_t;
typedef uintptr_t  ngx_uint_t;
typedef unsigned char u_char;

#define NGX_OK     0
#define NGX_ERROR -1

#define ngx_memcpy(dst, src, n)  (void) memcpy(dst, src, n)

/*
 * On-disk record layout — verbatim copy of the struct in
 * ../ngx_http_error_abuse_module.c. The field sizes/order define how
 * sizeof(record) and the payload arithmetic behave, so they MUST stay
 * byte-for-byte identical to production.
 */
typedef struct {
    uint16_t  key_len;
    uint16_t  event_count;
    int64_t   blocked_until;
    int64_t   last_seen;
} ngx_http_error_abuse_file_record_t;

/*
 * Only the field the validator reads (zone->threshold) is reproduced;
 * its type (ngx_uint_t) matches production so the
 * `record.event_count > zone->threshold` comparison promotes identically.
 */
typedef struct {
    ngx_uint_t  threshold;
} ngx_http_error_abuse_zone_t;

#endif /* NGX_ERROR_ABUSE_FUZZ_SHIM_H */
