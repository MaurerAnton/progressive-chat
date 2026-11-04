# SPDX-License-Identifier: AGPL-3.0-only
# The Android build: the program as a shared library, loaded by its own
# activity. What differs from a desktop build is said here, and nowhere else
# in the tree.

# NativeActivity runs on a small pthread stack: catch large startup locals
# before packaging something that crashes at function entry.
set_property(SOURCE src/main.cc APPEND PROPERTY COMPILE_OPTIONS
  -Wframe-larger-than=65536 -Werror=frame-larger-than)

# The entry a NativeActivity needs: Android starts the activity itself, and
# SDL3 bridges it to the program. Upstream has released no SDL3 that carries
# that bridge, so this build asks for the fork that does, and is given
# Android's own NDK sources for the glue the activity thread is made of.
set(PCHAT_ANDROID_NATIVE_APP_GLUE
  "${MANDK_ROOT}/src/ndk/sources/android/native_app_glue"
  CACHE PATH "AOSP native_app_glue source directory")
cme_declare_port(NAME sdl3 PROVIDES SDL3 sdl3 VERSION 3.5.0
  GITHUB_REPOSITORY j4niwzis/SDL
  GIT_TAG 91c08de87b43fdecc824464fcce15499ccbc7101
  LICENSE Zlib
  TARGETS SDL3::SDL3)
cme_options(sdl3 "SDL_ANDROID_NATIVE_ACTIVITY ON" "SDL_ANDROID_JAR OFF"
  "SDL_SHARED OFF" "SDL_STATIC ON" "SDL_INSTALL OFF" "SDL_VULKAN OFF"
  "SDL_ANDROID_NATIVE_APP_GLUE ${PCHAT_ANDROID_NATIVE_APP_GLUE}")
# A system SDL3 does not have that entry point in it.
set(CME_SYSTEM_SDL3 OFF)

# Nothing here opens a camera, and the NDK made from Android's own sources
# carries no headers for one: SDL3's Android camera driver is not built.
set(SDL_CAMERA OFF CACHE BOOL "" FORCE)

# The linker options the platform needs: no undefined symbol may stay
# unresolved in a library loaded by ART, and every part of the C++ runtime
# the NDK's build does not add by itself is named here.
set(PCHAT_ANDROID_LINK_OPTIONS
  -Wl,--no-undefined
  -Wl,-z,max-page-size=16384)
