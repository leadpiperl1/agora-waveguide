#!/usr/bin/env bash
# scripts/audit_wfx_loot.sh (v1.3.0)
# Read-only wfx-loot IOC, Crimson Speedo Gate, SHA-pinning, and dependabot-cooldown >= 14 audit.
set -euo pipefail

REPO_DIR="${1:-.}"
cd "${REPO_DIR}"

echo "=== [1/3] Crimson Speedo Root & Lifecycle Surface Audit ==="
ROGUE_HOOKS="$(find . -maxdepth 3 -not -path './.git/*' \( -name '.ci-*.sh' -o -name 'package.json' \) || true)"
if [[ -n "${ROGUE_HOOKS}" ]]; then
  echo "[CRIMSON SPEEDO ALERT] Unauthorized hook/package.json detected:"
  echo "${ROGUE_HOOKS}"
  exit 1
fi
echo "[OK] Zero .ci-*.sh or package.json lifecycle hooks."

echo "=== [2/3] Dependabot Cooldown (>= 14 days), SHA Pinning & Read-Only CI Audit ==="
if [[ ! -f ".github/dependabot.yml" ]]; then
  echo "[FAIL] Missing .github/dependabot.yml"
  exit 1
fi
python3 - ".github/dependabot.yml" << 'PYDEP'
import re, sys
txt = open(sys.argv[1], encoding="utf-8").read()
ecosystems = re.findall(r'package-ecosystem:\s*["\']?([A-Za-z0-9_-]+)', txt)
cooldowns = [int(x) for x in re.findall(r'default-days:\s*(\d+)', txt)]
if not ecosystems or len(cooldowns) < len(ecosystems) or any(d < 14 for d in cooldowns):
    print(f"[FAIL] dependabot cooldown must cover all {len(ecosystems)} ecosystems with >= 14 days (found: {cooldowns})", file=sys.stderr)
    sys.exit(1)
print(f"[OK] Dependabot cooldown verified: {cooldowns} days across {ecosystems}.")
PYDEP

for wf in .github/workflows/*.yml .github/workflows/*.yaml; do
  [[ -e "${wf}" ]] || continue
  if ! grep -Eq '^permissions:[[:space:]]*$' "${wf}" || ! grep -Eq '^[[:space:]]+contents:[[:space:]]*read[[:space:]]*$' "${wf}"; then
    echo "[FAIL] ${wf} lacks strict permissions: contents: read" >&2
    exit 1
  fi
  if ! grep -Eq 'persist-credentials:[[:space:]]*false' "${wf}"; then
    echo "[FAIL] ${wf} lacks persist-credentials: false" >&2
    exit 1
  fi
  UNPINNED="$(grep -E '^[[:space:]-]*uses:[[:space:]]*' "${wf}" | grep -Ev '@[0-9a-f]{40}([[:space:]]|$|#)' || true)"
  if [[ -n "${UNPINNED}" ]]; then
    echo "[FAIL] Unpinned action reference in ${wf}: ${UNPINNED}" >&2
    exit 1
  fi
  echo "[OK] ${wf}: contents:read + persist-credentials:false + 40-char SHA pins verified."
done

echo "=== [3/3] Git Branch & Commit History IOC Audit ==="
if [[ -d ".git" ]]; then
  SUS_BRANCHES="$(git branch -a | grep -E 'wfx-docs|ci-check-[0-9]{4}|chore/[a-z0-9]{8}' || true)"
  if [[ -n "${SUS_BRANCHES}" ]]; then
    echo "[CRIMSON SPEEDO ALERT] Suspicious branches matched:"
    echo "${SUS_BRANCHES}"
    exit 1
  fi
  if git rev-parse --verify HEAD >/dev/null 2>&1; then
    SUS_COMMITS="$(git log --all --oneline \
      --grep='docs: dependency audit notes' \
      --grep='ci: refresh build cache' \
      --grep='chore: add ci helper' \
      --grep='chore: add dependency audit workflow' \
      --grep='ci: restore' \
      --grep='ci: cleanup' || true)"
    if [[ -n "${SUS_COMMITS}" ]]; then
      echo "[CRIMSON SPEEDO ALERT] Matching wfx-loot sandwich commit signatures:"
      echo "${SUS_COMMITS}"
      exit 1
    fi
  fi
fi
echo "[OK] Zero wfx-loot IOCs detected. Waystation hygiene clean (S_runner = 0, exit_invariant = 255)."
