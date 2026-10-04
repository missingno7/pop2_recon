"""Compile a scratch C candidate and diagnose strict whole-segment equality."""
import argparse

from common import project_path, write_json
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
    args = parser.parse_args()
    work = project_path("build/workers/" + args.worker)
    try:
        result, report = compile_and_check(args.source, args.profile, args.symbol, args.target,
                                         work / "search", args.flags, language=args.language)
    except (ValueError, RuntimeError) as error:
        report = {"exact": False, "state": "REJECTED", "target_id": args.target, "reason": str(error)}
        write_json(work / "report.json", report)
        print("REJECTED: " + str(error))
        raise SystemExit(1)
    write_json(work / "report.json", report)
    print(f"{report['state']} {report['expected_size']} expected / {report['emitted_size']} emitted bytes")
    print(f"First difference: {report['first_difference']}; full report {work / 'report.json'}")


if __name__ == "__main__":
    main()
