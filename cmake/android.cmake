# SPDX-License-Identifier: AGPL-3.0-only
# The Android build: the program as a shared library, loaded by its own
# activity. What differs from a desktop build is said here, and nowhere else
# in the tree.

# NativeActivity runs on a small pthread stack: catch large startup locals
# before packaging something that crashes at function entry.
set_property(SOURCE src/main.cc APPEND PROPERTY COMPILE_OPTIONS
  -Wframe-larger-than=65536 -Werror=frame-larger-than)

# The linker options the platform needs: no undefined symbol may stay
# unresolved in a library loaded by ART, and every part of the C++ runtime
# the NDK's build does not add by itself is named here.
# Nothing here opens a camera, and the NDK made from Android's own sources
# carries no headers for one: SDL3's Android camera driver is not built.
set(SDL_CAMERA OFF CACHE BOOL "" FORCE)

# The linker options the platform needs:
  -Wl,--no-undefined
  -Wl,-z,max-page-size=16384)
