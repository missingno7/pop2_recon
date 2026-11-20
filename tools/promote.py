"""Serialize strict component publication after a fresh historical compilation."""
import argparse
import json
import re

from common import CANONICAL_NAME, ROOT, read_json, require, sha, project_path, json_bytes
from publication import publish, require_idle
from match import compile_and_check, get_target
from oracle import Oracle
from validate import check_manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("target")
    parser.add_argument("source")
    parser.add_argument("--profile", required=True)
    parser.add_argument("--symbol", required=True)
    parser.add_argument("--name", required=True)
    parser.add_argument("--flags", nargs="+")
    parser.add_argument("--language", choices=("c", "asm"), default="c")
    parser.add_argument("--binding", help="Reviewed original-derived symbolic binding JSON")
    parser.add_argument("--verify-only", action="store_true")
    args = parser.parse_args()
    require(re.fullmatch(CANONICAL_NAME, args.name), "Invalid canonical source name")
    require_idle(ROOT)
    oracle = Oracle.load()
    manifest_before = (ROOT / "layout/manifest.json").read_bytes()
    manifest = json.loads(manifest_before)
    check_manifest(oracle, manifest)
    require(all(r["target_id"] != args.target for r in manifest["owners"]), "Target already has canonical owner")
    binding_path = project_path(args.binding) if args.binding else None
    if binding_path:
        require(binding_path.is_relative_to(ROOT / "evidence/bindings"),
                "Canonical binding proof must live under evidence/bindings")
        require(b"\r" not in binding_path.read_bytes(),
                "Canonical binding proof must use LF newlines for stable Git identity")
    original_source = project_path(args.source).read_bytes()
    # Canonical source hashes must survive the repository's LF checkout policy.
    original_source = original_source.replace(b"\r\n", b"\n").replace(b"\r", b"\n")
    stage = ROOT / "build/workers/promotion" / args.name
    stage.mkdir(parents=True, exist_ok=True)
    frozen_source = stage / ("CAND.C" if args.language == "c" else "CAND.ASM")
    frozen_source.write_bytes(original_source)
    result, report = compile_and_check(frozen_source, args.profile, args.symbol, args.target,
                                       stage / "compile", args.flags, language=args.language,
                                       binding=read_json(binding_path) if binding_path else None)
    require(report["exact"], "Refusing promotion: complete component does not match")
    if args.verify_only:
        print(f"Strict {report['state']}: {args.target}, {report['emitted_size']} bytes")
        return
    source_path = (ROOT / "src" / (args.name + ".c") if args.language == "c"
                   else ROOT / "asm" / (args.name + ".asm"))
    recipe_path = ROOT / "recipes" / (args.name + ".json")
    require(not source_path.exists() and not recipe_path.exists(), "Canonical publication would overwrite a file")
    target, _, _ = get_target(args.target, oracle)
    recipe = {"schema": 1, "name": args.name, "target_id": args.target, "language": args.language,
              "source": source_path.relative_to(ROOT).as_posix(), "source_sha256": sha(original_source),
              "profile": args.profile, "flags": list(result.flags), "public": args.symbol,
              "object_sha256": report["object_sha256"], "proof_scope": report["proof_scope"]}
    if binding_path:
        recipe.update(binding=binding_path.relative_to(ROOT).as_posix(),
                      binding_sha256=sha(binding_path.read_bytes()), fixup_count=report["fixups"])
    if "relocation_order" in report:
        recipe["relocation_order"] = report["relocation_order"]
        recipe["relocation_order_debt"] = report["relocation_order_debt"]
    if "covered_relocation_indices" in report:
        recipe["relocation_indices"] = report["covered_relocation_indices"]
    owner = {key: target[key] for key in ("space", "segment", "offset", "size")}
    owner.update(target_id=args.target, kind="MATCHING_C" if args.language == "c" else "MATCHING_ASM",
                 state=report["state"],
                 recipe=recipe_path.relative_to(ROOT).as_posix(), relocation_count=report["relocations"],
                 fixup_count=report["fixups"])
    if "covered_relocation_indices" in report:
        owner["relocation_indices"] = report["covered_relocation_indices"]
    manifest["owners"].append(owner)
    publish(ROOT, {source_path: original_source, recipe_path: json_bytes(recipe)},
            manifest_before, manifest, lambda: check_manifest(oracle, manifest))
    print(f"Published {source_path.relative_to(ROOT)}: {target['size']} {report['state']} bytes")


if __name__ == "__main__":
    main()
