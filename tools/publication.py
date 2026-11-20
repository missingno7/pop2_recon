"""Recoverable single-writer publication. No byte or binding acceptance rules."""
import argparse
from pathlib import Path

from common import ROOT, atomic_write, json_bytes, read_json, require, sha


def journal_path(root):
    return Path(root) / "build/publication-pending.json"


def require_idle(root):
    require(not journal_path(root).exists(),
            "Unfinished publication; run python tools/publication.py --recover first")


def _new_path(root, relative, *, retired=False):
    root = Path(root).resolve()
    path = (root / relative).resolve()
    allowed = ("src", "asm", "recipes")
    parts = path.parts[len(root.parts):]
    # Relocation publishes renamed bindings, so bindings may be written as well as retired.
    require(path.is_relative_to(root) and
            (parts[0] in allowed or parts[:2] == ("evidence", "bindings")),
            "Publication file must be canonical source, recipe or binding")
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
    retired = [(_new_path(root, item["path"], retired=True), item["sha256"])
               for item in record.get("retire", [])]
    for path, expected in files:
        require(not path.exists() or sha(path.read_bytes()) == expected,
                f"Publication file changed outside transaction; recovery refused: {path}")
        require(not committed or path.is_file(), "Committed publication file missing; recovery refused")
    for path, expected in retired:
        # Rollback never touches retired files. After commit each must be gone or
        # still match its pin, so an edit made meanwhile is never deleted.
        require(not committed or not path.exists() or sha(path.read_bytes()) == expected,
                f"Retired file changed outside transaction; recovery refused: {path}")
    if committed:
        # Re-pin each file immediately before its own unlink. Without file locks a
        # concurrent save in the remaining instant is not excluded; the project has
        # one integrating writer and validation re-checks every live pin.
        for path, expected in retired:
            if path.exists():
                require(sha(path.read_bytes()) == expected,
                        f"Retired file changed outside transaction; recovery refused: {path}")
                path.unlink()
    else:
        for path, _ in files:
            path.unlink(missing_ok=True)
    journal.unlink()
    return "Completed committed publication" if committed else "Rolled back unfinished publication"


def publish(root, files, manifest_before, manifest_after, check, retire=()):
    """Stage new canonical files, run the unchanged gate, commit manifest last.

    `retire` lists canonical files whose ownership the new manifest drops
    (migrated owners). They are deleted only after the manifest commit.
    """
    root = Path(root)
    require_idle(root)
    manifest = root / "layout/manifest.json"
    require(manifest.read_bytes() == manifest_before, "Manifest changed during publication proof")
    files = {Path(path): data for path, data in files.items()}
    for path in files:
        require(_new_path(root, path.relative_to(root).as_posix()) == path.resolve(),
                "Invalid publication path")
        require(not path.exists(), "Canonical publication would overwrite a file")
    retire = [Path(path) for path in retire]
    for path in retire:
        require(_new_path(root, path.relative_to(root).as_posix(), retired=True) == path.resolve() and
                path.is_file() and path not in files, "Invalid retired file")
    after = json_bytes(manifest_after)
    record = {"schema": 1, "before_sha256": sha(manifest_before), "after_sha256": sha(after),
              "files": [{"path": path.relative_to(root).as_posix(), "sha256": sha(data)}
                        for path, data in files.items()]}
    if retire:
        record["retire"] = [{"path": path.relative_to(root).as_posix(), "sha256": sha(path.read_bytes())}
                            for path in retire]
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
        for item in record.get("retire", []):
            require(sha((root / item["path"]).read_bytes()) == item["sha256"],
                    f"Retired file changed during publication: {item['path']}")
        atomic_write(manifest, after)
        # The guarded recovery path completes the commit: it deletes retired files
        # only while they still match their journal pins, else keeps the journal.
        recover(root)
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
