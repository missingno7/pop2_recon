"""Build reproducible MSC code-generation and relocation probes into ignored scratch."""
from __future__ import annotations
import json
import hashlib
from pathlib import Path
from compiler import ROOT, CompileError, compile_c, extract_function, load_lock, verify_profile

PROBES = {
    "increment": ("recipes/probes/increment.c", "_increment"),
    "external_call": ("recipes/probes/external_call.c", "_invoke_external"),
    "signed_branch": ("recipes/probes/signed_branch.c", "_signed_branch"),
    "unsigned_branch": ("recipes/probes/unsigned_branch.c", "_unsigned_branch"),
    "switch_select": ("recipes/probes/switch_select.c", "_switch_select"),
    "far_pointer": ("recipes/probes/far_pointer.c", "_read_far"),
    "long_multiply": ("recipes/probes/long_multiply.c", "_multiply_long"),
    "long_divide": ("recipes/probes/long_divide.c", "_divide_long"),
    "register_locals": ("recipes/probes/register_locals.c", "_register_locals"),
}
FLAGS = ["/c", "/O", "/AS", "/Gs", "/Zl"]

def main() -> None:
    result = {
        "schema": 1,
        "runner": load_lock()["runner"],
        "toolchain_lock_sha256": hashlib.sha256(
            (ROOT / "layout" / "toolchain.lock.json").read_bytes()).hexdigest(),
        "probe_flags": FLAGS,
        "profiles": {},
    }
    lock = load_lock()
    for profile in ("msc510", "msc600", "msc600a_bound"):
        verify_profile(profile)
        spec = lock["profiles"][profile]
        per_profile = {"label": spec["label"], "directory": spec["directory"],
                       "executable": spec["executable"], "pinned_files": spec["files"],
                       "probe_flags": FLAGS, "probes": {}}
        probes = PROBES if profile in ("msc510", "msc600") else {
            key: PROBES[key] for key in ("increment", "external_call")
        }
        for name, (source, public) in probes.items():
            work = ROOT / "build" / "workers" / "toolchain" / "probes" / profile / name
            try:
                built = compile_c(ROOT / source, profile, flags=FLAGS, workdir=work)
            except (CompileError, OSError, TimeoutError) as exc:
                per_profile["probes"][name] = {"status": "BLOCKED", "error": str(exc)}
                continue
            item = {
                "status": "COMPILED" if built.ok else ("TIMED_OUT" if built.timed_out else "COMPILER_FAILED"),
                "source": source,
                "public_name": public,
                "source_sha256": built.source_sha256,
                "staged_source_sha256": built.staged_source_sha256,
                "argv": built.argv,
                "object_sha256": built.obj_sha256,
                "object_path": str(built.obj) if built.obj else None,
                "compiler_log": built.log,
            }
            if built.parsed:
                module = built.parsed
                item.update({
                    "module_name": module.name,
                    "segments": module.segment_defs,
                    "initialized_ranges": module.initialized_ranges,
                    "segment_bytes_hex": {key: value.hex() for key, value in module.segments.items()},
                    "publics": module.publics,
                    "externals": module.externals,
                    "fixups": module.fixups,
                    "omf_record_types": [record["type"] for record in module.records],
                })
                try:
                    complete = extract_function(module, public)
                    item["complete_function_extent_hex"] = complete.hex()
                    item["complete_function_extent_sha256"] = __import__("hashlib").sha256(complete).hexdigest()
                except CompileError as exc:
                    item["extent_status"] = "REFUSED"
                    item["extent_reason"] = str(exc)
            per_profile["probes"][name] = item
        result["profiles"][profile] = per_profile
    contrasts = {}
    for profile in ("msc510", "msc600"):
        contrasted = {}
        for name in ("far_pointer", "switch_select"):
            source, public = PROBES[name]
            work = ROOT / "build" / "workers" / "toolchain" / "contrasts" / profile / name
            flags = ["/c", "/O", "/AM", "/Gs", "/Zl"]
            try:
                built = compile_c(ROOT / source, profile, flags=flags, workdir=work)
            except (CompileError, OSError, TimeoutError) as exc:
                contrasted[name] = {"status": "BLOCKED", "error": str(exc)}
                continue
            item = {"status": "COMPILED" if built.ok else ("TIMED_OUT" if built.timed_out else "COMPILER_FAILED"),
                    "source": source, "public_name": public, "flags": flags,
                    "source_sha256": built.source_sha256,
                    "staged_source_sha256": built.staged_source_sha256, "argv": built.argv,
                    "object_sha256": built.obj_sha256, "compiler_log": built.log}
            if built.parsed:
                module = built.parsed
                item.update({"segments": module.segment_defs,
                             "initialized_ranges": module.initialized_ranges,
                             "segment_bytes_hex": {k: v.hex() for k, v in module.segments.items()},
                             "publics": module.publics, "externals": module.externals,
                             "fixups": module.fixups,
                             "omf_record_types": [r["type"] for r in module.records]})
                try:
                    payload = extract_function(module, public)
                    item["complete_function_extent_hex"] = payload.hex()
                except CompileError as exc:
                    item["extent_status"] = "REFUSED"
                    item["extent_reason"] = str(exc)
            contrasted[name] = item
        contrasts[profile] = contrasted
    result["model_contrasts"] = contrasts
    flags = ["/c", "/AM", "/Oe", "/Gs", "/Zl", "/Gc"]
    work = ROOT / "build" / "workers" / "toolchain" / "variants" / "msc600-game-pascal-oe"
    try:
        built = compile_c(ROOT / PROBES["increment"][0], "msc600", flags=flags, workdir=work)
        item = {"status": "COMPILED" if built.ok else ("TIMED_OUT" if built.timed_out else "COMPILER_FAILED"),
                "scope": "accepted floor candidate family only",
                "source": PROBES["increment"][0], "flags": flags, "argv": built.argv,
                "source_sha256": built.source_sha256, "object_sha256": built.obj_sha256,
                "compiler_log": built.log}
        if built.parsed:
            item.update({"segments": built.parsed.segment_defs,
                         "initialized_ranges": built.parsed.initialized_ranges,
                         "segment_bytes_hex": {k: v.hex() for k, v in built.parsed.segments.items()},
                         "publics": built.parsed.publics, "externals": built.parsed.externals,
                         "fixups": built.parsed.fixups,
                         "omf_record_types": [r["type"] for r in built.parsed.records]})
        result["optimizer_variant_msc600_game_pascal_oe"] = item
    except (CompileError, OSError, TimeoutError) as exc:
        result["optimizer_variant_msc600_game_pascal_oe"] = {"status": "BLOCKED", "error": str(exc)}
    destination = ROOT / "evidence" / "toolchain" / "probes.json"
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(destination)

if __name__ == "__main__":
    main()
