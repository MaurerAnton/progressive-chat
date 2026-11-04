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
  # The icon the launcher shows, in every form a phone may ask for it in: an
  # adaptive one -- its plate and its figure apart -- since API 26, and a
  # plate of each size for what is older than that.
  set(PCHAT_ANDROID_RESOURCES
    drawable/ic_launcher_background.xml
    drawable/ic_launcher_foreground.xml
    mipmap-anydpi-v26/ic_launcher.xml
    mipmap-anydpi-v26/ic_launcher_round.xml
    mipmap-mdpi/ic_launcher.png
    mipmap-hdpi/ic_launcher.png
    mipmap-xhdpi/ic_launcher.png
    mipmap-xxhdpi/ic_launcher.png
    mipmap-xxxhdpi/ic_launcher.png
    values/colors.xml
    CACHE STRING "The files of android/res compiled and linked into the APK")
  set(PCHAT_ANDROID_FRAMEWORK_RES_APK "${MANDK_ROOT}/tools/framework-res.apk" CACHE FILEPATH
      "AOSP framework resource APK")
  set(PCHAT_ANDROID_APKSIGNER_JAR "${MANDK_ROOT}/tools/apksigner.jar" CACHE FILEPATH
      "Source-built apksigner JAR")
  # What an Android has to be asked for to read this APK: 21, and not the
  # platform it is built against (28) -- nothing later than 21 is asked
  # for, which its own libraries say (there is no symbol of a later
  # release left undefined in them), and 21 asks for a phone of 2014.
  # It was 5 once, and 5 is what an Android of this decade answers
  # with "there was a problem while parsing the package": too old a
  # release to be asked for at all.
  set(PCHAT_ANDROID_TARGET_API 35 CACHE STRING "APK target API")
  set(PCHAT_ANDROID_MIN_API 21 CACHE STRING "APK minimum API")
  set(PCHAT_ANDROID_VERSION_CODE 1 CACHE STRING "Monotonic APK version code")
  set(PCHAT_ANDROID_REVISION "" CACHE STRING
      "What an APK made here is named after: its commit, where one is known")
  set(PCHAT_ANDROID_SDL_DEX_DIR "" CACHE PATH
      "Directory of the SDL fork's native-activity/dex script")
  option(PCHAT_ANDROID_TEST_KEY "Sign the APK with a generated test key" ON)
  set(PCHAT_ANDROID_LIBRARY_DIRS "" CACHE STRING "Additional Android runtime library search directories")
  if(PCHAT_ANDROID_MIN_API LESS 21 OR PCHAT_ANDROID_TARGET_API LESS PCHAT_ANDROID_MIN_API)
    message(FATAL_ERROR "Android packaging requires 21 <= PCHAT_ANDROID_MIN_API <= PCHAT_ANDROID_TARGET_API")
  endif()
  # Android keeps the signature outside the archive from 7.0 (API 24): only
  # below that is the signature inside the archive the one that is read, and
  # so only there is it made.
  if(PCHAT_ANDROID_MIN_API LESS 24)
    set(v1_signing true)
  else()
    set(v1_signing false)
  endif()
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
                           "PCHAT_KEYTOOL;keytool"
                           "PCHAT_READELF;llvm-readelf;llvm-readelf-23;readelf"
                           "PCHAT_STRIP;llvm-strip;llvm-strip-23;strip")
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
  # What the dex is written by is a program of this machine's own, and of
  # modules too -- that asks for clang 23 and its libc++, and not for the
  # default compiler, which is a gcc with no `import std` to give.
  # cmake-everywhere asks for it by name, and keeps it in its own cache
  # entry -- which a find_program of the same name may not overwrite.
  if(CME_BUILD_MACHINE_CXX_COMPILER STREQUAL "")
    find_program(pchat_host_cxx NAMES clang++-23 clang++ HINTS "${llvm_bin}")
    if(pchat_host_cxx)
      get_filename_component(pchat_host_cxx "${pchat_host_cxx}" NAME)
    else()
      set(pchat_host_cxx "clang++-23")
    endif()
    set(CME_BUILD_MACHINE_CXX_COMPILER "${pchat_host_cxx}" CACHE STRING
      "C++ compiler for programs this build runs rather than ships" FORCE)
  endif()
  if(CME_BUILD_MACHINE_CXX_FLAGS STREQUAL "")
    set(CME_BUILD_MACHINE_CXX_FLAGS "-stdlib=libc++" CACHE STRING
      "Flags for programs this build runs rather than ships" FORCE)
  endif()
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
  # Which commit of the program it is, said in the file's own name: an APK
  # is taken out of any build tree and out of any archive of one, and out
  # there nothing else says which build it came from. Asked of git here
  # because this build is of a checkout; set it where it is known already.
  if(NOT PCHAT_ANDROID_REVISION)
    find_package(Git QUIET)
    if(Git_FOUND)
      execute_process(COMMAND "${GIT_EXECUTABLE}" rev-parse --short=12 HEAD
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
        OUTPUT_VARIABLE pchat_revision OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET)
      if(pchat_revision)
        set(PCHAT_ANDROID_REVISION "${pchat_revision}" CACHE STRING
          "What an APK made here is named after: its commit, where one is known" FORCE)
      endif()
    endif()
  endif()
  if(PCHAT_ANDROID_REVISION)
    set(name "progressive-chat-${abi}-${PCHAT_ANDROID_REVISION}")
  else()
    set(name "progressive-chat-${abi}")
  endif()

  # The classes: NativeActivity itself, taken for what its callbacks are.
  add_custom_command(OUTPUT "${out}/classes.dex"
    COMMAND "${sdl-native-dex_PROGRAM}" --out "${out}/classes.dex"
      --library "$<TARGET_FILE_BASE_NAME:${target}>"
    DEPENDS "${sdl-native-dex_PROGRAM}" VERBATIM)
  # The resources: this manifest, against the platform's own, and the tree of
  # icons beside it -- each compiled to its own flat file first, which is how
  # a link is given resources that are not yet in a table of them.
  set(res "${PROJECT_SOURCE_DIR}/android/res")
  set(res_outs)
  set(res_inputs)
  foreach(one IN LISTS PCHAT_ANDROID_RESOURCES)
    if(NOT EXISTS "${res}/${one}")
      message(FATAL_ERROR "android/res/${one} is named for the icon, but is not there")
    endif()
    list(APPEND res_inputs "${res}/${one}")
    # Named as aapt2 names what it compiles: one file per resource, its
    # directory and its file said apart by an underscore, and a values
    # directory's table not a drawing but an arsc.
    get_filename_component(res_dir "${one}" DIRECTORY)
    get_filename_component(res_file "${one}" NAME)
    if(res_dir STREQUAL "values")
      get_filename_component(res_stem "${res_file}" NAME_WE)
      list(APPEND res_outs "${out}/${res_dir}_${res_stem}.arsc.flat")
    else()
      list(APPEND res_outs "${out}/${res_dir}_${res_file}.flat")
    endif()
  endforeach()
  add_custom_command(OUTPUT ${res_outs}
    COMMAND "${PCHAT_AAPT2}" compile --dir "${res}" -o "${out}"
    DEPENDS ${res_inputs} "${PCHAT_AAPT2}" VERBATIM)
  set(res_flat_args)
  foreach(flat IN LISTS res_outs)
    list(APPEND res_flat_args -R "${flat}")
  endforeach()
  add_custom_command(OUTPUT "${out}/progressive-chat-resources.apk"
    COMMAND "${PCHAT_AAPT2}" link -o "${out}/progressive-chat-resources.apk"
      --manifest "${manifest}" -I "${PCHAT_ANDROID_FRAMEWORK_RES_APK}"
      ${res_flat_args} --auto-add-overlay
      --min-sdk-version "${PCHAT_ANDROID_MIN_API}" --target-sdk-version "${PCHAT_ANDROID_TARGET_API}"
      --version-code "${PCHAT_ANDROID_VERSION_CODE}" --version-name "${PROJECT_VERSION}"
    DEPENDS "${manifest}" "${PCHAT_ANDROID_FRAMEWORK_RES_APK}" ${res_outs} VERBATIM)
  # The archive, of those two and every library they load with.
  # Two archives come of one command: the packager writes the unaligned one,
  # and zipalign writes the aligned one out of it -- named here as well, or
  # ninja is asked for a file no rule says it can make.
  add_custom_command(OUTPUT "${out}/progressive-chat-unaligned.apk"
    "${out}/progressive-chat-aligned.apk"
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
