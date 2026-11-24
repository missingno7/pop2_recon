"""Whole-image link of the accepted sources with pinned RTLink/Plus 6.10, compared with the original PRINCE.EXE.

Every accepted component is compiled under its module basename, the modules of `link/` (data no component owns yet)
are added, the game libraries are built from `layout/link-plan.json`, the Microsoft libraries are used unmodified, and
the linker's output is loaded by the same oracle reader as the original. Nothing is patched: the run-time library is
the shipped RTLUTILS.LIB 6.10 with the two accepted modules rebuilt for the original's older revision in their members' places
(`rtlutils` in the plan), and the generated `VARIABLES` module, which gives the variables no accepted source defines yet their original addresses.

The report grants no ownership. It says which bytes of a real link differ, so a wrong name, data order, alignment or
missing definition shows up as a difference."""
import argparse
import json
import os
import re
import shutil
import struct
import subprocess
from pathlib import Path

from assembler import verify_assembler
from common import ROOT, project_path, read_json, require, sha
from compiler import CompileError, compile_c, load_lock
from omf import OmfReader
from oracle import Oracle

PLAN = ROOT / "layout/link-plan.json"
DGROUP = 0x3302


def pinned(spec):
    path = Path(spec["path"])
    data = path.read_bytes()
    require(sha(data) == spec["sha256"], f"Pinned link input changed: {path}")
    return data


def dos(argv, cwd, timeout):
    """One MS-DOS Player process in its staging directory; returns its output."""
    lock = load_lock()["runner"]
    env = {"PATH": str(cwd), "MSDOS_PATH": str(cwd), "TEMP": ".", "TMP": ".", "MSDOS_TEMP": ".",
           "SYSTEMROOT": os.environ.get("SYSTEMROOT", r"C:\Windows")}
    done = subprocess.run([lock["path"], *lock["options"], *argv], cwd=cwd, env=env, stdin=subprocess.DEVNULL,
                          stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=timeout,
                          creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))
    return done.stdout.decode("latin1", "replace")


def assemble(source, basename, work, profile="masm510"):
    """The pinned assembler under a module basename (MASM puts it in THEADR)."""
    spec = verify_assembler(profile)
    work.mkdir(parents=True, exist_ok=True)
    for old in work.glob("*"):
        old.unlink()
    text = source.decode("ascii").replace("\r\n", "\n").replace("\n", "\r\n")
    (work / f"{basename}.ASM").write_bytes(text.encode("ascii"))
    lock = load_lock()["runner"]
    env = {"PATH": spec["directory"], "MSDOS_PATH": spec["directory"], "TEMP": ".", "TMP": ".", "MSDOS_TEMP": ".",
           "SYSTEMROOT": os.environ.get("SYSTEMROOT", r"C:\Windows")}
    names = [*spec["options"], f"{basename}.ASM"] if spec.get("driver") == "ml" else [f"{basename}.ASM,{basename}.OBJ,NUL,NUL;"]
    done = subprocess.run([lock["path"], *lock["options"], spec["executable"], *names],
                          cwd=work, env=env, stdin=subprocess.DEVNULL, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                          timeout=60, creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))
    out = work / f"{basename}.OBJ"
    if done.returncode or not out.is_file():
        raise CompileError(done.stdout.decode("latin1"))
    return out.read_bytes()


def build(name, module, out):
    """Object bytes of one plan entry, compiled under `module` (cached by source, flags and basename)."""
    if name.startswith("link:"):
        row = next(m for m in read_json(ROOT / "link/modules.json") if m["name"] == name[5:])
        source = project_path(row["source"]).read_bytes()
    else:
        row = read_json(ROOT / f"recipes/{name}.json")
        source = project_path(row["source"]).read_bytes()
        require(sha(source) == row["source_sha256"], f"Canonical source changed without acceptance: {name}")
    language = row.get("language", "c")
    key = sha(json.dumps([sha(source), row["profile"], row.get("flags"), module, language]).encode())[:24]
    cached = out / "cache" / f"{key}.obj"
    if cached.is_file():
        return cached.read_bytes()
    work = out / "compile" / re.sub(r"\W", "_", name)
    if language == "asm":
        data = assemble(source, module, work, row["profile"])
    else:
        result = compile_c(source.decode("ascii"), row["profile"], flags=row["flags"], basename=module, workdir=work)
        if not result.ok:
            raise CompileError(result.log)
        data = result.obj_bytes
    cached.parent.mkdir(parents=True, exist_ok=True)
    cached.write_bytes(data)
    return data


def variables(objects, oracle, plan):
    """MASM source that defines every variable no object defines, at its accepted address (plain labels: names inside
    another module's data). Communals are defined by their modules and placed by RTLink itself."""
    defined, used = {n.upper() for n in plan["variables"]["library"]}, set()
    for data in objects.values():
        module = OmfReader().read(data)
        defined |= {p["name"].upper() for p in module.publics} | {c["name"].upper() for c in module.communals}
        used |= {e.upper() for e in module.externals}
    near = {}
    for owner in read_json(ROOT / "layout/manifest.json")["owners"]:
        recipe = read_json(project_path(owner["recipe"]))
        if "binding" not in recipe or recipe.get("kind") == "PINNED_RUNTIME":
            continue
        for name, symbol in read_json(project_path(recipe["binding"])).get("symbols", {}).items():
            if name.upper() in used - defined and symbol.get("kind") == "dgroup-offset":
                near[name] = symbol["offset"]
    rows = ["; Generated by tools/whole_link.py: variables without an accepted definition.", "",
            *(f"EXTRN {n}:BYTE" for n in plan["force"]), "ADDRESSES SEGMENT BYTE PUBLIC 'BEGDATA'"]
    for name, address in sorted(near.items(), key=lambda q: (q[1], q[0])):
        rows += [f"PUBLIC {name}", f"{name} EQU THIS BYTE + 0{address:X}h"]
    rows += ["ADDRESSES ENDS", "", "END", ""]
    text = "\n".join(rows)
    return text.replace("ADDRESSES SEGMENT", "DGROUP GROUP ADDRESSES\nADDRESSES SEGMENT", 1).encode("ascii"), len(near)


def make_library(stage, name, members):
    """An OMF library with the pinned Microsoft LIB, members in the given order."""
    (stage / "LIB.EXE").write_bytes(pinned(read_json(PLAN)["tools"]["lib"]))
    lines = [f"{name}.LIB", "y"] + [f"+{m}.OBJ &" for m in members[:-1]] + [f"+{members[-1]}.OBJ", "NUL", ";"]
    (stage / "LIB.RSP").write_bytes(("\r\n".join(lines) + "\r\n").encode("ascii"))
    log = dos([str((stage / "LIB.EXE").resolve()), "@LIB.RSP"], stage, 300)
    require((stage / f"{name}.LIB").is_file(), "LIB failed: " + log[-400:])
    for m in members:
        (stage / f"{m}.OBJ").unlink()           # only reachable through the library


def rtlutils(plan, out):
    """The shipped RTLUTILS.LIB with each accepted rebuilt module (`tools/manager_asm.py`) in its member's place."""
    data = bytearray(pinned(plan["tools"]["rtlutils"]))
    for row in plan["rtlutils"]:
        at, length = row["at"], row["length"]
        new = build(row["module"], row["basename"], out)
        require(sha(bytes(data[at:at + length])) == row["old_sha256"] and len(new) <= length, "RTLUTILS member does not fit")
        data[at:at + length] = new.ljust(length, b"\0")
    return bytes(data)


def link(out):
    """Compile, build the libraries, link once; returns (EXE bytes, MAP text, number of generated variables)."""
    plan = read_json(PLAN)
    oracle = Oracle.load()
    stage = out / "stage"
    if stage.exists():
        shutil.rmtree(stage)
    stage.mkdir(parents=True)
    objects, files = {}, {}

    def add(name):
        row = plan["modules"][name]
        objects[name] = build(name, row["basename"], out)
        files[name] = row["file"]
    for name in plan["modules"]:
        add(name)
    library_names = {n for lib in plan["libraries"] for n in lib.get("members", [])}
    plan["force"] += [f"LINK_FILL_{k}" for k, f in enumerate(plan["fills"])
                      if (f.get("after") or f["before"]) in library_names]
    source, generated = variables(objects, oracle, plan)
    (out / "VARIABLES.ASM").write_bytes(source)
    (stage / "VARS.OBJ").write_bytes(assemble(source, "VARS", out / "compile/variables"))
    for name, data in objects.items():
        (stage / f"{files[name]}.OBJ").write_bytes(data)
    # Plan `fills`: statics no source declares yet (memory only), and the missing bytes of a draft that is too short,
    # so that everything behind it stays comparable. Each entry is a debt.
    root, members = list(plan["root"]), {lib["name"]: list(lib.get("members", [])) for lib in plan["libraries"]}
    sequences = [root, *(section["files"] for area in plan["areas"] for section in area), *members.values()]
    for number, fill in enumerate(plan["fills"]):
        name, label = f"fill:{number}", f"LINK_FILL_{number}"
        files[name] = f"FILL{number:02}"
        bss = fill["segment"] == "_BSS"
        text = ["DGROUP GROUP _BSS"] if bss else []
        text += [f"{fill['segment']} SEGMENT {'WORD' if bss else 'BYTE'} PUBLIC '{'BSS' if bss else 'CODE'}'",
                 f"PUBLIC {label}", f"{label} LABEL BYTE"]
        text += [f"    DB {fill['size']} DUP (?)"] if bss else ["    DB 0"] * fill["size"]
        source = "\n".join([*text, f"{fill['segment']} ENDS", "END", ""]).encode()
        (stage / f"{files[name]}.OBJ").write_bytes(assemble(source, files[name], out / "compile/fill"))
        neighbour = fill.get("after") or fill["before"]
        where = next(q for q in sequences if neighbour in q)
        where.insert(where.index(neighbour) + ("after" in fill), name)
    lines = ["SYNTAX FREEFORMAT", "VERBOSE", "MAP", "OUTPUT POP", *plan["options"], "VECTOROFF"]
    # The original's first object symbol has id 51 = 40 + 11, but its THRONE vectors are not among the first 40 names
    # (docs/rtlink.md): five further ALWAYS names are unknown. Placeholders keep the symbol numbers.
    lines += [f"ALWAYS {n}" for n in [*plan["vectors"], *(f"UNKNOWN_{k}" for k in range(plan["unknown_prefix_names"]))]]
    lines += [f"FILE {files[n]}" for n in root] + ["FILE VARS"]

    def sections(area, indent):
        out_lines = [indent + "BEGINAREA"]
        for section in area:
            out_lines += [f"{indent}  VECTORON"] if section.get("vectors") else []
            out_lines += [f"{indent}  SECTION = {section['name']}", f"{indent}    COMPRESS ON"]
            out_lines += [f"{indent}    FILE {files[n]}" for n in section["files"]]
            out_lines += [f"{indent}    MODULE {m}" for m in section.get("modules", [])]
            for nested in section.get("areas", []):
                out_lines += sections(nested, indent + "    ")
        return out_lines + [indent + "ENDAREA"]
    for area in plan["areas"]:
        lines += sections(area, "")
    for library in plan["libraries"]:
        lines.append(f"LIBRARY {library['name']}")
        if "members" in library:
            make_library(stage, library["name"], [files[n] for n in members[library["name"]]])
        else:
            (stage / f"{library['name']}.LIB").write_bytes(pinned(library))
    (stage / "LINK.ARF").write_bytes(("\r\n".join(lines) + "\r\n").encode("ascii"))
    (stage / "RTLUTILS.LIB").write_bytes(rtlutils(plan, out))
    pinned(plan["tools"]["rtlink"])
    log = dos([plan["tools"]["rtlink"]["path"], "@LINK.ARF"], stage, 900)
    (out / "link.log").write_text(log, encoding="latin1")
    require((stage / "POP.EXE").is_file(), "RTLink wrote no executable: " + log[-600:])
    return (stage / "POP.EXE").read_bytes(), (stage / "POP.MAP").read_text(encoding="latin1"), generated


def load_linked(candidate, oracle):
    """The linker's output through the oracle reader (its own descriptor table, its own overlay expander)."""
    import overlay_expand
    import oracle as oracle_module
    profile = dict(oracle.profile)
    name = candidate.find(b"\0POP.EXE\0")
    require(name > 0, "No overlay manager in the linked image")
    profile["descriptor_table_file_offset"] = name - 2 - 18 * profile["descriptor_count"]
    profile["payload_shortfall_by_overlay"] = {
        str(profile["first_overlay_id"] + profile["descriptor_count"] - 1): -len(candidate) % 16}
    original_expand = oracle_module.expand
    oracle_module.expand = lambda data, mz, prof, overlay_id, payload, paragraphs: overlay_expand.emulate(
        data, mz, prof["expander_entry"], payload, paragraphs)
    try:
        return Oracle(candidate, profile)
    finally:
        oracle_module.expand = original_expand


def compare(candidate, oracle):
    """Per space: size, differing bytes, relocation sets; plus the MZ header and the overlay payload bytes."""
    linked = load_linked(candidate, oracle)
    report = {"mz_header_equal": candidate[:0x1E] == oracle.data[:0x1E], "file_size": len(candidate),
              "original_file_size": len(oracle.data), "spaces": {}}
    for space_name, space in oracle.spaces.items():
        mine = linked.spaces[space_name]
        count = min(space.size, mine.size)
        different = [i for i in range(count) if space.data[i] != mine.data[i]]
        sites = [(r["image_offset"], r["stored_word"]) for r in space.relocations]
        own = [(r["image_offset"], r["stored_word"]) for r in mine.relocations]
        runs = []
        for i in different:
            if runs and i - runs[-1][1] <= 8:
                runs[-1][1] = i + 1
            else:
                runs.append([i, i + 1])
        row = {"size": space.size, "linked_size": mine.size, "different": len(different) + space.size - count,
               "relocations": len(sites), "relocations_missing": len(set(sites) - set(own)),
               "relocations_same_position": sum(a == b for a, b in zip(sites, own)), "ranges": runs[:60]}
        if space_name != "root":
            row["payload_equal"] = (oracle.data[space.file_offset:space.file_offset + space.descriptor["file_payload_size"]] ==
                                    candidate[mine.file_offset:mine.file_offset + mine.descriptor["file_payload_size"]])
        report["spaces"][space_name] = row
    total = sum(r["size"] for r in report["spaces"].values())
    equal = total - sum(r["different"] for r in report["spaces"].values())
    report.update(bytes_total=total, bytes_equal=equal, identical=candidate == oracle.data,
                  relocations_in_order=sum(r["relocations_same_position"] for r in report["spaces"].values()))
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", default="build/whole_link", help="working directory (under build/)")
    parser.add_argument("--check", action="store_true", help="fail if the result is worse than evidence/whole-link.json")
    parser.add_argument("--record", action="store_true", help="write evidence/whole-link.json (integrating writer)")
    args = parser.parse_args()
    out = project_path(args.output)
    out.mkdir(parents=True, exist_ok=True)
    exe, text, generated = link(out)
    (out / "POP.EXE").write_bytes(exe)
    (out / "POP.MAP").write_text(text, encoding="latin1")
    report = compare(exe, Oracle.load())
    report["generated_variables"] = generated
    (out / "report.json").write_text(json.dumps(report, indent=1) + "\n")
    for name, row in report["spaces"].items():
        print(f"{name:11} {row['size']:6} bytes, {row['different']:5} differ; relocations {row['relocations']:4}, "
              f"missing {row['relocations_missing']}, in original order {row['relocations_same_position']}"
              + ("" if row.get("payload_equal", True) else "; file payload differs"))
    print(f"{report['bytes_equal']} of {report['bytes_total']} image bytes equal "
          f"({100 * report['bytes_equal'] / report['bytes_total']:.2f}%); MZ header equal: {report['mz_header_equal']}; "
          f"{generated} generated variables")
    keep = {k: report[k] for k in ("bytes_total", "bytes_equal", "generated_variables", "mz_header_equal", "relocations_in_order")}
    record = ROOT / "evidence/whole-link.json"
    if args.record:
        record.write_text(json.dumps(keep, indent=1) + "\n", newline="\n")
    if args.check:
        best = read_json(record)
        require(keep["bytes_equal"] >= best["bytes_equal"] and keep["mz_header_equal"] and
                keep["generated_variables"] <= best["generated_variables"], f"Whole link got worse: {keep} < {best}")


if __name__ == "__main__":
    main()
