#!/usr/bin/env python3
"""scripts/audit_skill_portability_v1_0_0.py -- Agora Two-Plane Hygiene & Skill Linter."""

import argparse
import json
import pathlib
import re
import sys

PIPE = chr(124)
EXEC_EXTENSIONS = {
    ".py", ".sh", ".bash", ".pl", ".rb", ".js",
    ".ts", ".rs", ".c", ".jl", ".so", ".bin",
}
CLASS_II_PATTERNS = {
    "credential_dotfile": re.compile(
        PIPE.join([
            r"~/\.config/[^\s)\x60]*token",
            r"\$ZENODO_TOKEN",
            r"chmod\s+0?600",
        ]),
        re.IGNORECASE,
    ),
    "live_host_socket": re.compile(
        PIPE.join([
            r"/var/run/dandelion_mesh\.sock",
            r"/tmp/luanti_trit\.fifo",
            r"connect_dandyland\.sh",
        ])
    ),
}
SEMVER_RE = re.compile(r"\bv\d+\.\d+\.\d+\b")
ALLOWED_TOOLS_RE = re.compile(r"^allowed_tools:\s*(.+)$", re.MULTILINE)

CANONICAL_ROOT_FILES = {
    "README.md",
    "Makefile",
    "Cargo.toml",
    "Cargo.lock",
    "Project.toml",
    "LICENSE",
    "CITATION.cff",
    ".zenodo.json",
    ".gitignore",
    ".editorconfig",
}


def scan_skill_dir(skill_dir: pathlib.Path) -> dict:
    findings = {
        "skill": skill_dir.name,
        "class_i": [],
        "class_ii": [],
        "class_iii": [],
    }
    for path in sorted(skill_dir.rglob("*")):
        if (
            path.is_file()
            and path.name != "SKILL.md"
            and path.suffix.lower() in EXEC_EXTENSIONS
        ):
            findings["class_i"].append(str(path.relative_to(skill_dir)))

    skill_md = skill_dir / "SKILL.md"
    if not skill_md.exists():
        findings["class_i"].append("MISSING_SKILL_MD")
        return findings

    text = skill_md.read_text(encoding="utf-8")
    for label, regex in CLASS_II_PATTERNS.items():
        hits = sorted(set(m.group(0) for m in regex.finditer(text)))
        if hits:
            findings["class_ii"].append(f"{label}: {', '.join(hits)}")

    m_tools = ALLOWED_TOOLS_RE.search(text)
    if m_tools and "docs_agent" not in m_tools.group(1):
        findings["class_iii"].append(
            f"narrow_allowed_tools_missing_docs_agent: {m_tools.group(1).strip()}"
        )

    if not SEMVER_RE.search(text):
        findings["class_iii"].append("missing_3_digit_semver_vX_Y_Z")

    return findings


def audit_dun_ga_din_root(root: pathlib.Path) -> list[str]:
    violations: list[str] = []
    required = [
        "README.md",
        "Makefile",
        "Cargo.toml",
        "Project.toml",
        "LICENSE",
        "CITATION.cff",
        ".zenodo.json",
    ]
    for req in required:
        if not (root / req).is_file():
            violations.append(f"missing_canonical_root_file: {req}")
    if not (root / "manifests").is_dir():
        violations.append("missing_canonical_dir: manifests/")

    for entry in sorted(root.iterdir()):
        if entry.is_file() and entry.name not in CANONICAL_ROOT_FILES:
            violations.append(f"non_canonical_root_file: {entry.name}")
    return violations


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Audit Agora Two-Plane skill portability and Dun-Ga-Din root hygiene."
    )
    parser.add_argument(
        "root",
        nargs="?",
        default=".",
        help="Repository or skill directory root",
    )
    parser.add_argument(
        "--seed",
        type=int,
        default=42,
        help="Deterministic audit seed (default: 42)",
    )
    args = parser.parse_args()

    root = pathlib.Path(args.root).resolve()
    skill_dirs = sorted(p.parent for p in root.rglob("SKILL.md"))
    skill_results = [scan_skill_dir(d) for d in skill_dirs]
    root_violations = audit_dun_ga_din_root(root)

    routing_map = root / "manifests" / "agora_hot_routing_map_v1.0.0.json"
    routing_bytes = routing_map.stat().st_size if routing_map.is_file() else -1
    if routing_bytes < 0 or routing_bytes > 4096:
        root_violations.append(
            f"hot_routing_map_size_out_of_bounds: {routing_bytes} B (must be 1..4096 B)"
        )

    report = {
        "schema": "AGORA_PORTABILITY_AND_ROOT_AUDIT",
        "version": "v1.0.0",
        "seed": args.seed,
        "skills_audited": len(skill_results),
        "skill_results": skill_results,
        "dun_ga_din_root_violations": root_violations,
        "hot_routing_map_bytes": routing_bytes,
        "exit_invariant": 255,
    }
    print(json.dumps(report, indent=2))
    has_skill_blockers = any(r["class_i"] or r["class_ii"] for r in skill_results)
    return 1 if (has_skill_blockers or root_violations) else 0


if __name__ == "__main__":
    sys.exit(main())
