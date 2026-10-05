"""Compile a scratch C candidate and diagnose strict whole-segment equality."""
import argparse

from common import ROOT, project_path, read_json, require, sha, write_json
from match import CompiledObjectRejected, compile_and_check


def rejection_report(error, target_id):
    """Retain full raw CODE identity for deduplication; never score a refusal."""
    report = {"exact": False, "state": "REJECTED", "target_id": target_id, "reason": str(error)}
    if not isinstance(error, CompiledObjectRejected):
        return report
    result = error.result
    module = result.parsed
    code = [s for s in module.segment_defs if s['class'] == 'CODE']
    if len(code) != 1:
        return report
    segment = code[0]
    raw = module.segments.get(segment['name'])
    if (not raw or len(raw) != segment['length'] or
            module.initialized_ranges.get(segment['name']) != [(0, len(raw))]):
        return report
    import json
    report.update(emitted_size=len(raw), emitted_sha256=sha(raw),
                  expected_size=len(error.expected), expected_sha256=sha(error.expected),
                  object_sha256=sha(result.obj_bytes),
                  binding_content_sha256=sha(json.dumps(error.binding, sort_keys=True).encode('utf-8')),
                  object_declarations={"segments": module.segment_defs, "groups": module.groups,
                                       "publics": module.publics, "externals": module.externals},
                  fixup_thread_declarations=[e for r in module.fixups for e in r['resolved']],
                  mismatch_basis="Rejected proof; full raw CODE identity is diagnostic only")
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("target")
    parser.add_argument("source")
    parser.add_argument("--profile", required=True)
    parser.add_argument("--symbol", required=True)
    parser.add_argument("--worker", required=True)
    parser.add_argument("--flags", nargs="+")
    parser.add_argument("--language", choices=("c", "asm"), default="c")
    parser.add_argument("--binding", help="Reviewed original-derived symbolic binding JSON")
    parser.add_argument("--trial", help="Record compact bounded diagnostics using this worker task.json")
    args = parser.parse_args()
    work = project_path("build/workers/" + args.worker)
    require(work.is_relative_to((ROOT / "build/workers").resolve()) and
            work != (ROOT / "build/workers").resolve(), "Search output escapes its worker directory")
    if args.trial:
        from trial_log import check_task
        check_task(work, args.target, args.trial)
        require(project_path(args.source).is_relative_to(work), "Trial candidate must be owned by its worker")
    try:
        result, report = compile_and_check(args.source, args.profile, args.symbol, args.target,
                                         work / "search", args.flags, language=args.language,
                                         binding=read_json(project_path(args.binding)) if args.binding else None)
    except (ValueError, RuntimeError) as error:
        report = rejection_report(error, args.target)
        write_json(work / "report.json", report)
        if args.trial:
            from trial_log import record_trial
            flags = list(error.result.flags) if isinstance(error, CompiledObjectRejected) else args.flags
            record_trial(work, args.target, args.trial, project_path(args.source), args.profile,
                         flags, args.language, report)
        print("REJECTED: " + str(error))
        raise SystemExit(1)
    write_json(work / "report.json", report)
    if args.trial:
        from trial_log import record_trial
        summary = record_trial(work, args.target, args.trial, project_path(args.source), args.profile,
                               list(result.flags), args.language, report)
        print(f"Trial {args.trial}: {summary['unique_code_count']} unique CODE outputs; stop={summary['stop_recommended']}")
    print(f"{report['state']} {report['expected_size']} expected / {report['emitted_size']} emitted bytes")
    print(f"First difference: {report['first_difference']}; full report {work / 'report.json'}")


if __name__ == "__main__":
    main()
