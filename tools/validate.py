"""Fresh authoritative bootstrap validation; never alters locks or oracle assets."""
import argparse
import json
import struct
import subprocess
import sys
from datetime import datetime, timezone
from uuid import uuid4

from common import ROOT, read_json, require, sha, project_path, write_json
from match import compile_and_check, get_target
from metrics import metrics
from oracle import Oracle
from publication import require_idle


GROUP_KIND = "same_source_group"


def check_manifest(oracle, manifest):
    owned = {}
    ids = set()
    group_claims = {}
    runtime_reports = []
    for owner in manifest["owners"]:
        require(owner["target_id"] not in ids, "Duplicate target owner")
        ids.add(owner["target_id"])
        require((owner["kind"], owner["state"]) in (("MATCHING_C", "CODE_EXACT"),
                                                      ("MATCHING_ASM", "ASM_EXACT"),
                                                      ("PINNED_RUNTIME", "PINNED_RUNTIME")),
                "Unsupported bootstrap ownership kind/state")
        recipe = read_json(project_path(owner["recipe"]))
        if owner["kind"] in ("MATCHING_C", "MATCHING_ASM"):
            target, space, _ = get_target(owner["target_id"], oracle)
        else:
            target = recipe
            space = oracle.spaces[target["space"]]
            require(owner["target_id"] == f"{space.name}:{target['segment']:04x}:{target['offset']:04x}",
                    "Runtime address identity mismatch")
        require(all(owner[key] == target[key] for key in ("space", "segment", "offset", "size")),
                "Owner disagrees with reviewed oracle extent")
        start = space.position(owner["segment"], owner["offset"])
        touched = [r["index"] for r in space.relocations
                   if start-1 <= r["image_offset"] < start+owner["size"]]
        # A group recipe records relocation obligations per claim.
        pinned = (next((c.get("relocation_indices", []) for c in recipe["claims"]
                        if c["target_id"] == owner["target_id"]), None)
                  if recipe.get("kind") == GROUP_KIND else recipe.get("relocation_indices", []))
        require(owner.get("relocation_indices", []) == touched and
                owner.get("relocation_count", 0) == len(touched) and pinned == touched,
                "Owner/recipe original relocation indices differ from extent obligations")
        span = set(range(start, start+owner["size"]))
        require(not owned.setdefault(space.name, set()).intersection(span), "Overlapping canonical owners")
        owned[space.name].update(span)
        if owner["kind"] == "PINNED_RUNTIME":
            from runtime import verify_member
            runtime_report = verify_member(recipe, oracle, manifest)
            runtime_reports.append(runtime_report)
            require(runtime_report["fixups"] == owner.get("fixup_count", 0) and
                    runtime_report["relocations"] == owner.get("relocation_count", 0),
                    "Runtime owner obligation counts differ from fresh proof")
            continue
        if recipe.get("kind") == GROUP_KIND:
            require(owner["kind"] == ("MATCHING_ASM" if recipe.get("language", "c") == "asm" else "MATCHING_C") and
                    owner.get("group") == recipe["name"],
                    "Group owner does not name its group recipe")
            claim = [c for c in recipe["claims"] if c["target_id"] == owner["target_id"]]
            require(len(claim) == 1 and claim[0]["fixup_count"] == owner["fixup_count"],
                    "Group owner is not exactly one recipe claim")
            group_claims.setdefault(owner["recipe"], set()).add(owner["target_id"])
        else:
            require(recipe["target_id"] == owner["target_id"], "Recipe target mismatch")
        source = project_path(recipe["source"])
        language = recipe.get("language", "c")
        require(language == ("asm" if owner["kind"] == "MATCHING_ASM" else "c"),
                "Recipe source language disagrees with ownership")
        source_dir = ROOT / ("asm" if language == "asm" else "src")
        require(source.is_relative_to(source_dir), "Canonical source outside its language directory")
        if language == "asm":
            # Only genuine original assembly is matched as MASM (evidence/asm-origin-audit.json).
            from origin import require_genuine_asm
            require_genuine_asm([owner["target_id"]])
        require(sha(source.read_bytes()) == recipe["source_sha256"], "Canonical source changed without acceptance")
        if "binding" in recipe:
            binding_path = project_path(recipe["binding"])
            require(binding_path.is_relative_to(ROOT / "evidence/bindings") and
                    sha(binding_path.read_bytes()) == recipe["binding_sha256"],
                    "Canonical binding evidence changed without acceptance")
            if recipe.get("kind") != GROUP_KIND:
                require(owner["fixup_count"] == recipe["fixup_count"], "Owner fixup count differs from recipe")
    for path, claimed in group_claims.items():
        require(claimed == {c["target_id"] for c in read_json(project_path(path))["claims"]},
                "Group recipe claims and canonical owners differ")
    from runtime_fixups import check_layout
    check_layout(runtime_reports)


def verify_group(recipe):
    """Fresh compilation of one same-source group; every claim must stay exact."""
    from module_gate import compile_group
    # Hash exactly the bytes that are compiled and parsed; a later edit cannot pass under an old pin.
    source = project_path(recipe["source"]).read_bytes()
    require(sha(source) == recipe["source_sha256"], "Canonical source changed without acceptance")
    binding = None
    if "binding" in recipe:
        raw = project_path(recipe["binding"]).read_bytes()
        require(sha(raw) == recipe["binding_sha256"], "Canonical binding evidence changed without acceptance")
        binding = json.loads(raw)
    _, report = compile_group(source.decode("ascii"), recipe["profile"], recipe["flags"],
                              [{"target_id": c["target_id"], "public": c["public"]} for c in recipe["claims"]],
                              ROOT / "build/workers/validation" / recipe["name"],
                              binding=binding, group_id=recipe["group_id"], language=recipe.get("language", "c"))
    require(report["exact"], "Previously accepted group no longer compiles exactly")
    require(report["object_sha256"] == recipe["object_sha256"], "Accepted group OMF identity changed")
    require(all(report[key] == recipe[key] for key in ("space", "origin", "emitted_size", "frame", "drafts")),
            "Accepted group placement, extent or draft record changed")
    return report


def check_relocation_order(recipe, report):
    """Old exact-order recipes remain valid; a grouped proof must be explicit."""
    if "relocation_order" in report:
        require(recipe.get("relocation_order", "EXACT") == report["relocation_order"] and
                recipe.get("relocation_order_debt", []) == report["relocation_order_debt"],
                "Fresh relocation order qualification/debt differs from recipe")


def _validate(run_tests=True):
    oracle = Oracle.load()
    structure = read_json(ROOT / "layout/structure.lock.json")
    spaces = [{"name": s.name, "link_segment": s.link_segment, "file_offset": s.file_offset,
               "size": s.size, "sha256": sha(s.data), "ordered_relocation_count": len(s.relocations),
               "ordered_relocation_sha256": sha(b"".join(struct.pack("<HH", r["offset"], r["segment"])
                                                        for r in s.relocations))}
              for s in oracle.spaces.values()]
    require(structure["target_sha256"] == sha(oracle.data) and
            structure["spaces"] == spaces and structure["gaps"] == oracle.gaps,
            "Parsed structure disagrees with original-derived frozen structure")
    manifest = read_json(ROOT / "layout/manifest.json")
    check_manifest(oracle, manifest)
    from compiler import load_lock, verify_profile
    profiles = read_json(ROOT / "recipes/compiler-profiles.json")
    # Pin checks cover installed tested profiles even before one owns canonical code.
    require(set(profiles["profiles"]).issubset(load_lock()["profiles"]), "Unknown recipe compiler profile")
    for profile in load_lock()["profiles"]:
        verify_profile(profile)
    from assembler import verify_assembler
    for profile in load_lock().get("assemblers", {}):
        verify_assembler(profile)
    accepted = []
    groups = {}
    for owner in manifest["owners"]:
        recipe = read_json(project_path(owner["recipe"]))
        if recipe.get("kind") == GROUP_KIND:
            if owner["recipe"] not in groups:
                groups[owner["recipe"]] = verify_group(recipe)
                accepted.append(groups[owner["recipe"]])
            claim = next(c for c in groups[owner["recipe"]]["claims"] if c["target_id"] == owner["target_id"])
            pinned = next(c for c in recipe["claims"] if c["target_id"] == owner["target_id"])
            require(claim["exact"] and claim["fixups"] == owner["fixup_count"] and
                    claim["relocations"] == owner.get("relocation_count", 0) and
                    claim.get("covered_relocation_indices", []) == owner.get("relocation_indices", []) and
                    claim.get("relocation_order", "EXACT") == pinned.get("relocation_order", "EXACT") and
                    claim.get("relocation_order_debt", []) == pinned.get("relocation_order_debt", []),
                    "Group claim no longer exact or its obligations changed")
            continue
        if owner["kind"] == "PINNED_RUNTIME":
            from runtime import verify_member
            accepted.append(verify_member(recipe, oracle, manifest))
            continue
        _, report = compile_and_check(project_path(recipe["source"]), recipe["profile"],
                                      recipe["public"], owner["target_id"],
                                      ROOT / "build/workers/validation" / recipe["name"], recipe["flags"],
                                      language=recipe.get("language", "c"),
                                      binding=read_json(project_path(recipe["binding"])) if "binding" in recipe else None)
        require(report["exact"], "Previously accepted source no longer compiles exactly")
        require(report["object_sha256"] == recipe["object_sha256"], "Accepted OMF identity changed")
        require(report["fixups"] == owner.get("fixup_count", 0) and
                report["relocations"] == owner.get("relocation_count", 0), "Owner obligation coverage changed")
        require(report.get("covered_relocation_indices", []) == owner.get("relocation_indices", []) and
                report.get("covered_relocation_indices", []) == recipe.get("relocation_indices", []),
                "Fresh original relocation index coverage changed")
        check_relocation_order(recipe, report)
        accepted.append(report)
    # Live canonical files must still carry their pins after all compilation, so a
    # PASS never certifies files edited while proofs ran on earlier snapshots.
    check_manifest(oracle, manifest)
    if run_tests:
        result = subprocess.run([sys.executable, "-m", "unittest", "discover", "-s", "tests", "-v"], cwd=ROOT)
        require(result.returncode == 0, "Focused invariant tests failed")
    report = {"status": "PASS", "target_sha256": sha(oracle.data), "accepted": accepted,
              "metrics": metrics(oracle, manifest),
              "limits": ["Component code proof only; RTLink structural closure unrecovered",
                         "Binding modes are external DGROUP offset16 (resident zero addend/word pair, overlay byte/word zero addend) and independently anchored runtime near offset16; root external CALL FAR v1 requires reviewed direct entries plus independent relocated witnesses, exact MZ site coverage and within-symbol order; GROUPED reports retain between-symbol order as structural debt"]}
    return report


def validate(run_tests=True):
    """Invalidate old success before any checks; exceptions never leave PASS."""
    path = ROOT / "build/validation/report.json"
    run = {"run_id": str(uuid4()), "started_at": datetime.now(timezone.utc).isoformat(),
           "tests_requested": run_tests}
    try:
        write_json(path, {**run, "status": "RUNNING"})
    except BaseException:
        # If replacement itself fails, do not leave an old PASS as this run's result.
        path.unlink(missing_ok=True)
        raise
    try:
        require_idle(ROOT)
        manifest_before = (ROOT / "layout/manifest.json").read_bytes()
        report = _validate(run_tests)
        require_idle(ROOT)
        require((ROOT / "layout/manifest.json").read_bytes() == manifest_before,
                "Manifest changed during validation")
        report.update(run, finished_at=datetime.now(timezone.utc).isoformat(),
                      manifest_sha256=sha(manifest_before))
        write_json(path, report)
        return report
    except BaseException as exc:
        failure = {**run, "status": "INTERRUPTED" if isinstance(exc, (KeyboardInterrupt, SystemExit)) else "FAIL",
                   "finished_at": datetime.now(timezone.utc).isoformat(),
                   "error": f"{type(exc).__name__}: {exc}"}
        try:
            write_json(path, failure)
        except OSError as report_error:
            if hasattr(exc, "add_note"):
                exc.add_note(f"Failure report unavailable; prior RUNNING marker remains: {report_error}")
        raise


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--skip-tests", action="store_true", help="Local acceptance check; normal validation runs tests")
    args = parser.parse_args()
    report = validate(not args.skip_tests)
    print(f"PASS: {report['metrics']['exact_function_count']} freshly recompiled exact functions; "
          f"{report['metrics']['matching_c_bytes']} C bytes; "
          f"{report['metrics']['matching_asm_bytes']} ASM bytes; "
          f"{report['metrics']['pinned_runtime_bytes']} pinned runtime bytes; structural link UNRECOVERED")


if __name__ == "__main__":
    main()
