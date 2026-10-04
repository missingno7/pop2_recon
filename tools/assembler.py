"""Pinned MASM runner; assemble source in isolated scratch, never patch output."""
from pathlib import Path
import os
import re
import subprocess

from compiler import CompileError, CompileResult, WORKERS, load_lock, sha256
from omf import OmfReader, OmfError


def verify_assembler(profile):
    lock = load_lock()
    if profile not in lock.get("assemblers", {}):
        raise CompileError(f"unknown assembler profile {profile!r}")
    spec = lock["assemblers"][profile]
    for item in [lock["runner"], *spec["files"]]:
        path = Path(item["path"])
        if not path.is_file() or sha256(path.read_bytes()) != item["sha256"]:
            raise CompileError(f"pinned assembler/runner missing or hash mismatch: {path}")
    return spec


def assemble_asm(source, profile, flags=None, *, workdir, timeout=30):
    spec = verify_assembler(profile)
    selected = list(flags if flags is not None else spec.get("flags", []))
    # Command-line file arguments and extra output paths remain driver-owned.
    if any(flag not in spec.get("allowed_flags", []) for flag in selected):
        raise CompileError("assembler flag is not in the pinned profile allowlist")
    original = Path(source).read_bytes()
    try:
        text = original.decode("ascii")
    except UnicodeDecodeError as error:
        raise CompileError("historical ASM source must be ASCII") from error
    if re.search(r"^[ \t]*(?:include|includelib|incbin)\b", text, re.I | re.M):
        raise CompileError("ASM includes are refused until their exact inputs are pinned")
    staged = text.replace("\r\n", "\n").replace("\r", "\n").replace("\n", "\r\n").encode("ascii")
    work = Path(workdir).resolve()
    if not work.is_relative_to(WORKERS.resolve()):
        raise CompileError(f"assembler workdir must be isolated under {WORKERS}")
    work.mkdir(parents=True, exist_ok=True)
    (work / "UNIT.ASM").write_bytes(staged)
    obj_path = work / "UNIT.OBJ"
    for path in (obj_path, work / "UNIT.LST", work / "UNIT.CRF"):
        if path.exists():
            path.unlink()
    lock = load_lock()
    argv = [lock["runner"]["path"], *lock["runner"]["options"], spec["executable"],
            *selected, "UNIT.ASM,UNIT.OBJ,NUL,NUL;"]
    env = {"PATH": spec["directory"], "MSDOS_PATH": spec["directory"],
           "TEMP": ".", "TMP": ".", "MSDOS_TEMP": ".",
           "SYSTEMROOT": os.environ.get("SYSTEMROOT", r"C:\Windows")}
    exit_code = None
    try:
        completed = subprocess.run(argv, cwd=work, env=env, stdout=subprocess.PIPE,
                                   stderr=subprocess.STDOUT, timeout=timeout,
                                   creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))
        exit_code = completed.returncode
        log = completed.stdout.decode("latin1", "replace")
    except subprocess.TimeoutExpired as error:
        log = (error.stdout or b"").decode("latin1", "replace") + "\nassembler runner timeout\n"
    obj = obj_path.read_bytes() if obj_path.is_file() else None
    parsed = None
    if obj is not None:
        try:
            parsed = OmfReader().read(obj, "UNIT.OBJ")
        except OmfError as error:
            log += f"\nOMF parse refused: {error}\n"
    result = CompileResult(exit_code == 0 and obj is not None and parsed is not None,
                           obj_path if obj is not None else None, log, work, argv, profile, selected,
                           sha256(original), sha256(staged), sha256(obj) if obj is not None else None,
                           parsed, obj, exit_code is None)
    (work / "assembler.log").write_text(log, encoding="latin1")
    from common import write_json
    write_json(work / "receipt.json", {"language": "asm", "profile": profile, "argv": argv,
               "flags": selected, "source_sha256": result.source_sha256,
               "staged_source_sha256": result.staged_source_sha256,
               "object_sha256": result.obj_sha256, "exit_code": exit_code})
    return result
