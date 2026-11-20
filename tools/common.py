"""Small project primitives. Verification never updates expected identities."""
import hashlib
import json
import os
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
# Canonical names: optional original-module directory, then the component name.
CANONICAL_NAME = r"(?:[a-z][a-z0-9_]*/)?[a-z][a-z0-9_]*"


def sha(data):
    return hashlib.sha256(data).hexdigest()


def read_json(path):
    return json.loads(Path(path).read_text(encoding="utf-8"))


def json_bytes(value):
    return (json.dumps(value, indent=2, sort_keys=True) + "\n").encode("utf-8")


def atomic_write(path, data, *, replace=True):
    """Publish a complete file on the same filesystem, never a partial write.

    Exclusive publication uses Windows rename or a POSIX hard link so an
    existing destination cannot be overwritten. The temporary file is closed.
    """
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    fd, temporary = tempfile.mkstemp(prefix=".publish-", suffix=".tmp", dir=path.parent)
    try:
        with os.fdopen(fd, "wb") as stream:
            stream.write(data)
            stream.flush()
            os.fsync(stream.fileno())
        if replace:
            os.replace(temporary, path)
        elif os.name == "nt":
            os.rename(temporary, path)
        else:
            os.link(temporary, path)
    finally:
        Path(temporary).unlink(missing_ok=True)


def write_json(path, value):
    atomic_write(path, json_bytes(value))


def require(condition, message):
    if not condition:
        raise ValueError(message)


def project_path(value):
    path = (ROOT / value).resolve()
    require(path.is_relative_to(ROOT), "Path escapes project")
    return path
