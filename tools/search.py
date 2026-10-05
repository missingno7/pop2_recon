"""Compile a scratch C candidate and diagnose strict whole-segment equality."""
import argparse

from common import ROOT, project_path, read_json, require, write_json
from match import compile_and_check


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
        report = {"exact": False, "state": "REJECTED", "target_id": args.target, "reason": str(error)}
        write_json(work / "report.json", report)
        if args.trial:
            from trial_log import record_trial
            record_trial(work, args.target, args.trial, project_path(args.source), args.profile,
                         args.flags, args.language, report)
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
