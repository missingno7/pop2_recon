"""Fresh authoritative bootstrap validation; never alters locks or oracle assets."""
import argparse
import json
import struct
import subprocess
import sys
from datetime import datetime, timezone
from uuid import uuid4

from common import ROOT, read_json, read_json_shared, require, sha, project_path, write_json
from match import compile_and_check, get_table, get_target
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
                                                      ("MATCHING_ASM_DATA", "ASM_DATA_EXACT"),
                                                      ("REBUILT_RUNTIME", "ASM_EXACT"),
                                                      ("PINNED_RUNTIME", "PINNED_RUNTIME")),
                "Unsupported bootstrap ownership kind/state")
        recipe = read_json_shared(project_path(owner["recipe"]))
        require((owner["kind"] == "REBUILT_RUNTIME") == (recipe.get("kind") == "manager_asm"),
                "A rebuilt run-time module has its own owner kind")
        if owner["kind"] in ("MATCHING_C", "MATCHING_ASM", "REBUILT_RUNTIME"):
            target, space, _ = get_target(owner["target_id"], oracle)
        elif owner["kind"] == "MATCHING_ASM_DATA":
            target, space, _ = get_table(owner["target_id"], oracle)
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
        pinned = (next((c.get("relocation_indices", []) for c in recipe["claims"] + recipe.get("tables", [])
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
            table = owner["kind"] == "MATCHING_ASM_DATA"
            require((table and recipe.get("language") == "asm" or
                     owner["kind"] == ("MATCHING_ASM" if recipe.get("language", "c") == "asm" else "MATCHING_C")) and
                    owner.get("group") == recipe["name"],
                    "Group owner does not name its group recipe")
            claim = [c for c in (recipe.get("tables", []) if table else recipe["claims"])
                     if c["target_id"] == owner["target_id"]]
            require(len(claim) == 1 and claim[0]["fixup_count"] == owner["fixup_count"],
                    "Group owner is not exactly one recipe claim")
            group_claims.setdefault(owner["recipe"], set()).add(owner["target_id"])
        else:
            require(recipe["target_id"] == owner["target_id"] and owner["kind"] != "MATCHING_ASM_DATA", "Recipe target mismatch")
        source = project_path(recipe["source"])
        language = recipe.get("language", "c")
        require(language == ("c" if owner["kind"] == "MATCHING_C" else "asm"),
                "Recipe source language disagrees with ownership")
        source_dir = ROOT / ("asm" if language == "asm" else "src")
        require(source.is_relative_to(source_dir), "Canonical source outside its language directory")
        if owner["kind"] in ("MATCHING_ASM", "REBUILT_RUNTIME"):
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
        group_recipe = read_json_shared(project_path(path))
        require(claimed == {c["target_id"] for c in group_recipe["claims"] + group_recipe.get("tables", [])},
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
    matches = recipe.get("code_matches", [])
    _, report = compile_group(source.decode("ascii"), recipe["profile"], recipe["flags"],
                              [{"target_id": c["target_id"], "public": c["public"]} for c in recipe["claims"] + matches],
                              ROOT / "build/workers/validation" / recipe["name"],
                              binding=binding, group_id=recipe["group_id"], language=recipe.get("language", "c"),
                              tables=[{"target_id": t["target_id"], "public": t["public"]} for t in recipe.get("tables", [])])
    # Code matches (exact code, listed unproven bindings) are rechecked but never counted as exact.
    match_ids = {c["target_id"] for c in matches}
    fresh = {c["target_id"]: c for c in report["claims"]}
    require(all(fresh[c["target_id"]]["exact"] for c in recipe["claims"]),
            "Previously accepted group no longer compiles exactly")
    require(all(fresh[c["target_id"]]["state"] == "CODE_MATCH" and fresh[c["target_id"]]["unproven"] == c["unproven"]
                for c in matches), "Recorded code match changed (or its bindings became proven: republish it as a claim)")
    report["code_matches"] = [fresh[c] for c in sorted(match_ids)]
    report["claims"] = [c for c in report["claims"] if c["target_id"] not in match_ids]
    report["exact"] = True
    require(report["object_sha256"] == recipe["object_sha256"], "Accepted group OMF identity changed")
    require(not report["object_order"], "Relocation order inside the file differs from the original: " +
            ", ".join(report["object_order"]))
    require(all(report[key] == recipe[key] for key in ("space", "origin", "emitted_size", "frame", "drafts")),
            "Accepted group placement, extent or draft record changed")
    require([{k: t[k] for k in ("target_id", "public", "fixups")} for t in report.get("tables", [])] ==
            [{"target_id": t["target_id"], "public": t["public"], "fixups": t["fixup_count"]}
             for t in recipe.get("tables", [])], "Accepted code tables changed")
    # Data publics ground other groups' externals (module_gate._provider_public); old recipes have none.
    require(report["data_publics"] == recipe.get("data_publics", []) and
            report.get("provider_span_aliases", []) == recipe.get("provider_span_aliases", []),
            "Accepted group data publics or provider span alias debt changed")
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
    aligned = []  # (component, segment, SEGDEF alignment, start) for the whole-link alignment check
    for owner in manifest["owners"]:
        recipe = read_json(project_path(owner["recipe"]))
        if recipe.get("kind") == GROUP_KIND:
            if owner["recipe"] not in groups:
                groups[owner["recipe"]] = verify_group(recipe)
                accepted.append(groups[owner["recipe"]])
            if owner["kind"] == "MATCHING_ASM_DATA":
                table = next(t for t in groups[owner["recipe"]]["tables"] if t["target_id"] == owner["target_id"])
                require(table["exact"] and table["fixups"] == owner["fixup_count"] and not owner["relocation_count"],
                        "Code table owner differs from its fresh proof")
                continue
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
        if recipe.get("kind") == "manager_asm":
            from manager_asm import verify_recipe
            report = verify_recipe(recipe, oracle, manifest)
            require(owner["fixup_count"] == report["fixups"] and owner["relocation_count"] == report["relocations"] and
                    owner["relocation_indices"] == report["relocation_indices"], "Manager module owner differs from its proof")
            accepted.append(report)
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
        target, space, _ = get_target(owner["target_id"], oracle)
        aligned.append((recipe["name"], "CODE", report["segment"]["alignment"],
                        space.position(target["segment"], target["offset"])))
        accepted.append(report)
    # Data-only groups (string/table modules without code): recompiled and rechecked like groups;
    # their placements join the overlap check below.
    from module_gate import DATA_GROUP_KIND, compile_data_group
    for path in manifest.get("data_groups", []):
        recipe = read_json(project_path(path))
        require(recipe.get("kind") == DATA_GROUP_KIND, "data_groups lists a recipe that is not a data-only group")
        source = project_path(recipe["source"]).read_bytes()
        require(sha(source) == recipe["source_sha256"], "Canonical source changed without acceptance")
        raw = project_path(recipe["binding"]).read_bytes()
        require(sha(raw) == recipe["binding_sha256"], "Canonical binding evidence changed without acceptance")
        _, report = compile_data_group(source.decode("ascii"), recipe["profile"], recipe["flags"],
                                       ROOT / "build/workers/validation" / recipe["name"],
                                       binding=json.loads(raw), group_id=recipe["group_id"])
        require(report["object_sha256"] == recipe["object_sha256"], "Accepted data group OMF identity changed")
        require(report["placements"] == recipe["placements"] and report["data_publics"] == recipe["data_publics"],
                "Accepted data group placement or data publics changed")
        groups[path] = report
    # A data public grounds another file's EXTDEF by name: each must be the only accepted
    # definition of that name, as a successful link requires.
    from module_gate import _accepted_recipes, accepted_public_names
    recipes = _accepted_recipes()
    counts = accepted_public_names(recipes)
    clashes = sorted(d["name"] for _, r in recipes for d in r.get("data_publics", []) if counts[d["name"]] != 1)
    require(not clashes, f"Accepted data publics defined more than once: {clashes}")
    # Two accepted files cannot both define the same original DGROUP bytes: placed group
    # segments must be pairwise disjoint. Overlaps accepted before 2026-11-22 are recorded in
    # evidence/placement-overlap-debt.json; that list may only shrink.
    placed = sorted((p["placement"], p["placement"] + p["length"], f"{path}:{seg}")
                    for path, g in groups.items() for seg, p in (g.get("placements") or {}).items())
    by_recipe = {owner.get("recipe"): r for owner, r in recipes}

    def common(key):
        """Segment name when the piece is declared with COMMON combine (contributions of one name overlay)."""
        path, seg = key.rsplit(":", 1)
        recipe = by_recipe.get(path) or read_json(project_path(path))
        declared = read_json_shared(project_path(recipe["binding"]))["declarations"]["segments"]
        return seg if any(d["name"] == seg and d["combine"] == 6 for d in declared) else None
    overlaps = {f"{a[2]}|{b[2]}" for i, a in enumerate(placed) for b in placed[i + 1:]
                if b[0] < a[1] and not (a[:2] == b[:2] and common(a[2]) and common(a[2]) == common(b[2]))}
    # Own far data (docs/far-call-binding.md): paragraphs of different groups are disjoint too.
    far = sorted((p["space"], p["paragraph"] * 16, p["paragraph"] * 16 + p["length"], f"{path}:{seg}")
                 for path, g in groups.items() for seg, p in (g.get("far_placements") or {}).items())
    require(not any(a[0] == b[0] and b[1] < a[2] for a, b in zip(far, far[1:])), "Own far data of different groups overlaps")
    # Placement brackets (docs/dgroup-binding.md): each neighbour's own placement is fixed by its fresh content.
    by_group = {recipe.get("group_id"): path for path, recipe in by_recipe.items()}
    for path, report in groups.items():
        for seg, bracket in ((read_json_shared(project_path((by_recipe.get(path) or read_json(project_path(path)))["binding"]))
                              .get("brackets", {}) if report.get("placements") else {})).items():
            for group in bracket.values():
                piece = groups[by_group[group]]["placements"][seg]
                require(piece["unique"] and piece["basis"].startswith("initialized content equals") and
                        "bracketed" not in piece["basis"], f"Placement bracket {group} is not fixed by its own content")
    overlap_debt = set(read_json(ROOT / "evidence/placement-overlap-debt.json")["pairs"])
    require(overlaps == overlap_debt, "Placed DGROUP segments of different groups overlap: "
            f"unlisted {sorted(overlaps - overlap_debt)}, stale {sorted(overlap_debt - overlaps)}")
    # Whole-link consistency (docs/acceptance.md "One name per entry", "Segment alignment"):
    # violations may only shrink.
    from module_gate import alignment_violations, name_consistency_violations
    by_path = {owner.get("recipe"): recipe for owner, recipe in recipes}
    for path, report in groups.items():
        recipe = by_path.get(path) or read_json(project_path(path))
        component = recipe.get("group_id") or recipe["name"]
        if report.get("emitted_size"):
            aligned.append((component, "CODE", report["segment"]["alignment"], report["origin"]))
        if report.get("placements"):
            declared = {s["name"]: s["alignment"]
                        for s in read_json(project_path(recipe["binding"]))["declarations"]["segments"]}
            aligned += [(component, seg, declared[seg], p["placement"])
                        for seg, p in report["placements"].items() if p["length"]]
    from tu_census import overlay_vectors
    code = {n: s for n, s in oracle.spaces.items() if n not in ("root", "overlay-17")}
    vectors = {k: f"{v['target_space']}:{v['target_segment']:04x}:{v['target_offset']:04x}"
               for k, v in overlay_vectors(oracle, code).items()}
    link = name_consistency_violations(recipes, vectors) | alignment_violations(aligned)
    link_debt = set(read_json(ROOT / "evidence/link-consistency-debt.json")["violations"])
    require(link == link_debt, "evidence/link-consistency-debt.json differs from the whole-link violations: "
            f"unlisted {sorted(link - link_debt)}, stale {sorted(link_debt - link)}")
    # BSS layout refutation (docs/dgroup-binding.md "BSS layout"): violations may only shrink.
    from module_gate import bss_layout_violations
    bss = bss_layout_violations(groups, recipes)
    bss_debt = set(read_json(ROOT / "evidence/bss-layout-debt.json")["violations"])
    require(bss == bss_debt, "evidence/bss-layout-debt.json differs from the BSS layout violations: "
            f"unlisted {sorted(bss - bss_debt)}, stale {sorted(bss_debt - bss)}")
    # Ambiguous immediate witnesses (docs/dgroup-binding.md "Immediate witnesses"): violations may only shrink.
    from module_gate import immediate_witness_violations
    immediate = immediate_witness_violations(recipes, oracle)
    immediate_debt = set(read_json(ROOT / "evidence/immediate-witness-debt.json")["violations"])
    require(immediate == immediate_debt, "evidence/immediate-witness-debt.json differs from the ambiguous immediate "
            f"witnesses: unlisted {sorted(immediate - immediate_debt)}, stale {sorted(immediate_debt - immediate)}")
    # Placement debt may only shrink: every listed claim must still be owned and still need it.
    debt = {c["target_id"] for g in groups.values() for c in g.get("claims", []) if c.get("placement_debt")}
    listed = set(read_json(ROOT / "evidence/placement-debt.json")["targets"])
    require(debt == listed, "evidence/placement-debt.json differs from the claims that still need it: "
            f"stale {sorted(listed - debt)}, unlisted {sorted(debt - listed)}")
    # Explicit segment placement debt may only shrink: the ledger lists exactly the accepted
    # C sources that still use _segname.
    from module_gate import SEGNAME
    segname = {r.get("group_id") or "recipe:" + r["name"] for o, r in recipes
               if o.get("kind") != "PINNED_RUNTIME" and r.get("language", "c") == "c" and "source" in r
               and SEGNAME.search(project_path(r["source"]).read_bytes())}
    const_debt = set(read_json(ROOT / "evidence/const-layout-debt.json")["keys"])
    require(segname == const_debt, "evidence/const-layout-debt.json differs from the sources using _segname: "
            f"stale {sorted(const_debt - segname)}, unlisted {sorted(segname - const_debt)}")
    from module_gate import case_aliases
    aliases = {}
    for o, r in recipes:
        if o.get("kind") != "PINNED_RUNTIME" and r.get("language", "c") == "c" and "source" in r:
            found = case_aliases(project_path(r["source"]).read_bytes())
            if found:
                aliases[r.get("group_id") or "recipe:" + r["name"]] = found
    alias_debt = read_json(ROOT / "evidence/case-alias-debt.json")["aliases"]
    require(aliases == {k: sorted(v) for k, v in alias_debt.items()},
            f"evidence/case-alias-debt.json differs from the accepted sources' case aliases: {aliases}")
    # Everything accepted, linked as one program and compared with the whole original file (docs/rtlink.md).
    import whole_link
    linked = whole_link.verify(ROOT / "build/workers/validation/whole_link", oracle)
    # Live canonical files must still carry their pins after all compilation, so a
    # PASS never certifies files edited while proofs ran on earlier snapshots.
    check_manifest(oracle, manifest)
    if run_tests:
        result = subprocess.run([sys.executable, "-m", "unittest", "discover", "-s", "tests", "-v"], cwd=ROOT)
        require(result.returncode == 0, "Focused invariant tests failed")
    report = {"status": "PASS", "target_sha256": sha(oracle.data), "accepted": accepted,
              "metrics": metrics(oracle, manifest), "placement_debt_claims": len(debt),
              "code_matches": [{"target_id": c["target_id"], "public": c["public"], "size": c["expected_size"],
                                "unproven": c["unproven"]}
                               for g in groups.values() for c in g.get("code_matches", [])],
              "placement_overlap_pairs": len(overlaps),
              "whole_link": {k: v for k, v in linked.items() if k != "spaces"},
              "whole_link_summary": whole_link.summary(linked),
              "limits": ["Ownership is component code proof; the whole link is compared separately and grants none",
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
          f"{report['metrics']['matching_asm_data_bytes']} bytes in {report['metrics']['code_table_count']} code tables; "
          f"{report['metrics']['pinned_runtime_bytes']} pinned runtime bytes; "
          f"{report['metrics']['rebuilt_runtime_bytes']} rebuilt runtime bytes; "
          f"{report['placement_debt_claims']} claims with placement debt; "
          f"{len(report['code_matches'])} code matches ({sum(c['size'] for c in report['code_matches'])} bytes) "
          f"with unproven bindings, not counted; {report['whole_link_summary']}")


if __name__ == "__main__":
    main()
