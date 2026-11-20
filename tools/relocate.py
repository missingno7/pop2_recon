"""Re-publish an accepted component under its original module path and names.

Renames are source-level identifier substitutions (functions and globals); the
component is recompiled freshly and must stay exact through the same gate that
accepted it (single-function gate with its binding mode, or the group gate).
Binding files keep every independent witness unchanged: only symbol keys follow
the renamed externals, and the object-derived `declarations`/`fixups` are taken
from the fresh object. Old source/recipe/binding are retired after the manifest
commit through the recoverable publication journal.

  python tools/relocate.py OLD_RECIPE_NAME --name module/new_name \
      --rename OLD_IDENT=NewIdent ... [--verify-only]
"""
import argparse
import json
import re

from binding import declarations, fixes
from common import CANONICAL_NAME, ROOT, json_bytes, project_path, read_json, require, sha
from omf import OmfReader
from oracle import Oracle
from publication import publish, require_idle
from validate import GROUP_KIND, check_manifest


def parse_rename(text):
    old, sep, new = text.partition("=")
    ident = r"[A-Za-z_][A-Za-z0-9_]*"
    require(sep and re.fullmatch(ident, old) and re.fullmatch(ident, new), "Rename must be OLD=NEW identifiers")
    return old, new


def rename_source(text, renames):
    for old, new in renames:
        require(re.search(rf"\b{old}\b", text), f"Identifier {old} does not occur in the source")
        text = re.sub(rf"\b{old}\b", new, text)
    return text


def compile_object(source_text, recipe, workdir):
    from assembler import assemble_asm
    from compiler import compile_c
    if recipe.get("language", "c") == "asm":
        stage = workdir / "UNIT.ASM"
        stage.parent.mkdir(parents=True, exist_ok=True)
        stage.write_text(source_text, encoding="ascii", newline="\n")
        result = assemble_asm(stage, recipe["profile"], flags=recipe["flags"], workdir=workdir / "asm")
    else:
        result = compile_c(source_text, recipe["profile"], flags=recipe["flags"], workdir=workdir)
    require(result.ok, "Historical compilation failed: " + result.log)
    return OmfReader().read(result.obj_bytes), result


def renamed_binding(binding, old_module, new_module, group_id):
    """Witness-preserving rename: externals pair by declaration position in old and new objects."""
    require(len(old_module.externals) == len(new_module.externals), "Renaming changed the external count")
    key_map = dict(zip(old_module.externals, new_module.externals))
    out = json.loads(json.dumps(binding))
    for holder in (out, out.get("data") or {}):
        if "symbols" in holder:
            holder["symbols"] = {key_map.get(k, k): v for k, v in holder["symbols"].items()}
    out["declarations"] = declarations(new_module)
    out["fixups"] = fixes(new_module)
    if group_id:
        out["target_id"] = group_id
    return out


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("old_name", help="Existing recipe name (recipes/OLD_NAME.json)")
    parser.add_argument("--name", required=True, help="New canonical name, e.g. ctrl/get_opp_dist")
    parser.add_argument("--rename", action="append", default=[], type=parse_rename)
    parser.add_argument("--verify-only", action="store_true")
    args = parser.parse_args()
    require(re.fullmatch(CANONICAL_NAME, args.name), "Invalid canonical name")
    require_idle(ROOT)
    oracle = Oracle.load()
    manifest_before = (ROOT / "layout/manifest.json").read_bytes()
    manifest = json.loads(manifest_before)
    check_manifest(oracle, manifest)
    old_recipe_path = project_path(f"recipes/{args.old_name}.json")
    recipe = read_json(old_recipe_path)
    owners = [o for o in manifest["owners"] if o.get("recipe") == old_recipe_path.relative_to(ROOT).as_posix()]
    require(owners and all(o["kind"] in ("MATCHING_C", "MATCHING_ASM") for o in owners),
            "Relocation needs an accepted C/ASM recipe with owners")
    old_source = project_path(recipe["source"]).read_bytes()
    require(sha(old_source) == recipe["source_sha256"], "Canonical source changed without acceptance")
    new_text = rename_source(old_source.decode("ascii"), args.rename)
    group = recipe.get("kind") == GROUP_KIND
    stage = ROOT / "build/workers/relocation" / args.name
    old_module, _ = compile_object(old_source.decode("ascii"), recipe, stage / "old")
    new_module, result = compile_object(new_text, recipe, stage / "new")
    old_offsets = {p["name"]: p["offset"] for p in old_module.publics}
    new_by_offset = {p["offset"]: p["name"] for p in new_module.publics}
    public_map = {name: new_by_offset[offset] for name, offset in old_offsets.items()}

    binding = None
    if "binding" in recipe:
        raw = project_path(recipe["binding"]).read_bytes()
        require(sha(raw) == recipe["binding_sha256"], "Canonical binding changed without acceptance")
        binding = renamed_binding(json.loads(raw), old_module, new_module,
                                  "group:" + args.name if group else None)

    if group:
        from module_gate import compile_group
        claims = [{"target_id": c["target_id"], "public": public_map[c["public"]]} for c in recipe["claims"]]
        result, report = compile_group(new_text, recipe["profile"], recipe["flags"], claims, stage / "proof",
                                       binding=binding, group_id="group:" + args.name)
        require(report["exact"], "Relocated group is not exact")
        new_recipe = dict(recipe, name=args.name, group_id="group:" + args.name, object_sha256=report["object_sha256"],
                          claims=[dict(c, public=public_map[c["public"]]) for c in recipe["claims"]],
                          drafts=report["drafts"])
    else:
        from match import compile_and_check
        stage.mkdir(parents=True, exist_ok=True)
        suffix = "ASM" if recipe.get("language") == "asm" else "C"
        frozen = stage / f"CAND.{suffix}"
        frozen.write_text(new_text, encoding="ascii", newline="\n")
        result, report = compile_and_check(frozen, recipe["profile"], public_map[recipe["public"]],
                                           recipe["target_id"], stage / "proof", recipe["flags"],
                                           language=recipe.get("language", "c"), binding=binding)
        require(report["exact"], "Relocated component is not exact")
        new_recipe = dict(recipe, name=args.name, public=public_map[recipe["public"]],
                          object_sha256=report["object_sha256"])
    language_dir = "asm" if recipe.get("language") == "asm" else "src"
    source_path = ROOT / language_dir / (args.name + (".asm" if language_dir == "asm" else ".c"))
    recipe_path = ROOT / "recipes" / (args.name + ".json")
    new_recipe["source"] = source_path.relative_to(ROOT).as_posix()
    new_recipe["source_sha256"] = sha(new_text.encode("ascii"))
    files = {source_path: new_text.encode("ascii"), recipe_path: None}
    if binding is not None:
        binding_path = ROOT / "evidence/bindings" / (args.name + ".json")
        binding_bytes = json_bytes(binding)
        files[binding_path] = binding_bytes
        new_recipe.update(binding=binding_path.relative_to(ROOT).as_posix(), binding_sha256=sha(binding_bytes))
    files[recipe_path] = json_bytes(new_recipe)
    print(f"Relocated {args.old_name} -> {args.name}: exact; publics {public_map}")
    if args.verify_only:
        return
    new_recipe_rel = recipe_path.relative_to(ROOT).as_posix()
    for owner in owners:
        owner["recipe"] = new_recipe_rel
        if group:
            owner["group"] = args.name
    retire = [project_path(recipe["source"]), old_recipe_path]
    if "binding" in recipe:
        still = any(read_json(project_path(o["recipe"])).get("binding") == recipe["binding"]
                    for o in manifest["owners"] if o["recipe"] != new_recipe_rel and o["kind"] != "PINNED_RUNTIME")
        if not still:
            retire.append(project_path(recipe["binding"]))
    retire = [p for p in retire if p not in files]
    publish(ROOT, files, manifest_before, manifest, lambda: check_manifest(oracle, manifest), retire=retire)
    print(f"Published {source_path.relative_to(ROOT)}; retired {[p.relative_to(ROOT).as_posix() for p in retire]}")


if __name__ == "__main__":
    main()
