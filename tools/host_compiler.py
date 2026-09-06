"""Select the optional Windows LLVM-MinGW bundle; otherwise use GCC on PATH."""
from pathlib import Path
import os

Import("env")

root = Path(env.subst("$PROJECT_DIR"))
compiler_bin = root / ".pio" / "host-tools" / "llvm-mingw-20250709-ucrt-x86_64" / "bin"
if os.name == "nt" and (compiler_bin / "clang.exe").is_file():
    # The native platform rediscovers GCC after pre-scripts, then clones build
    # environments before post-scripts. Supply wrappers on PATH for discovery.
    wrappers = root / ".pio" / "host-tools" / "bin"
    wrappers.mkdir(exist_ok=True)
    for name, command in {"gcc": "clang.exe", "g++": "clang++.exe",
                          "ar": "llvm-ar.exe", "ranlib": "llvm-ranlib.exe"}.items():
        (wrappers / (name + ".cmd")).write_text(f'@"{compiler_bin / command}" %*\n')
    env.PrependENVPath("PATH", str(compiler_bin))
    env.PrependENVPath("PATH", str(wrappers))
    # The test runner launches the executable outside SCons' compiler PATH.
    env.Append(LINKFLAGS=["-static"])
