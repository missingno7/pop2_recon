#!/usr/bin/env python3
"""Check or publish one worker submission (build/workers/NAME/submission.json).

The submission is the only hand-off format between a matching worker and the
integrating writer. Paths inside it are relative to the submission's directory.

{
  "schema": 1,
  "groups": [{"name": "act/act", "language": "c", "source": "act.c", "binding": "binding.json",
              "profile": "msc600", "flags": ["/c", "/AM", "/Oe", "/Gs", "/Zl", "/Gc"],
              "claims": ["overlay-3:25da:000e=RESET_ACTOR_ALERT", ...]}],
  "targets": "proposed-targets.json",          # optional reviewed rows ({"functions": [...]} or list)
  "target_replacements": ["root:1101:6650"],   # optional: existing rows this submission replaces
  "asm_review": "asm-review.json",             # optional: origin evidence for new ASM claims
  "blocked": [{"target_id": "...", "kind": "callee|data|codegen|reloc_order|cs_frame|boundary|other",
               "needs": "target id or address (optional)", "detail": "..."}]
}

--verify-only runs promote_group.py --verify-only per group with the submission's targets and
ASM review, staged inside the worker directory (safe in a worker sandbox). Publication (the
integrating writer only) merges target rows (new rows appended; existing rows replaced only
when listed, after which dependent bindings are repinned and their groups republished),
merges the ASM review, then publishes each group. It never commits.
"""
import argparse
import json
import os
import subprocess
import sys
from glob import glob
from pathlib import Path

from common import ROOT, read_json, require

IDENTITY = ("sha256", "size")  # a row's extent; descriptive evidence may differ between reviewers
KINDS = ("callee", "data", "codegen", "reloc_order", "cs_frame", "boundary", "other")


def load(path):
    path = Path(path).resolve()
    sub = read_json(path)
    require(sub.get("schema") == 1 and isinstance(sub.get("groups"), list), "Unsupported submission schema")
    for block in sub.get("blocked", []):
        require(block.get("kind") in KINDS and block.get("target_id"), f"Bad blocked entry {block}")
    return path.parent, sub


def _rows(path):
    data = read_json(path)
    return data["functions"] if isinstance(data, dict) else data


def _write_json(path, data):
    Path(path).write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8", newline="\n")


def _promote(base, group, extra, verify_only, stage=None):
    cmd = [sys.executable, str(ROOT / "tools/promote_group.py"), str(base / group["source"]),
           "--name", group["name"], "--profile", group["profile"], "--language", group.get("language", "c")]
    if group.get("flags"):
        cmd += ["--flags"] + group["flags"]
    for claim in group["claims"]:
        cmd += ["--claim", claim]
    binding = group.get("binding")
    if binding:
        raw = (base / binding).read_bytes()
        if b"\r" in raw:  # canonical bindings are LF-only JSON
            normalized = base / (Path(binding).stem + ".lf.json")
            _write_json(normalized, json.loads(raw))
            binding = normalized.name
        cmd += ["--binding", str(base / binding)]
    if verify_only:
        cmd += ["--verify-only", "--stage", str(stage or base / "verify" / group["name"].replace("/", "_"))]
        cmd += [arg for arg in extra]
    env = dict(os.environ, MSYS_NO_PATHCONV="1")
    out = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True, env=env)
    lines = (out.stdout + out.stderr).strip().splitlines()
    return out.returncode, lines


def verify(base, sub):
    extra = []
    if sub.get("targets"):
        extra += ["--extra-targets", str(base / sub["targets"])]
    if sub.get("asm_review"):
        extra += ["--extra-asm-review", str(base / sub["asm_review"])]
    ok = True
    for group in sub["groups"]:
        rc, lines = _promote(base, group, extra, True)
        ok &= rc == 0
        print(f"[{'OK' if rc == 0 else 'REFUSED'}] {group['name']}")
        for line in lines:  # per-claim CODE_EXACT rows and owner-less drafts are noise here
            if line.endswith(": CODE_EXACT") or line.endswith("(owns nothing)"):
                continue
            print("   " + (line if len(line) <= 240 else line[:240] + " ..."))
    return ok


def _repin(changed):
    """Refresh entry_sha256 pins of canonical bindings naming changed target rows; return names."""
    touched = []
    for path in glob(str(ROOT / "evidence/bindings/**/*.json"), recursive=True):
        binding = read_json(path)
        hit = False
        for symbol in binding.get("symbols", {}).values():
            tid = symbol.get("entry_target_id")
            if tid in changed and symbol.get("entry_sha256") != changed[tid]:
                symbol["entry_sha256"] = changed[tid]
                hit = True
        if hit:
            name = Path(path).relative_to(ROOT / "evidence/bindings").with_suffix("").as_posix()
            staged = ROOT / "build/workers/promotion/repin" / (name.replace("/", "_") + ".json")
            staged.parent.mkdir(parents=True, exist_ok=True)
            _write_json(staged, binding)
            touched.append((name, staged))
    return touched


def _republish(name, binding):
    recipe = read_json(ROOT / "recipes" / (name + ".json"))
    group = {"name": name, "source": str(ROOT / recipe["source"]), "profile": recipe["profile"],
             "language": recipe.get("language", "c"), "flags": recipe.get("flags", []),
             "claims": [f"{c['target_id']}={c['public']}" for c in recipe["claims"]],
             "binding": str(binding)}
    return _promote(Path("/"), group, [], False)


def publish(base, sub):
    targets_path = ROOT / "evidence/targets.json"
    inventory = read_json(targets_path)
    existing = {f["id"]: i for i, f in enumerate(inventory["functions"])}
    replace = set(sub.get("target_replacements", []))
    changed, added = {}, []
    if sub.get("targets"):
        for row in _rows(base / sub["targets"]):
            if row["id"] in existing:
                old = inventory["functions"][existing[row["id"]]]
                if all(old.get(k) == row.get(k) for k in IDENTITY):
                    continue  # same reviewed extent: the canonical row and its evidence stay
                if old != row:
                    require(row["id"] in replace, f"Submission changes existing row {row['id']} "
                                                  "without listing it in target_replacements")
                    inventory["functions"][existing[row["id"]]] = row
                    changed[row["id"]] = row["sha256"]
            else:
                inventory["functions"].append(row)
                added.append(row["id"])
        _write_json(targets_path, inventory)
    print(f"target rows: {len(added)} added, {len(changed)} replaced")
    if sub.get("asm_review"):
        review_path = ROOT / "evidence/asm-origin-review.json"
        review = read_json(review_path)
        new = read_json(base / sub["asm_review"]).get("targets", {})
        review.setdefault("targets", {}).update(new)
        _write_json(review_path, review)
        print(f"asm review: {len(new)} entries merged")
    for name, staged in _repin(changed):
        rc, lines = _republish(name, staged)
        print(f"[{'REPINNED' if rc == 0 else 'REPIN FAILED'}] {name}: {lines[-1] if lines else ''}")
        require(rc == 0, f"Repin of {name} failed")
    for group in sub["groups"]:
        rc, lines = _promote(base, group, [], False)
        print(f"[{'PUBLISHED' if rc == 0 else 'REFUSED'}] {group['name']}: {lines[-1] if lines else ''}")
        require(rc == 0, f"Publication of {group['name']} failed")


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("submission")
    parser.add_argument("--verify-only", action="store_true")
    args = parser.parse_args()
    base, sub = load(args.submission)
    if args.verify_only:
        sys.exit(0 if verify(base, sub) else 1)
    publish(base, sub)


if __name__ == "__main__":
    main()
