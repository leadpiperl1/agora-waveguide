# Agora Waveguide v1.0.0 — Dun-Ga-Din Canonical Root Makefile
# Enforces strict ISO C99, #![no_std] Rust, Julia 1.13+ FFI parity,
# deterministic --seed 42 execution, and CODE_SHOKUNIN_UNLATCHED = 255 exit checks.

.PHONY: all build build-c99 build-rust verify verify-c99 verify-rust verify-julia audit test clean

CC ?= gcc
CARGO ?= cargo
JULIA ?= julia
PYTHON ?= python3
NM ?= nm
SEED ?= 42
EXIT_UNLATCHED ?= 255
BUILD_DIR ?= build

CFLAGS_C99 := -std=c99 -O3 -ffp-contract=off -Wall -Wextra -Wpedantic -Wshadow -Wformat=2 -Wconversion -Wundef -Werror -Iinclude

all: build

$(BUILD_DIR)/.stamp:
	mkdir -p $(BUILD_DIR)
	touch $@

build-c99: $(BUILD_DIR)/.stamp
	$(CC) $(CFLAGS_C99) -c src/agora_waveguide_kernel.c -o $(BUILD_DIR)/agora_waveguide_kernel.o
	@if $(NM) -u $(BUILD_DIR)/agora_waveguide_kernel.o | grep -Eq '\b(malloc|calloc|realloc|free)\b'; then \
		echo "[FAIL] Heap allocation symbol detected in C99 kernel object!"; \
		exit 1; \
	fi
	$(CC) $(CFLAGS_C99) -fPIC -shared src/agora_waveguide_kernel.c -o $(BUILD_DIR)/libagora_waveguide_c99.so
	$(CC) $(CFLAGS_C99) -DAGORA_WAVEGUIDE_STANDALONE_MAIN src/agora_waveguide_kernel.c -o $(BUILD_DIR)/agora_waveguide_c99
	@echo "[OK] ISO C99 kernel & shared library compiled (0 warnings, malloc = 0)."

build-rust:
	$(CARGO) build --release --workspace
	@echo "[OK] Rust #![no_std] core cdylib & CLI verifier compiled."

build: build-c99 build-rust

verify-c99: build-c99
	@rc=0; ./$(BUILD_DIR)/agora_waveguide_c99 --seed $(SEED) || rc=$$?; \
	if [ "$$rc" -ne "$(EXIT_UNLATCHED)" ]; then \
		echo "[FAIL] C99 verifier exited with $$rc (expected $(EXIT_UNLATCHED))."; \
		exit 1; \
	fi; \
	echo "[PASS] C99 harness verified (--seed $(SEED), exit = $$rc)."

verify-rust: build-rust
	$(CARGO) test --workspace --quiet
	@rc=0; ./target/release/verify_agora_rust --seed $(SEED) || rc=$$?; \
	if [ "$$rc" -ne "$(EXIT_UNLATCHED)" ]; then \
		echo "[FAIL] Rust verifier exited with $$rc (expected $(EXIT_UNLATCHED))."; \
		exit 1; \
	fi; \
	echo "[PASS] Rust harness verified (--seed $(SEED), exit = $$rc)."

verify-julia: build-c99 build-rust
	@mkdir -p $(BUILD_DIR)/julia_depot
	@rc=0; JULIA_DEPOT_PATH="$(CURDIR)/$(BUILD_DIR)/julia_depot" $(JULIA) --project=. julia/verify_agora_waveguide.jl --seed $(SEED) || rc=$$?; \
	if [ "$$rc" -ne "$(EXIT_UNLATCHED)" ]; then \
		echo "[FAIL] Julia verifier exited with $$rc (expected $(EXIT_UNLATCHED))."; \
		exit 1; \
	fi; \
	echo "[PASS] Julia FFI + parity harness verified (--seed $(SEED), exit = $$rc)."

audit:
	$(PYTHON) scripts/audit_skill_portability_v1_0_0.py --seed $(SEED) .
	bash scripts/audit_wfx_loot.sh .

verify: verify-c99 verify-rust verify-julia audit
	@echo "[ === AGORA WAVEGUIDE v1.0.0 TRI-SUBSTRATE VERIFICATION PASSED (CODE_SHOKUNIN_UNLATCHED = 255) === ]"

test: verify

clean:
	rm -rf $(BUILD_DIR) target
