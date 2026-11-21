"""Serialized publication of a same-source group after a fresh historical compilation.

Every claim must be CODE_EXACT. Existing single-function owners inside the
group's placement must be among the claims: they are migrated (owner replaced,
old source/recipe/binding retired) in the same transaction, never double counted.
Owners of any other kind inside the placement refuse the publication.
"""
import argparse
import json
import re

from common import CANONICAL_NAME, ROOT, json_bytes, project_path, read_json, require, sha
from module_gate import compile_group
from oracle import Oracle
from publication import publish, require_idle
from validate import GROUP_KIND, check_manifest

def parse_claim(text):
    target, sep, public = text.partition("=")
    require(sep and target and re.fullmatch(r"@?[A-Za-z_][A-Za-z0-9_]*", public), "Claim must be TARGET_ID=PUBLIC")
    return {"target_id": target, "public": public}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source")
    parser.add_argument("--name", required=True, help="Canonical group name (src/NAME.c or asm/NAME.asm, recipes/NAME.json)")
    parser.add_argument("--profile", required=True)
    parser.add_argument("--flags", nargs="+")
    parser.add_argument("--language", choices=("c", "asm"), default="c",
                        help="asm: one MASM source (profile masm510), canonical under asm/NAME.asm")
    parser.add_argument("--claim", action="append", required=True, type=parse_claim,
                        help="TARGET_ID=PUBLIC, repeated in any order")
    parser.add_argument("--binding", help="Reviewed group binding under evidence/bindings")
    parser.add_argument("--verify-only", action="store_true")
    parser.add_argument("--stage", help="verify-only: staging directory (a worker's own build/workers/NAME/...)")
    parser.add_argument("--extra-asm-review", help="verify-only: worker asm-review.json (origin evidence before merge)")
    parser.add_argument("--extra-targets", help="verify-only: worker-proposed reviewed targets JSON (list or "
                        "{'functions': [...]}) for research before canonical review")
    args = parser.parse_args()
    require(re.fullmatch(CANONICAL_NAME, args.name), "Invalid canonical group name")
    if args.extra_targets:
        import match
        require(args.verify_only, "Proposed targets are for --verify-only research runs only")
        extra = read_json(project_path(args.extra_targets))
        match.EXTRA_TARGETS[:] = extra["functions"] if isinstance(extra, dict) else extra
    if args.extra_asm_review:
        import origin
        require(args.verify_only, "Worker ASM review evidence is for --verify-only runs only")
        origin.EXTRA_REVIEW.update(read_json(project_path(args.extra_asm_review)).get("targets", {}))
    require(not args.stage or args.verify_only, "--stage is for --verify-only runs only")
    require_idle(ROOT)
    oracle = Oracle.load()
    manifest_before = (ROOT / "layout/manifest.json").read_bytes()
    manifest = json.loads(manifest_before)
    check_manifest(oracle, manifest)
    group_id = "group:" + args.name
    binding_path = project_path(args.binding) if args.binding else None
    binding_bytes = None
    if binding_path:
        # The binding may come from anywhere; publication writes these exact bytes to
        # evidence/bindings/NAME.json. One read: the proven, pinned and committed binding are the same bytes.
        binding_bytes = binding_path.read_bytes()
        require(b"\r" not in binding_bytes, "Canonical binding proof must use LF newlines")
    source_bytes = project_path(args.source).read_bytes().replace(b"\r\n", b"\n").replace(b"\r", b"\n")
    stage = project_path(args.stage) if args.stage else ROOT / "build/workers/promotion" / args.name
    stage.mkdir(parents=True, exist_ok=True)
    (stage / ("CAND.ASM" if args.language == "asm" else "CAND.C")).write_bytes(source_bytes)
    result, report = compile_group(source_bytes.decode("ascii"), args.profile, args.flags, args.claim,
                                   stage / "compile",
                                   binding=json.loads(binding_bytes) if binding_bytes else None,
                                   group_id=group_id, language=args.language)
    for claim in report["claims"]:
        print(f"  {claim['target_id']} {claim['public']}: {claim['state']}"
              + (f" ({claim['reason']})" if claim.get("reason") else "")
              + (f" first difference {claim['first_difference']}" if claim.get("first_difference") is not None else ""))
    for draft in report["drafts"]:
        print(f"  draft {draft['public']} at +{draft['object_offset']} (owns nothing)")
    exact_state = "ASM_EXACT" if args.language == "asm" else "CODE_EXACT"
    if args.language == "asm":
        from origin import require_genuine_asm
        require_genuine_asm([c["target_id"] for c in args.claim])
    require(report["exact"], f"Refusing promotion: not every claim is {exact_state}")
    space = oracle.spaces[report["space"]]
    # Only claimed extents take ownership; an owner overlapping just an unclaimed draft
    # keeps its bytes (drafts own nothing), while one overlapping a claim must migrate.
    hull = set()
    for c in report["claims"]:
        start = report["origin"] + c["object_offset"]
        hull.update(range(start, start + c["expected_size"]))
    claimed = {c["target_id"] for c in args.claim}
    migrated, kept = [], []
    for owner in manifest["owners"]:
        position = space.position(owner["segment"], owner["offset"]) if owner["space"] == space.name else None
        inside = position is not None and hull.intersection(range(position, position + owner["size"]))
        if not inside and owner["target_id"] not in claimed:
            kept.append(owner)
            continue
        recipe = read_json(project_path(owner["recipe"]))
        if recipe.get("kind") == GROUP_KIND:
            # A smaller group grows into this one only whole: every old claim is reclaimed.
            require({c["target_id"] for c in recipe["claims"]} <= claimed,
                    f"Group {recipe['name']} overlaps this group but not all its claims are reclaimed")
        else:
            require(owner["kind"] in ("MATCHING_C", "MATCHING_ASM") and owner["target_id"] in claimed,
                    f"Owner {owner['target_id']} overlaps a claim but is not itself claimed")
        migrated.append((owner, recipe))
    if args.verify_only:
        print(f"Strict group {exact_state}: {len(report['claims'])} claims at {space.name}+{report['origin']:#x}, "
              f"{report['emitted_size']} bytes; would migrate {[o['target_id'] for o, _ in migrated]}")
        return
    source_path = (ROOT / "asm" / (args.name + ".asm") if args.language == "asm"
                   else ROOT / "src" / (args.name + ".c"))
    recipe_path = ROOT / "recipes" / (args.name + ".json")
    canonical_binding = ROOT / "evidence/bindings" / (args.name + ".json")
    claims = []
    for c in report["claims"]:
        row = {"target_id": c["target_id"], "public": c["public"], "fixup_count": c["fixups"]}
        if c.get("covered_relocation_indices"):
            row["relocation_indices"] = c["covered_relocation_indices"]
            row["relocation_order"] = c.get("relocation_order", "EXACT")
            row["relocation_order_debt"] = c.get("relocation_order_debt", [])
        claims.append(row)
    recipe = {"schema": 1, "kind": GROUP_KIND, "name": args.name, "group_id": group_id, "language": args.language,
              "source": source_path.relative_to(ROOT).as_posix(), "source_sha256": sha(source_bytes),
              "profile": args.profile, "flags": list(result.flags), "object_sha256": report["object_sha256"],
              "space": report["space"], "origin": report["origin"], "emitted_size": report["emitted_size"],
              "frame": report["frame"],
              "claims": claims, "drafts": report["drafts"], "proof_scope": report["proof_scope"]}
    if binding_path:
        recipe.update(binding=canonical_binding.relative_to(ROOT).as_posix(), binding_sha256=sha(binding_bytes))
    owners = []
    for claim in report["claims"]:
        target = next(t for t in read_json(ROOT / "evidence/targets.json")["functions"]
                      if t["id"] == claim["target_id"])
        owner = {key: target[key] for key in ("space", "segment", "offset", "size")}
        owner.update(target_id=claim["target_id"], kind="MATCHING_ASM" if args.language == "asm" else "MATCHING_C",
                     state=exact_state,
                     recipe=recipe_path.relative_to(ROOT).as_posix(), group=args.name,
                     relocation_count=claim["relocations"], fixup_count=claim["fixups"])
        if claim.get("covered_relocation_indices"):
            owner["relocation_indices"] = claim["covered_relocation_indices"]
        owners.append(owner)
    manifest["owners"] = kept + owners
    # Retire only files that no recipe in the final manifest still references.
    still_used = set()
    for owner in kept:
        if owner["kind"] == "PINNED_RUNTIME":
            continue
        kept_recipe = read_json(project_path(owner["recipe"]))
        still_used |= {project_path(owner["recipe"]), project_path(kept_recipe["source"])}
        if "binding" in kept_recipe:
            still_used.add(project_path(kept_recipe["binding"]))
    retire = []
    for old_owner, old in migrated:
        paths = [project_path(old["source"]), project_path(old_owner["recipe"])]
        if "binding" in old:
            paths.append(project_path(old["binding"]))
        retire += [path for path in paths if path not in still_used and path not in retire]

    files = {source_path: source_bytes, recipe_path: json_bytes(recipe)}
    if binding_path:
        files[canonical_binding] = binding_bytes
    # Existing files may only be replaced when they belong to a group grown under its own name.
    migrated_files = set(retire) | {p for o, r in migrated for p in
                                    [project_path(r["source"]), project_path(o["recipe"])] +
                                    ([project_path(r["binding"])] if "binding" in r else [])}
    replace = [p for p in files if p.exists()]
    require(all(p in migrated_files for p in replace), "Canonical publication would overwrite a file")
    retire = [p for p in retire if p not in files]
    publish(ROOT, files, manifest_before, manifest, lambda: check_manifest(oracle, manifest),
            retire=retire, replace=replace)
    print(f"Published group {args.name}: {sum(o['size'] for o in owners)} {exact_state} bytes in {len(owners)} claims; "
          f"migrated {[o['target_id'] for o, _ in migrated]}")


if __name__ == "__main__":
    main()
