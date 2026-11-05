#!/usr/bin/env python3
"""Build the isolated Debug overlay against an exact official source revision.

This operates only on an explicitly supplied source checkout. It never resets a
checkout, updates the official pin, publishes a release, or handles signing keys.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import zipfile

ROOT = Path(__file__).resolve().parents[1]
DATA = ROOT / "debug"
PIN = json.loads((DATA / "upstream.json").read_text(encoding="utf8"))


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def run(argv: list[str], source: Path, *, capture=False, timeout=180):
    return subprocess.run(argv, cwd=source, check=True, timeout=timeout,
                          stdout=subprocess.PIPE if capture else None,
                          env={**os.environ, "NODE_OPTIONS": "--max-old-space-size=3072"})


def git(source: Path, *argv: str) -> str:
    return run(["git", "-c", "core.autocrlf=false", *argv], source,
               capture=True).stdout.decode("utf8").strip()


def check_pin(source: Path):
    if git(source, "rev-parse", "HEAD") != PIN["officialCommit"]:
        raise SystemExit("Wrong official revision. Use the exact commit in debug/upstream.json.")
    if digest((DATA / "game.patch").read_bytes()) != PIN["patchSha256"]:
        raise SystemExit("Debug patch checksum differs from its reviewed source map.")


def check_sources(source: Path) -> bool:
    return all((source / name).is_file() and
               digest((source / name).read_bytes().replace(b"\r\n", b"\n")) == expected
               for name, expected in PIN["patchedSourceSha256"].items())


def prepare(source: Path):
    check_pin(source)
    if not check_sources(source):
        # Never apply over an unknown partial migration or local source changes.
        if git(source, "status", "--porcelain", "--untracked-files=all", "--", "src", "test"):
            raise SystemExit("Source changes do not match the map. Use a separate clean official checkout.")
        run(["git", "-c", "core.autocrlf=false", "apply", "--check", str(DATA / "game.patch")], source)
        run(["git", "-c", "core.autocrlf=false", "apply", str(DATA / "game.patch")], source)
    if not check_sources(source):
        raise SystemExit("Patched source does not match the reviewed source map.")
    shutil.copyfile(DATA / "vite.config.ts", source / "debug.vite.config.ts")
    shutil.copyfile(DATA / "game.env", source / ".env.debug")


def merge(base: dict, overlay: dict) -> dict:
    for key, value in overlay.items():
        if isinstance(value, dict) and isinstance(base.get(key), dict):
            merge(base[key], value)
        else:
            base[key] = value
    return base


def locale_overlays(source: Path) -> dict[str, bytes]:
    result = {}
    delta = json.loads((DATA / "locale-delta.json").read_text(encoding="utf8"))
    for module, overlay in delta.items():
        # Read the pinned original, never accumulate edits from a previous build.
        original = json.loads(git(source / "locales", "show", f"HEAD:en/{module}.json"))
        result[f"locales/en/{module}.json"] = (json.dumps(merge(original, overlay),
                                                          ensure_ascii=False, indent=2) + "\n").encode("utf8")
    return result


def check_submodules(source: Path):
    for name in ("assets", "locales"):
        if git(source / name, "rev-parse", "HEAD") != PIN[f"{name}Commit"]:
            raise SystemExit(f"Wrong {name} revision; initialize the pinned official submodule.")


def package(source: Path, out: Path, checks: dict):
    check_pin(source)
    check_submodules(source)
    if not check_sources(source):
        raise SystemExit("Source changed since review; regenerate and review its mapping first.")
    dist = source / "dist"
    if not (dist / "index.html").is_file():
        raise SystemExit("No compiled Debug game. Run the build command first.")
    files = {p.relative_to(dist).as_posix(): p.read_bytes() for p in dist.rglob("*")
             if p.is_file() and p.suffix != ".map"}
    if any(name.startswith(("images/", "audio/", "fonts/")) for name in files):
        raise SystemExit("Unexpected bulk game assets in Debug output.")
    files.update(locale_overlays(source))
    files["daily-seeds.json"] = (DATA / "daily-seeds.json").read_bytes()
    files["manifest.json"] = b'{"manifest":{}}\n'
    marker = {"schema": "pokerogue-debug-v1", "version": PIN["debugVersion"],
              "baseVersion": PIN["baseVersion"], "officialCommit": PIN["officialCommit"],
              "officialVersion": PIN["gameVersion"], "assetsCommit": PIN["assetsCommit"],
              "localesCommit": PIN["localesCommit"], "saveProfile": "debug",
              "accountAccess": False, "minigames": False, "legacyControllers": False,
              "googleDrive": False, "files": {name: digest(data) for name, data in sorted(files.items())}}
    files["pokerogue-debug.json"] = (json.dumps(marker, indent=2) + "\n").encode("utf8")
    if len(files["pokerogue-debug.json"]) > 4096:
        raise SystemExit("Debug marker exceeds the native reader's 4096-byte limit.")
    out.mkdir(parents=True, exist_ok=True)
    archive = out / "debug-pack.zip"
    # Fixed timestamps and ordering make identical compiler output reproducible.
    with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED, compresslevel=6) as zipped:
        for name, data in sorted(files.items()):
            info = zipfile.ZipInfo(name, (2026, 1, 1, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = 0o100644 << 16
            zipped.writestr(info, data)
    with zipfile.ZipFile(archive) as zipped:
        if zipped.testzip() is not None:
            raise SystemExit("Archive verification failed.")
    receipt = {"file": archive.name, "bytes": archive.stat().st_size,
               "sha256": digest(archive.read_bytes()), "officialCommit": PIN["officialCommit"],
               "sourceMapSha256": digest((DATA / "upstream.json").read_bytes()),
               "patchSha256": PIN["patchSha256"], "checksThisInvocation": checks,
               "deviceTesting": "See docs/debug-testing.md; packaging does not establish hardware coverage."}
    (out / "debug-build.json").write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf8")
    print(json.dumps(receipt, indent=2))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("command", choices=("prepare", "verify", "build", "package"))
    parser.add_argument("--source", required=True, type=Path)
    parser.add_argument("--output", type=Path, default=ROOT / "outputs/Debug")
    args = parser.parse_args()
    source = args.source.resolve(strict=True)
    if args.command == "prepare":
        prepare(source)
        print("Pinned Debug source applied and verified. Initialize assets/locales and install pnpm dependencies next.")
        return
    check_pin(source)
    if not check_sources(source):
        raise SystemExit("Debug source differs from the reviewed map; use prepare on a clean pinned checkout.")
    check_submodules(source)
    if args.command == "verify":
        print("Official, asset, locale and patched-source pins verified.")
        return
    checks = {"sourcePins": True, "typecheck": "not run", "featureTests": "not run", "viteBuild": "not run"}
    if args.command == "build":
        for name, data in locale_overlays(source).items():
            (source / name).write_bytes(data)
        shutil.copyfile(DATA / "vite.config.ts", source / "debug.vite.config.ts")
        shutil.copyfile(DATA / "game.env", source / ".env.debug")
        suffix = ".cmd" if os.name == "nt" else ""
        tools = source / "node_modules/.bin"
        for label, command in (
            ("typecheck", ["tsc", "--noEmit"]),
            ("featureTests", ["vitest", "run", "test/system/pokemon-editor.test.ts", "test/tests/system/daily-run", "--maxWorkers=1"]),
            ("viteBuild", ["vite", "build", "--mode", "debug", "--config", "debug.vite.config.ts"]),
        ):
            run([str(tools / (command[0] + suffix)), *command[1:]], source, timeout=600)
            checks[label] = "passed"
    package(source, args.output.resolve(), checks)


if __name__ == "__main__":
    main()
