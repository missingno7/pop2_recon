"""Compact, worker-local search history. Diagnostic only; never grants ownership."""
import json
import re

from common import ROOT, read_json, require, sha, write_json


def check_task(work, target_id, label):
    require(work.is_relative_to((ROOT / "build/workers").resolve()) and
            work != (ROOT / "build/workers").resolve(), "Trial output must stay in one worker directory")
    require(re.fullmatch(r"[A-Za-z0-9_-]{1,64}", label), "Invalid trial label")
    card = read_json(work / "task.json")
    require(card["target_id"] == target_id and card.get("canonical_head"),
            "Trial does not match a supervisor target/HEAD card")
    require(type(card["max_trials"]) is int and 1 <= card["max_trials"] <= 64 and
            type(card["stagnation_limit"]) is int and 1 <= card["stagnation_limit"] <= card["max_trials"],
            "Invalid bounded trial/stagnation limits")
    path = work / "trials.json"
    history = read_json(path) if path.exists() else {
        "schema": 1, "target_id": target_id, "canonical_head": card["canonical_head"], "trials": []}
    require(history["target_id"] == target_id and history["canonical_head"] == card["canonical_head"],
            "Worker history belongs to another target/HEAD; use a new wave directory")
    require(len(history["trials"]) < card["max_trials"], "Worker trial budget reached; supervisor must review")
    require(all(t["trial"] != label for t in history["trials"]), "Trial label already recorded")
    summary_path = work / "summary.json"
    if summary_path.exists():
        summary = read_json(summary_path)
        require(not summary["stop_recommended"], "Worker exact/stagnation stop reached; supervisor must review")
    return card, history


def score(report):
    if report.get("exact"):
        return (0, 0, 0)
    if report.get("mismatch_count") is None:
        return None
    return (1, abs(report["emitted_size"] - report["expected_size"]), report["mismatch_count"])


def record_trial(work, target_id, label, source, profile, flags, language, report):
    card, history = check_task(work, target_id, label)
    source = source.resolve()
    require(source.is_relative_to(work), "Trial candidate must be owned by its worker")
    source_bytes = source.read_bytes()
    rows = history["trials"]
    code_hash = report.get("emitted_sha256")
    known = set(card.get("known_code_hashes", [])) | {r["emitted_code_sha256"] for r in rows}
    comparison_key = sha(json.dumps({
        "code": code_hash, "original": report.get("expected_sha256"),
        "binding": report.get("binding_content_sha256"),
        "fixups": report.get("fixup_thread_declarations", [])}, sort_keys=True).encode()) if code_hash else None
    row = {"trial": label, "profile": profile, "flags": flags, "language": language,
           "candidate_input": source.relative_to(ROOT).as_posix(), "source_sha256": sha(source_bytes),
           "emitted_code_sha256": code_hash, "emitted_size": report.get("emitted_size"),
           "expected_size": report.get("expected_size"), "first_difference": report.get("first_difference"),
           "mismatch_count": report.get("mismatch_count"), "mismatch_basis": report.get("mismatch_basis"),
           "object_sha256": report.get("object_sha256"), "exact": report.get("exact", False),
           "duplicate_code": bool(code_hash and code_hash in known), "comparison_key": comparison_key,
           "duplicate_result": bool(comparison_key and any(r["comparison_key"] == comparison_key for r in rows)),
           "reason": report.get("reason")}
    prior_scores = [score(r) for r in rows if score(r) is not None]
    current = score(row)
    improved = current is not None and (not prior_scores or current < min(prior_scores))
    summary_path = work / "summary.json"
    previous = read_json(summary_path) if summary_path.exists() else {}
    best = previous.get("best")
    if improved:
        frozen = work / ("best.asm" if language == "asm" else "best.c")
        frozen.write_bytes(source_bytes)
        best = {**row, "candidate_source": frozen.relative_to(ROOT).as_posix()}
    stale = 0 if improved or (code_hash and not row["duplicate_code"]) else previous.get("stale_trials", 0) + 1
    rows.append(row)
    stop = bool(best and best["exact"]) or stale >= card["stagnation_limit"] or len(rows) >= card["max_trials"]
    summary = {"schema": 1, "target_id": target_id, "canonical_head": card["canonical_head"],
               "status": "exact" if best and best["exact"] else "blocked" if stop or not best else "improved",
               "best": best, "trial_count": len(rows), "unique_code_count": len({r["emitted_code_sha256"] for r in rows if r["emitted_code_sha256"]}),
               "stale_trials": stale, "stop_recommended": stop,
               "explanation": "Fresh strict exact candidate; supervisor verification required" if best and best["exact"] else
                   "Budget or repeated output stop; resume only with a new supervisor reasoning lead" if stop else
                   "Best measured diagnostic candidate; no canonical ownership",
               "scope": "Search diagnostics only. Raw CODE duplicates do not imply equal bindings or accepted proof."}
    write_json(work / "trials.json", history)
    write_json(summary_path, summary)
    return summary
