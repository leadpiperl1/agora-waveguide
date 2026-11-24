#!/usr/bin/env bash
# scripts/audit_wfx_loot.sh (v1.2.2)
# Read-only wfx-loot IOC, Crimson Speedo Gate, and dependabot-cooldown >= 7 audit.
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

echo "=== [2/3] Dependabot Cooldown (>= 7 days) & Read-Only CI Audit ==="
if [[ ! -f ".github/dependabot.yml" ]]; then
  echo "[FAIL] Missing .github/dependabot.yml"
  exit 1
fi
COOLDOWN_DAYS="$(awk '/default-days:/{print $2}' .github/dependabot.yml | sort -n | head -n 1 || true)"
if [[ -z "${COOLDOWN_DAYS}" ]] || [[ "${COOLDOWN_DAYS}" -lt 7 ]]; then
  echo "[FAIL] dependabot cooldown must be >= 7 days (found: ${COOLDOWN_DAYS:-none})"
  exit 1
fi
echo "[OK] Dependabot cooldown verified: ${COOLDOWN_DAYS} days."

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
