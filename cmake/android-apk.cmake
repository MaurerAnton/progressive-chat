# SPDX-License-Identifier: AGPL-3.0-only
# An APK, out of the library: what a library loaded by a NativeActivity is
# not yet, and what a phone installs.
#
# The parts of it that no compiler here makes -- the platform's own resource
# package every application's resources are linked against, and the signer,
# which is an AOSP project rather than a package -- come from the toolchain
# the NDK was made with (tools/ of its root). The part that would otherwise
# be Java is not: the fork of SDL this build asks for carries a script that
# writes a dex out directly, which is built for this machine and run here.

function(pchat_add_android_apk target)
  set(PCHAT_ANDROID_FRAMEWORK_RES_APK "${MANDK_ROOT}/tools/framework-res.apk" CACHE FILEPATH
      "AOSP framework resource APK")
  set(PCHAT_ANDROID_APKSIGNER_JAR "${MANDK_ROOT}/tools/apksigner.jar" CACHE FILEPATH
      "Source-built apksigner JAR")
  set(PCHAT_ANDROID_TARGET_API 34 CACHE STRING "APK target API")
  set(PCHAT_ANDROID_MIN_API 28 CACHE STRING "APK minimum API")
  set(PCHAT_ANDROID_VERSION_CODE 1 CACHE STRING "Monotonic APK version code")
  set(PCHAT_ANDROID_SDL_DEX_DIR "" CACHE PATH
      "Directory of the SDL fork's native-activity/dex script")
  option(PCHAT_ANDROID_TEST_KEY "Sign the APK with a generated test key" ON)
  set(PCHAT_ANDROID_LIBRARY_DIRS "" CACHE STRING "Additional Android runtime library search directories")
  if(PCHAT_ANDROID_MIN_API LESS 28 OR PCHAT_ANDROID_TARGET_API LESS PCHAT_ANDROID_MIN_API)
    message(FATAL_ERROR "Android packaging requires 28 <= PCHAT_ANDROID_MIN_API <= PCHAT_ANDROID_TARGET_API")
  endif()
  # Android keeps the signature outside the archive from 7.0 (API 24): an
  # APK signed in that scheme alone installs on everything this one runs on,
  # so the older signature inside the archive is not made.
  set(v1_signing false)
  foreach(input PCHAT_ANDROID_FRAMEWORK_RES_APK PCHAT_ANDROID_APKSIGNER_JAR)
    if(NOT EXISTS "${${input}}")
      message(FATAL_ERROR "Set ${input} to an existing file (currently '${${input}}')")
    endif()
  endforeach()
  # These run on the machine that builds, not on the one the APK runs on.
  # readelf and strip are of the same llvm as the compiler, which is not on
  # PATH as a whole -- only what this build asks of clang is.
  get_filename_component(llvm_bin "${CMAKE_CXX_COMPILER}" DIRECTORY)
  foreach(program IN ITEMS "PCHAT_AAPT2;aapt2" "PCHAT_ZIPALIGN;zipalign" "PCHAT_JAVA;java"
                           "PCHAT_KEYTOOL;keytool" "PCHAT_READELF;llvm-readelf;readelf"
                           "PCHAT_STRIP;llvm-strip;strip")
    list(POP_FRONT program variable names)
    find_program(${variable} NAMES ${names} HINTS "${llvm_bin}" REQUIRED
      NO_CMAKE_FIND_ROOT_PATH)
  endforeach()
  find_package(Python3 COMPONENTS Interpreter REQUIRED)

  # The fork's own script that writes the activity's classes out. Set
  # PCHAT_ANDROID_SDL_DEX_DIR where its checkout is known already; where it
  # is not, ask the library SDL was built from where its sources are.
  if(PCHAT_ANDROID_SDL_DEX_DIR)
    set(dex_source "${PCHAT_ANDROID_SDL_DEX_DIR}")
  else()
    get_target_property(sdl_source SDL3::SDL3 SOURCE_DIR)
    if(sdl_source)
      set(dex_source "${sdl_source}/build-scripts/native-activity/dex")
    else()
      set(dex_source "")
    endif()
  endif()
  if(NOT EXISTS "${dex_source}/CMakeLists.txt")
    message(FATAL_ERROR
      "Android packaging needs the SDL fork's build-scripts/native-activity/dex "
      "(set PCHAT_ANDROID_SDL_DEX_DIR to its checkout; here: '${dex_source}')")
  endif()
  # Built for this machine -- it is what writes a dex, and neither dex nor
  # the script that makes one is anything the NDK's own toolchain builds.
  cme_declare_port(NAME sdl-native-dex PROVIDES sdl-native-dex VERSION 1.0
    SOURCE_DIR "${dex_source}" MACHINE build
    PROGRAMS "sdl-native-dex=sdl::native-dex" TARGETS sdl::native-dex)
  find_package(sdl-native-dex REQUIRED)

  set(out "${CMAKE_BINARY_DIR}/apk")
  file(MAKE_DIRECTORY "${out}")
  set(manifest "${PROJECT_SOURCE_DIR}/android/AndroidManifest.xml")
  set(packager "${PROJECT_SOURCE_DIR}/tools/android_apk.py")
  # Libraries built here can be anywhere below the build tree. SDL is linked
  # into the application statically, and so is not one of them.
  set(search --library-dir "${CMAKE_BINARY_DIR}")
  foreach(dir IN LISTS CMAKE_PREFIX_PATH PCHAT_ANDROID_LIBRARY_DIRS)
    list(APPEND search --library-dir "${dir}")
  endforeach()
  set(abi "${ANDROID_ABI}")
  set(name "progressive-chat-${abi}")

  # The classes: NativeActivity itself, taken for what its callbacks are.
  add_custom_command(OUTPUT "${out}/classes.dex"
    COMMAND "${sdl-native-dex_PROGRAM}" --out "${out}/classes.dex"
      --library "$<TARGET_FILE_BASE_NAME:${target}>"
    DEPENDS "${sdl-native-dex_PROGRAM}" VERBATIM)
  # The resources: this manifest, against the platform's own.
  add_custom_command(OUTPUT "${out}/progressive-chat-resources.apk"
    COMMAND "${PCHAT_AAPT2}" link -o "${out}/progressive-chat-resources.apk"
      --manifest "${manifest}" -I "${PCHAT_ANDROID_FRAMEWORK_RES_APK}"
      --min-sdk-version "${PCHAT_ANDROID_MIN_API}" --target-sdk-version "${PCHAT_ANDROID_TARGET_API}"
      --version-code "${PCHAT_ANDROID_VERSION_CODE}" --version-name "${PROJECT_VERSION}"
    DEPENDS "${manifest}" "${PCHAT_ANDROID_FRAMEWORK_RES_APK}" VERBATIM)
  # The archive, of those two and every library they load with.
  add_custom_command(OUTPUT "${out}/progressive-chat-unaligned.apk"
    COMMAND "${Python3_EXECUTABLE}" "${packager}"
      --resources "${out}/progressive-chat-resources.apk" --dex "${out}/classes.dex"
      --library "$<TARGET_FILE:${target}>" --strip-tool "${PCHAT_STRIP}"
      ${search} --readelf "${PCHAT_READELF}" --abi "${abi}"
      --out "${out}/progressive-chat-unaligned.apk"
    COMMAND "${PCHAT_ZIPALIGN}" -f 4 "${out}/progressive-chat-unaligned.apk"
      "${out}/progressive-chat-aligned.apk"
    COMMAND "${PCHAT_ZIPALIGN}" -c 4 "${out}/progressive-chat-aligned.apk"
    DEPENDS ${target} "${out}/classes.dex" "${out}/progressive-chat-resources.apk" "${packager}"
      "${PROJECT_SOURCE_DIR}/android/abis.json"
    VERBATIM COMMAND_EXPAND_LISTS)
  # A key of this build's own, so that the APK has one: signed is what an
  # Android installs, and a key made here is as good as any other key for
  # that, which is all it is for -- it is not who published the program.
  if(PCHAT_ANDROID_TEST_KEY)
    add_custom_command(OUTPUT "${out}/progressive-chat-test.p12"
      COMMAND "${PCHAT_KEYTOOL}" -genkeypair -keystore "${out}/progressive-chat-test.p12"
        -storetype PKCS12 -alias androiddebugkey -storepass android -keypass android
        -keyalg RSA -keysize 2048 -validity 10950
        -dname "CN=progressive-chat test key, OU=build, O=progressive-chat, C=XX"
      VERBATIM)
    add_custom_command(OUTPUT "${out}/progressive-chat-signed.apk"
      COMMAND "${PCHAT_JAVA}" -jar "${PCHAT_ANDROID_APKSIGNER_JAR}" sign
        --ks "${out}/progressive-chat-test.p12" --ks-type PKCS12 --ks-key-alias androiddebugkey
        --ks-pass pass:android --key-pass pass:android
        --min-sdk-version "${PCHAT_ANDROID_MIN_API}"
        --v1-signing-enabled ${v1_signing}
        --v2-signing-enabled true --v3-signing-enabled true
        --out "${out}/progressive-chat-signed.apk" "${out}/progressive-chat-aligned.apk"
      COMMAND "${PCHAT_JAVA}" -jar "${PCHAT_ANDROID_APKSIGNER_JAR}" verify
        --min-sdk-version "${PCHAT_ANDROID_MIN_API}" "${out}/progressive-chat-signed.apk"
      DEPENDS "${out}/progressive-chat-unaligned.apk" "${out}/progressive-chat-test.p12"
        "${PCHAT_ANDROID_APKSIGNER_JAR}" VERBATIM)
    add_custom_target(progressive-chat-apk DEPENDS "${out}/progressive-chat-signed.apk")
    add_custom_command(OUTPUT "${CMAKE_BINARY_DIR}/${name}.apk"
      COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${out}/progressive-chat-signed.apk"
        "${CMAKE_BINARY_DIR}/${name}.apk"
      DEPENDS "${out}/progressive-chat-signed.apk" VERBATIM)
  endif()
  # Whatever the key, the unsigned one is kept too: it is the APK before
  # the only step that is anybody's name on it.
  add_custom_target(progressive-chat-apk-unsigned DEPENDS "${out}/progressive-chat-aligned.apk")
  add_custom_command(OUTPUT "${CMAKE_BINARY_DIR}/${name}-unsigned.apk"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${out}/progressive-chat-aligned.apk"
      "${CMAKE_BINARY_DIR}/${name}-unsigned.apk"
    DEPENDS "${out}/progressive-chat-aligned.apk" VERBATIM)
  add_custom_target(progressive-chat-apk-out DEPENDS "${CMAKE_BINARY_DIR}/${name}.apk"
    "${CMAKE_BINARY_DIR}/${name}-unsigned.apk")
endfunction()
