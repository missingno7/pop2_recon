"""Reproduce a source/profile matrix against a reviewed target, without promotion."""
import argparse
import re
from pathlib import Path

from common import ROOT, read_json, write_json, project_path, require, sha
from match import compile_and_check, get_target


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("matrix")
    parser.add_argument("--worker", required=True)
    parser.add_argument("--trial", type=int, help="Run only this zero-based trial")
    args = parser.parse_args()
    require(re.fullmatch(r"[a-zA-Z0-9_-]+", args.worker), "Invalid isolated worker name")
    matrix = read_json(project_path(args.matrix))
    target, _, _ = get_target(matrix["target_id"])
    require(target["sha256"] == matrix["target_sha256"], "Matrix target identity changed")
    work = ROOT / "build/workers" / args.worker
    trials = list(enumerate(matrix["trials"]))
    if args.trial is not None:
        require(0 <= args.trial < len(trials), "Trial index outside matrix")
        trials = [trials[args.trial]]
    rows = []
    for index, trial in trials:
        source = matrix["sources"][trial["source"]]
        raw = source["text"].encode("ascii")
        require(sha(raw) == source["sha256"], "Independent matrix source hash mismatch")
        scratch = work / str(index)
        scratch.mkdir(parents=True, exist_ok=True)
        path = scratch / "candidate.c"
        path.write_bytes(raw)
        row = {"trial": index, **trial, "source_sha256": sha(raw)}
        try:
            _, report = compile_and_check(path, trial["profile"], matrix["public"],
                                           matrix["target_id"], scratch / "compile", trial["flags"])
            row["comparison"] = report
            print(f"{index}: {report['state']}, {report['emitted_size']} bytes, diff {report['first_difference']}", flush=True)
        except (RuntimeError, ValueError) as error:
            row["comparison"] = {"state": "REJECTED", "exact": False, "reason": str(error)}
            print(f"{index}: REJECTED {error}", flush=True)
        rows.append(row)
        write_json(work / "matrix-results.json", {"target_id": matrix["target_id"],
                   "scope": "Complete component comparisons only; this command never grants ownership", "results": rows})


if __name__ == "__main__":
    main()
