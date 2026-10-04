"""Serialize strict component publication after a fresh historical compilation."""
import argparse
import re

from common import ROOT, read_json, require, sha, project_path, write_json
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
    require(re.fullmatch(r"[a-z][a-z0-9_]*", args.name), "Invalid canonical source name")
    oracle = Oracle.load()
    manifest = read_json(ROOT / "layout/manifest.json")
    check_manifest(oracle, manifest)
    require(all(r["target_id"] != args.target for r in manifest["owners"]), "Target already has canonical owner")
    original_source = project_path(args.source).read_bytes()
    stage = ROOT / "build/workers/promotion" / args.name
    stage.mkdir(parents=True, exist_ok=True)
    frozen_source = stage / ("CAND.C" if args.language == "c" else "CAND.ASM")
    frozen_source.write_bytes(original_source)
    result, report = compile_and_check(frozen_source, args.profile, args.symbol, args.target,
                                       stage / "compile", args.flags, language=args.language,
                                       binding=read_json(project_path(args.binding)) if args.binding else None)
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
    if args.binding:
        binding_path = project_path(args.binding)
        require(binding_path.is_relative_to(ROOT / "evidence/bindings"),
                "Canonical binding proof must live under evidence/bindings")
        recipe.update(binding=binding_path.relative_to(ROOT).as_posix(),
                      binding_sha256=sha(binding_path.read_bytes()), fixup_count=report["fixups"])
    owner = {key: target[key] for key in ("space", "segment", "offset", "size")}
    owner.update(target_id=args.target, kind="MATCHING_C" if args.language == "c" else "MATCHING_ASM",
                 state=report["state"],
                 recipe=recipe_path.relative_to(ROOT).as_posix(), relocation_count=report["relocations"],
                 fixup_count=report["fixups"])
    manifest["owners"].append(owner)
    source_path.parent.mkdir(parents=True, exist_ok=True)
    source_path.write_bytes(original_source)
    write_json(recipe_path, recipe)
    try:
        check_manifest(oracle, manifest)
    except Exception:
        source_path.unlink()
        recipe_path.unlink()
        raise
    write_json(ROOT / "layout/manifest.json", manifest)
    print(f"Published {source_path.relative_to(ROOT)}: {target['size']} {report['state']} bytes")


if __name__ == "__main__":
    main()
