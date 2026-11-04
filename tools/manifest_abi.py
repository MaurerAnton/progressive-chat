# SPDX-License-Identifier: AGPL-3.0-only
# Say the manifest of the tool that makes an Android toolchain for the ABI a
# run is building: its own copy names 64-bit ARM, and only that, and a 32-bit
# ARM toolchain is the same sources said another way.

import argparse
import re
import sys

# What 32-bit ARM is called, and what Clang has to be told about it:
# arm-linux-androideabi is ARMv5 to Clang, so the compiler is given the
# armv7a spelling while the sysroot's own directories keep the shorter one,
# and floating point is neon with the soft-float ABI the NDK itself uses --
# Clang 23 assumes neither, and everything built for the target would
# otherwise emulate its arithmetic.
ARM_32 = """{
    "triple": "armv7a-linux-androideabi",
    "sysroot-triple": "arm-linux-androideabi",
    "arch": "arm",
    "abi": "armeabi-v7a",
    "api": 27,
    "flags": ["-mfpu=neon", "-mfloat-abi=softfp", "-mthumb"]
  }"""


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--manifest", required=True)
    ap.add_argument("--abi", required=True)
    ap.add_argument("--out", required=True)
    args = ap.parse_args()

    text = open(args.manifest).read()
    if args.abi == "armeabi-v7a":
        text, said = re.subn(r'"target":\s*\{.*?\}', '"target": ' + ARM_32, text, count=1, flags=re.S)
        if said != 1:
            print("no target block in the manifest", file=sys.stderr)
            return 1
    open(args.out, "w").write(text)
    print(f"manifest for {args.abi} written to {args.out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
