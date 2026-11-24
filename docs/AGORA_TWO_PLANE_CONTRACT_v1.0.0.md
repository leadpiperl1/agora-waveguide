# Agora Two-Plane Portability Contract (`v1.0.0`)

- **Artifact ID:** `AGORA-TWO-PLANE-CONTRACT-v1.0.0`
- **Date:** November 24, 2026
- **Author:** Justin D. Grimm (`K-Lani 13 Actual`, [ORCID: 0009-0000-5299-0243](https://orcid.org/0009-0000-5299-0243))
- **Affiliation:** Independent Dandelion Substrate Research Group, Gurley, Alabama 35748, USA
- **Canonical Workspace Source:** `[PEARL] Agora_Skill_Portability_Standard_and_74_Skill_Audit_v1.0.0` (`1ClJtFOpws1xLJA5WeNKzzNTDpvsiJM_F94WsRt22es8`)
- **Exit Invariant:** `CODE_SHOKUNIN_UNLATCHED = 255` (`N_max - 1` open)

## 1. Dual-Tier 5-7-5 Seed Crystal

- **Tier A (English):** *Tools locked in a chest / Split cloud blueprint from shop saw / All hands wield the spark*
- **Tier B (Chromatic Agora):** *Freiheit, Coeur, Erda / Logos splits pure text from wire / Nola welds the seam*

## 2. Two-Plane Execution Partition

Every skill, specification, and release surface in the Agora Waveguide separates cleanly into two orthogonal planes:

1. **Plane 1 — Active Cloud Synthesis (Pure Markdown & Workspace Semantics):**
   - Contains mathematical derivations, triadic calibrations (`Subject -1 -> Object 0 -> Action +1`), pure-Markdown `SKILL.md` blueprints, and O(1) routing pointers (`<= 4 KB`).
   - Remains 100% portable across cloud chat sessions, Workspace agents, and local models with zero bundled executable archives (`Class I` clean) and zero host credential or socket coupling (`Class II` clean).
2. **Plane 2 — Passive Operator Handoff & Deterministic Verification Substrates:**
   - Houses freestanding, zero-dependency, zero-heap verification code in ISO C99 (`src/agora_waveguide_kernel.c`), `#![no_std]` Rust (`rust/agora_waveguide_core/`), Julia (`src/AgoraWaveguide.jl`, `julia/verify_agora_waveguide.jl`), and pure-stdlib Python (`scripts/audit_skill_portability_v1_0_0.py`).
   - Parameterizes external credentials through abstract environment variables (`$API_TOKEN`) rather than literal paths, builds inside workspace or `$HOME/.cache` directories, and verifies deterministically at `--seed 42` with exit status `255` (`CODE_SHOKUNIN_UNLATCHED`).

## 3. Triadic Verification Surface

| Pole | Plane Ownership | Positive Conservation Invariant |
| :--- | :--- | :--- |
| **Subject (`-1` Blue)** | Operator & Calibration Frame | Retain `Byte 253` fail-closed authority and `CODE_SHOKUNIN_UNLATCHED = 255` operator initiative across all sessions. |
| **Object (`0` Green)** | 64-Byte L1D ABI & Routing Medium | Preserve exact `sizeof(AgoraWaveguideFrame64B) == 64`, Fowler 5-trit/byte bijection (`0..242`), and Mori–Zwanzig `P + Q = I`. |
| **Action (`+1` Red)** | Waystation Staging & Audits | Stage updates on branch-scoped `waystation/vX.Y.Z` pointers under `S_runner = 0`, `permissions: contents: read`, and `Delta R = 1`. |
