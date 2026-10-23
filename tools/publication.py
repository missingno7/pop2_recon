"""Recoverable single-writer publication. No byte or binding acceptance rules."""
import argparse
from pathlib import Path

from common import ROOT, atomic_write, json_bytes, read_json, require, sha


def journal_path(root):
    return Path(root) / "build/publication-pending.json"


def require_idle(root):
    require(not journal_path(root).exists(),
            "Unfinished publication; run python tools/publication.py --recover first")


def _new_path(root, relative):
    root = Path(root).resolve()
    path = (root / relative).resolve()
    require(path.is_relative_to(root) and path.parts[len(root.parts)] in ("src", "asm", "recipes"),
            "Publication file must be canonical source or recipe")
    return path


def recover(root):
    """Rollback pre-commit files, or finish a committed transaction.

    Preflight every path before removing anything. External edits are preserved
    and leave the journal in place for manual review. Recovery is idempotent.
    """
    root = Path(root)
    journal = journal_path(root)
    if not journal.exists():
        return "No unfinished publication"
    record = read_json(journal)
    require(record["schema"] == 1, "Unsupported publication journal")
    manifest = root / "layout/manifest.json"
    current = sha(manifest.read_bytes())
    require(current in (record["before_sha256"], record["after_sha256"]),
            "Manifest changed outside publication; recovery refused")
    committed = current == record["after_sha256"]
    files = [(_new_path(root, item["path"]), item["sha256"]) for item in record["files"]]
    for path, expected in files:
        require(not path.exists() or sha(path.read_bytes()) == expected,
                f"Publication file changed outside transaction; recovery refused: {path}")
        require(not committed or path.is_file(), "Committed publication file missing; recovery refused")
    if not committed:
        for path, _ in files:
            path.unlink(missing_ok=True)
    journal.unlink()
    return "Completed committed publication" if committed else "Rolled back unfinished publication"


def publish(root, files, manifest_before, manifest_after, check):
    """Stage new canonical files, run the unchanged gate, commit manifest last."""
    root = Path(root)
    require_idle(root)
    manifest = root / "layout/manifest.json"
    require(manifest.read_bytes() == manifest_before, "Manifest changed during publication proof")
    files = {Path(path): data for path, data in files.items()}
    for path in files:
        require(_new_path(root, path.relative_to(root).as_posix()) == path.resolve(),
                "Invalid publication path")
        require(not path.exists(), "Canonical publication would overwrite a file")
    after = json_bytes(manifest_after)
    record = {"schema": 1, "before_sha256": sha(manifest_before), "after_sha256": sha(after),
              "files": [{"path": path.relative_to(root).as_posix(), "sha256": sha(data)}
                        for path, data in files.items()]}
    # Exclusive journal creation also refuses a competing publisher.
    atomic_write(journal_path(root), json_bytes(record), replace=False)
    try:
        # A published owner makes any earlier validation report obsolete.
        report = root / "build/validation/report.json"
        atomic_write(report, json_bytes({"status": "INVALIDATED", "reason": "Publication started"}))
        for path, data in files.items():
            atomic_write(path, data, replace=False)
        check()
        require(manifest.read_bytes() == manifest_before, "Manifest changed before publication commit")
        atomic_write(manifest, after)
        journal_path(root).unlink()
    except BaseException as exc:
        try:
            recover(root)
        except BaseException as recovery_error:
            if hasattr(exc, "add_note"):
                exc.add_note(f"Publication journal retained: {recovery_error}")
        raise


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--recover", action="store_true", required=True)
    parser.parse_args()
    print(recover(ROOT))


if __name__ == "__main__":
    main()
