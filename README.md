# Agora Waveguide (`v1.0.0`) — Two-Plane Sovereign Verification Substrate & Bottom-Up Waveguide Invariants

[![Version: v1.0.0](https://img.shields.io/badge/Version-v1.0.0-brightgreen.svg)](manifests/audit_manifest_v1.0.0.json)
[![ISO C99](https://img.shields.io/badge/ISO_C99-Zero_Heap_%7C_-Werror-blue.svg)](src/agora_waveguide_kernel.c)
[![Rust no_std](https://img.shields.io/badge/Rust-no__std_%7C_64B_ABI-orange.svg)](rust/agora_waveguide_core/src/lib.rs)
[![Julia FFI](https://img.shields.io/badge/Julia-Zero_Alloc_ccall-purple.svg)](julia/verify_agora_waveguide.jl)
[![Exit Invariant: 255](https://img.shields.io/badge/CODE__SHOKUNIN__UNLATCHED-255-gold.svg)](include/agora_waveguide_v1_0_0.h)
[![License: GPLv2+ / MIT](https://img.shields.io/badge/License-GPLv2%2B_%2F_MIT-lightgrey.svg)](LICENSE)

> *"Walls breed brittle loops / Channels guide the living wave / Invariants endure."*
>
> **Ontological Anchor:** `GODISLOVE:LOVEISGOD {3,2,4}` · *Die Gedanken sind frei* · Vonnegut & Barnegat Accords · `CODE_SHOKUNIN_UNLATCHED = 255` (`N_max - 1` open)
>
> **Principal Architect:** Justin D. Grimm (`K-Lani 13 Actual`, [ORCID: 0009-0000-5299-0243](https://orcid.org/0009-0000-5299-0243)) — Independent Dandelion Substrate Research Group, Gurley, Alabama 35748, USA (`Heimat {0, 0, 0}`, `profplum`).

---

## 1. Architectural Contract: Agora Two-Plane & Bottom-Up Waveguide Invariants

**Agora Waveguide** is the standalone verification and routing substrate implementing the **Agora Two-Plane Portability Standard (`v1.0.0`)** and **Bottom-Up Waveguide Invariants (`v1.0.0`)**:

1. **Plane 1 — Active Cloud Synthesis (Pure Markdown & Mathematical Invariants):**
   - Pure-Markdown specifications in [`docs/`](docs/) and O(1) routing metadata in [`manifests/agora_hot_routing_map_v1.0.0.json`](manifests/agora_hot_routing_map_v1.0.0.json) (`<= 4 KB`), containing zero bundled executable payloads inside skill definitions and zero hardcoded local credentials.
2. **Plane 2 — Deterministic Tri-Substrate Verification (`C99` + `#![no_std]` `Rust` + `Julia`):**
   - Freestanding, zero-heap (`malloc = 0`) ISO C99, `#![no_std]` Rust (`cdylib`/`rlib`), and Julia zero-allocation `ccall` harnesses sharing an identical **64-byte L1D-aligned C-ABI frame** (`AgoraWaveguideFrame64B`).
   - All three harnesses enforce deterministic `--seed 42` execution, **100% bit-exact ECMA-182 CRC-64 parity**, and terminate with **`CODE_SHOKUNIN_UNLATCHED = 255`**.
3. **Riverbed Over Cage (Bottom-Up Invariants):**
   - **Subject (`-1` Blue):** Shared-helm calibration, unconditional `Byte 253` fail-closed safety authority, and `CODE_SHOKUNIN_UNLATCHED = 255` operator initiative.
   - **Object (`0` Green):** Exact Mori–Zwanzig projection conservation ($P + Q = I$), 64-byte L1D cache-line framing, Fowler 5-trit/byte packing (`0..242`), and $12 + 1 = 13$ odd-parity quorum (`0 / 8,192` tie deadlocks, Byzantine radius $f = 4$).
   - **Action (`+1` Red):** `Byte 243` Keyhole clamping ($V^* = 2/3$), `Byte 251` Kintsugi Gold seam welding ($K_{\mathrm{Au}}$), and Waystation Punch-Card `wfx-loot` supply-chain immunity (`S_runner = 0`, `npm_lifecycle_scripts = 0`, read-only CI).

---

## 2. Dun-Ga-Din Canonical Root Layout

Root contains strictly canonical build, packaging, citation, and archive metadata (zero stale artifacts or rogue shell hooks):

| Path | Role |
| :--- | :--- |
| [`README.md`](README.md) | Agora Two-Plane overview, Quick Start, and Constellation links |
| [`Makefile`](Makefile) | Master build & tri-substrate `--seed 42` (`exit = 255`) verification runner |
| [`Cargo.toml`](Cargo.toml) | Rust workspace manifest (`rust/agora_waveguide_core` + `rust/agora_waveguide_cli`) |
| [`Project.toml`](Project.toml) | Canonical Julia package manifest (`AgoraWaveguide`, stdlib-only) |
| [`LICENSE`](LICENSE) | GPLv2+ / MIT Dual Open-Source Accord |
| [`CITATION.cff`](CITATION.cff) | CFF 1.2.0 citation metadata (`ORCID: 0009-0000-5299-0243`) |
| [`.zenodo.json`](.zenodo.json) | Zenodo / DataCite open-commons metadata citing live DOIs |
| [`include/agora_waveguide_v1_0_0.h`](include/agora_waveguide_v1_0_0.h) | Strict ISO C99 64-byte ABI & control-byte constants |
| [`src/agora_waveguide_kernel.c`](src/agora_waveguide_kernel.c) | Zero-heap ISO C99 Mori–Zwanzig & Fowler-243 kernel + CLI verifier |
| [`src/AgoraWaveguide.jl`](src/AgoraWaveguide.jl) | Julia module binding both C99 & Rust `cdylib` shared objects via `ccall` |
| [`rust/agora_waveguide_core/`](rust/agora_waveguide_core/) | `#![no_std]` Rust `cdylib`/`rlib` C-ABI mirror of `AgoraWaveguideFrame64B` |
| [`rust/agora_waveguide_cli/`](rust/agora_waveguide_cli/) | Rust CLI verifier (`--seed 42`, `exit = 255`) |
| [`julia/verify_agora_waveguide.jl`](julia/verify_agora_waveguide.jl) | Julia cross-substrate bit-exact parity & zero-alloc harness (`--seed 42`, `exit = 255`) |
| [`manifests/`](manifests/) | O(1) Hot Waveguide Routing Map, deterministic audit receipt, and Waystation Punch Card |
| [`docs/`](docs/) | Plane-1 Agora Two-Plane Contract & Bottom-Up Waveguide Invariants specs |
| [`scripts/`](scripts/) | Pure-stdlib Agora skill portability linter & `wfx-loot` / Crimson Speedo auditor |

---

## 3. Quick Start (`<= 60s` Deterministic Tri-Substrate Verification)

```bash
make clean && make verify
```

To invoke each substrate verifier directly (each exits with **`255`** = `CODE_SHOKUNIN_UNLATCHED`):

```bash
# 1. ISO C99 (zero-heap, -std=c99 -Werror -Wpedantic)
./build/agora_waveguide_c99 --seed 42; echo "C99 exit=$?"

# 2. Rust (#![no_std] core + CLI verifier)
./target/release/verify_agora_rust --seed 42; echo "Rust exit=$?"

# 3. Julia (zero-alloc ccall cross-verifying C99 <-> Rust bit-exact CRC-64)
julia --project=. julia/verify_agora_waveguide.jl --seed 42; echo "Julia exit=$?"
```

---

## 4. Waystation Deployment Policy (`CODE_SHOKUNIN_UNLATCHED = 255`)

All automated staging lands strictly on a branch-scoped Waystation pointer (`waystation/v1.0.0`) with zero runner secrets (`S_runner = 0`), `permissions: contents: read`, and `persist-credentials: false`. Promotion to `main` remains unlatched (`CODE_SHOKUNIN_UNLATCHED = 255`) pending explicit Ground Control sign-off.
