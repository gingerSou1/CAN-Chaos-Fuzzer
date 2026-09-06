# SPDX-License-Identifier: MIT
"""Select the pinned local Windows host compiler without downloading or shell wrappers.

Loaded by PlatformIO/SCons. The compiler directory must contain trusted tools;
otherwise native compiler discovery uses the caller's PATH.
"""
import os
from pathlib import Path

Import("env")

root = Path(env.subst("$PROJECT_DIR"))
compiler_bin = root / ".pio" / "host-tools" / "llvm-mingw-20250709-ucrt-x86_64" / "bin"
if os.name == "nt" and (compiler_bin / "clang.exe").is_file():
    # The bundle supplies gcc.exe/g++.exe aliases for native platform discovery.
    env.PrependENVPath("PATH", str(compiler_bin))
    # The test runner launches the executable outside SCons' compiler PATH.
    env.Append(LINKFLAGS=["-static"])
