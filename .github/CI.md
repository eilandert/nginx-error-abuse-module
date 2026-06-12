# Continuous integration

The CI pipeline is intentionally split by failure class:

| Workflow | Required check | Coverage |
|---|---|---|
| `build-test.yml` | `Validation` | workflow lint, shellcheck, Python syntax, cppcheck |
| `build-test.yml` | `Build` | nginx 1.31.1, strict warnings-as-errors compile |
| `build-test.yml` | `Runtime` | multi-worker behavior, two-host Redis aggregation, snapshots and restart restore |
| `build-test.yml` | `ASan and UBSan` | memory safety and undefined behavior |
| `valgrind.yml` | `Memcheck` | uninitialized reads, invalid memory access, and definite/indirect leaks (`--errors-for-leak-kinds=definite,indirect`) |
| `codeql.yml` | `Analyze C` | CodeQL security-extended C/C++ queries |
| `security-scanners.yml` | `Scan` | flawfinder (SARIF + high-severity gate), clang-tidy (`cert-*`, `bugprone-*`, `clang-analyzer-security.*`), Semgrep (`p/c`, `p/security-audit`) → Code Scanning |
| `fuzzing.yml` | scheduled | monthly libFuzzer run of the parse targets |

All third-party actions are pinned to immutable commit SHAs. Workflows use
read-only repository permissions except CodeQL, which additionally receives
`security-events: write`.

## Local commands

```bash
# Build nginx mainline and the dynamic module.
bash tools/ci-build.sh nginx 1.31.1

# Native multi-worker runtime suite.
python3 tools/test_runtime.py \
  --nginx-binary .build/nginx-1.31.1/objs/nginx \
  --module .build/nginx-1.31.1/objs/ngx_http_error_abuse_module.so \
  --redis-server /usr/bin/redis-server

# ASan and UBSan.
bash tools/ci-build.sh nginx 1.31.1 asan
ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 \
UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
python3 tools/test_runtime.py --single-process \
  --redis-server /usr/bin/redis-server \
  --nginx-binary .build/nginx-1.31.1/objs/nginx

# Valgrind.
python3 tools/test_runtime.py --single-process \
  --runner "valgrind --tool=memcheck --track-origins=yes --error-exitcode=99" \
  --redis-server /usr/bin/redis-server \
  --nginx-binary .build/nginx-1.31.1/objs/nginx \
  --module .build/nginx-1.31.1/objs/ngx_http_error_abuse_module.so
```
