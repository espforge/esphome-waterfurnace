#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."
ROOT="$PWD"

RED='\033[0;31m'
GREEN='\033[0;32m'
BOLD='\033[1m'
RESET='\033[0m'

PASS=0
FAIL=0
RESULTS=()

run_test() {
  local name="$1"
  shift
  printf "${BOLD}=== %s ===${RESET}\n" "$name"
  if "$@"; then
    RESULTS+=("${GREEN}PASS${RESET}  $name")
    ((PASS++))
  else
    RESULTS+=("${RED}FAIL${RESET}  $name")
    ((FAIL++))
  fi
  echo
}

# Unit tests (gtest)
run_test "Unit tests" make -C "${ROOT}/tests/unit" test

# Integration tests
run_test "Integration tests" bash -c '
  cd tests
  docker compose up --build --abort-on-container-exit --exit-code-from test
'

# ESPHome config validation (uses tests/waterfurnace-test.yaml with local components)
run_test "ESPHome config validation" \
  docker run --rm \
    -v "${ROOT}":/config \
    -w /config/tests \
    ghcr.io/esphome/esphome config waterfurnace-test.yaml

# ESPHome compile (requires at least one git commit for ESP-IDF cmake)
run_test "ESPHome compile" \
  docker run --rm \
    -v "${ROOT}":/config \
    -w /config/tests \
    ghcr.io/esphome/esphome compile waterfurnace-test.yaml

# Example config validation (validates root config with local components)
run_test "Example config validation" \
  docker run --rm \
    -v "${ROOT}":/config \
    -v "${ROOT}/tests/secrets.yaml":/config/secrets.yaml \
    -w /config \
    ghcr.io/esphome/esphome -s component_source components config waterfurnace-esp32-s3.yaml

# Summary
echo
printf "${BOLD}================================${RESET}\n"
printf "${BOLD}Test Summary${RESET}\n"
printf "${BOLD}================================${RESET}\n"
for r in "${RESULTS[@]}"; do
  printf "  %b\n" "$r"
done
echo
printf "${BOLD}Total: %d passed, %d failed${RESET}\n" "$PASS" "$FAIL"

[ "$FAIL" -eq 0 ]
