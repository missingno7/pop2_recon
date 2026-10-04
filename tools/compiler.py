"""Pinned local Microsoft C compiler driver for historical reconstruction probes."""
from __future__ import annotations

from dataclasses import dataclass
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile

from omf import ObjectModule, OmfError, OmfReader

ROOT = Path(__file__).resolve().parents[1]
LOCK = ROOT / "layout" / "toolchain.lock.json"
WORKERS = ROOT / "build" / "workers"

class CompileError(RuntimeError):
    pass

def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()

def load_lock() -> dict:
    return json.loads(LOCK.read_text(encoding="utf-8"))

def verify_profile(profile: str) -> dict:
    lock = load_lock()
    if profile not in lock["profiles"]:
        raise CompileError(f"unknown compiler profile {profile!r}")
    spec = lock["profiles"][profile]
    runner = lock["runner"]
    paths = [(Path(runner["path"]), runner["sha256"])]
    paths += [(Path(item["path"]), item["sha256"]) for item in spec["files"]]
    for path, expected in paths:
        if not path.is_file():
            raise CompileError(f"pinned tool is missing: {path}")
        actual = sha256(path.read_bytes())
        if actual != expected:
            raise CompileError(f"pinned tool hash mismatch for {path}: {actual}")
    return spec

@dataclass
class CompileResult:
    ok: bool
    obj: Path | None
    log: str
    workdir: Path
    argv: list[str]
    profile: str
    flags: list[str]
    source_sha256: str
    staged_source_sha256: str
    obj_sha256: str | None
    parsed: ObjectModule | None = None
    obj_bytes: bytes | None = None
    timed_out: bool = False

def compile_c(source: str | Path, profile: str, flags: list[str] | None = None, *,
              basename: str = "UNIT", workdir: str | Path | None = None,
              keep: bool = True, timeout: int = 60) -> CompileResult:
    spec = verify_profile(profile)
    timeout = int(spec.get("timeout_seconds", timeout))
    if not basename.isascii() or not basename.isalnum() or len(basename) > 8:
        raise CompileError("DOS input basename must be 1-8 ASCII letters or digits")
    try:
        if isinstance(source, Path):
            original_source = source.read_bytes()
            text = original_source.decode("ascii")
        else:
            text = source
            original_source = text.encode("ascii")
        source_bytes = text.replace("\r\n", "\n").replace("\r", "\n").replace("\n", "\r\n").encode("ascii")
    except (UnicodeDecodeError, UnicodeEncodeError) as exc:
        raise CompileError(f"historical DOS source must be ASCII: {exc}") from exc
    if re.search(r"^[ \t]*#[ \t]*include\b", text, re.MULTILINE):
        raise CompileError("C includes are refused until their exact header files are pinned in the toolchain lock")
    workroot = WORKERS / "toolchain"
    workroot.mkdir(parents=True, exist_ok=True)
    work = Path(workdir).resolve() if workdir is not None else Path(tempfile.mkdtemp(prefix=f"{profile}-", dir=workroot))
    try:
        work.relative_to(WORKERS.resolve())
    except ValueError as exc:
        raise CompileError(f"compiler workdir must be isolated under {WORKERS}") from exc
    work.mkdir(parents=True, exist_ok=True)
    (work / f"{basename}.C").write_bytes(source_bytes)
    selected_flags = list(flags if flags is not None else spec["flags"])
    if not any(flag.lower() == "/c" for flag in selected_flags):
        raise CompileError("compile_c requires the /c switch; linker invocation is a separate experiment")
    for flag in selected_flags:
        if not flag.startswith("/") or any(char in flag for char in (" ", "\t", ":", "\\")):
            raise CompileError(f"compiler flag is not a plain DOS switch: {flag!r}")
    exe = Path(spec["executable"])
    for stale in (work / f"{basename}.OBJ", work / f"{basename}.ASM"):
        if stale.exists():
            stale.unlink()
    argv = [load_lock()["runner"]["path"], *load_lock()["runner"]["options"],
            str(exe), *selected_flags, f"{basename}.C"]
    search = [spec["directory"], *spec.get("path_extra", [])]
    env = {"PATH": ";".join(search), "MSDOS_PATH": ";".join(search),
           "TEMP": ".", "TMP": ".", "MSDOS_TEMP": ".",
           "SYSTEMROOT": os.environ.get("SYSTEMROOT", "C:\\Windows")}
    try:
        completed = subprocess.run(argv, cwd=work, env=env, stdout=subprocess.PIPE,
                                   stderr=subprocess.STDOUT, timeout=timeout,
                                   creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))
        exit_code = completed.returncode
        log = completed.stdout.decode("latin1", "replace")
    except subprocess.TimeoutExpired as exc:
        output = exc.stdout or b""
        log = output.decode("latin1", "replace") + f"\nrunner timeout after {timeout} seconds\n"
        exit_code = None
    obj_path = work / f"{basename}.OBJ"
    obj = obj_path.read_bytes() if obj_path.is_file() else None
    parsed = None
    if obj is not None:
        try:
            parsed = OmfReader().read(obj, obj_path.name)
        except OmfError as exc:
            log += f"\nOMF parse refused: {exc}\n"
    result = CompileResult(exit_code == 0 and obj is not None and parsed is not None,
                           obj_path if obj is not None else None, log, work, argv, profile, selected_flags,
                           sha256(original_source), sha256(source_bytes),
                           sha256(obj) if obj is not None else None, parsed, obj, exit_code is None)
    (work / "compiler.log").write_text(log, encoding="latin1")
    receipt = {"profile": profile, "argv": argv, "flags": selected_flags,
               "source_sha256": result.source_sha256,
               "staged_source_sha256": result.staged_source_sha256,
               "object_sha256": result.obj_sha256,
               "exit_code": exit_code, "workdir": str(work)}
    (work / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    if workdir is None and not keep and result.ok:
        shutil.rmtree(work, ignore_errors=True)
    return result

def extract_function(obj: ObjectModule | CompileResult, public_name: str) -> bytes:
    """Return a complete one-function CODE contribution, never a caller-sized slice."""
    if isinstance(obj, CompileResult):
        if not obj.ok or obj.parsed is None:
            raise CompileError("compile result did not produce a parsed OMF object")
        obj = obj.parsed
    found = [p for p in obj.publics if p["name"] == public_name]
    if len(found) != 1:
        raise CompileError(f"expected one PUBLIC {public_name!r}; found {len(found)}")
    public = found[0]
    seg = public["segment"]
    if seg is None or public["offset"] != 0:
        raise CompileError(f"{public_name!r} must begin at offset zero of its owning segment")
    if len(obj.publics) != 1:
        raise CompileError("single-function extraction requires exactly one PUBLIC in the object")
    defs = [d for d in obj.segment_defs if d["name"] == seg]
    if len(defs) != 1 or defs[0]["class"].upper() != "CODE":
        raise CompileError(f"{public_name!r} does not have one unambiguous CODE SEGDEF")
    if len([p for p in obj.publics if p["segment"] == seg]) != 1:
        raise CompileError("owning CODE segment has additional PUBLIC symbols")
    if any(d["name"] != seg and (d["length"] != 0 or d["class"].upper() == "CODE")
           for d in obj.segment_defs):
        raise CompileError("object has nonzero storage outside the owning CODE segment")
    payload = obj.segments.get(seg)
    if payload is None:
        raise CompileError("owning CODE segment has no initialized bytes")
    declared = defs[0]["length"]
    if declared <= 0:
        raise CompileError("owning CODE segment has an empty extent")
    if len(payload) != declared:
        raise CompileError(f"initialized CODE bytes {len(payload)} != SEGDEF extent {declared}")
    if obj.initialized_ranges.get(seg) != [(0, declared)]:
        raise CompileError("CODE bytes are not fully initialized across the declared extent")
    if any(f["segment_index"] in (None, public["segment_index"]) and
           any(row.get("kind") == "fixup" for row in f["decoded"])
           for f in obj.fixups):
        raise CompileError("CODE segment carries FIXUPP records; unresolved fields cannot be compared as final code")
    return payload
