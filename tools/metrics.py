"""Honest reconstruction ownership metrics; unknown payload is explicit debt."""
from collections import Counter

from common import ROOT, read_json, require, write_json
from oracle import Oracle


def metrics(oracle=None, manifest=None):
    oracle = oracle or Oracle.load()
    manifest = manifest or read_json(ROOT / "layout/manifest.json")
    rows = []
    for space in oracle.spaces.values():
        owners = [r for r in manifest["owners"] if r["space"] == space.name]
        counts = Counter()
        for owner in owners:
            counts[owner["kind"]] += owner["size"]
        covered = [index for owner in owners for index in owner.get("relocation_indices", [])]
        require(all(type(i) is int and 0 <= i < len(space.relocations) for i in covered) and
                len(covered) == len(set(covered)) and
                all(len(r.get("relocation_indices", [])) == r.get("relocation_count", 0) for r in owners),
                "Invalid/duplicate original relocation coverage indices")
        c = counts["MATCHING_C"]
        asm = counts["MATCHING_ASM"]
        runtime = counts["PINNED_RUNTIME"]
        third = counts["EXTERNAL_DRIVER"]
        rows.append({"space": space.name, "payload_bytes": space.size, "matching_c_bytes": c,
                     "matching_asm_bytes": asm, "pinned_runtime_bytes": runtime,
                     "external_driver_bytes": third,
                     "unresolved_or_unclassified_payload_bytes": space.size-c-asm-runtime-third,
                     "exact_function_count": sum(r["kind"] in ("MATCHING_C", "MATCHING_ASM") for r in owners),
                     "pinned_runtime_component_count": sum(r["kind"] == "PINNED_RUNTIME" for r in owners),
                     "total_relocation_records": len(space.relocations),
                     "covered_relocation_indices": sorted(covered),
                     "proven_relocation_records": len(covered)})
    targets = read_json(ROOT / "evidence/targets.json")["functions"]
    accepted = {r["target_id"] for r in manifest["owners"] if r["kind"] in ("MATCHING_C", "MATCHING_ASM")}
    game = [r for r in targets if r.get("classification") == "GAME_C"]
    attempted = {(r["space"], oracle.spaces[r["space"]].position(r["segment"], r["offset"]))
                 for r in targets if r.get("state") == "CANDIDATE_C" and r["id"] not in accepted}
    report = {"schema": 1, "spaces": rows,
              "total_payload_bytes": sum(r["payload_bytes"] for r in rows),
              "matching_c_bytes": sum(r["matching_c_bytes"] for r in rows),
              "matching_asm_bytes": sum(r["matching_asm_bytes"] for r in rows),
              "pinned_runtime_bytes": sum(r["pinned_runtime_bytes"] for r in rows),
              "pinned_runtime_component_count": sum(r["pinned_runtime_component_count"] for r in rows),
              "identified_game_code_bytes": sum(r["size"] for r in game),
              "unresolved_identified_game_bytes": sum(r["size"] for r in game if r["id"] not in accepted),
              "unknown_or_unclassified_payload_bytes": sum(r["unresolved_or_unclassified_payload_bytes"] for r in rows)
                 - sum(r["size"] for r in game if r["id"] not in accepted),
              "exact_function_count": len(accepted), "reviewed_target_count": len(targets),
              "unresolved_reviewed_target_count": len(targets)-len(accepted),
              "candidate_function_count": len(attempted),
              "total_relocation_records": sum(len(s.relocations) for s in oracle.spaces.values()),
              "proven_relocation_records": sum(r["proven_relocation_records"] for r in rows),
              "proven_candidate_fixups": sum(r.get("fixup_count", 0) for r in manifest["owners"]),
              "structural_link_status": manifest["structural_link_status"],
              "behavior_oracle_status": manifest["behavior_oracle_status"],
              "metric_scope": "Payload includes code, data and linker structures; discovered heuristic extents are not identified game-byte totals"}
    return report


def main():
    report = metrics()
    write_json(ROOT / "build/metrics.json", report)
    for row in report["spaces"]:
        print(f"{row['space']:12} C={row['matching_c_bytes']:5} ASM={row['matching_asm_bytes']:5} "
              f"runtime={row['pinned_runtime_bytes']:5} debt={row['unresolved_or_unclassified_payload_bytes']:6} "
              f"relocs={row['proven_relocation_records']}/{row['total_relocation_records']}")
    print(f"Exact functions: {report['exact_function_count']}; exact C bytes: {report['matching_c_bytes']}")
    print(f"Structural link: {report['structural_link_status']}")


if __name__ == "__main__":
    main()
