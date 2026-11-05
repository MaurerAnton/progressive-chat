# SPDX-License-Identifier: AGPL-3.0-only
cmake_minimum_required(VERSION 4.3.4 FATAL_ERROR)

# progressive-chat: Agora's look, drawn on mux's stack.
#
#   pchat.core     the model: accounts, conversations, messages, presence
#   pchat.config   what is kept between runs: settings, looks, accounts
#   pchat.theme    a Material palette made from one seed colour
#   pchat.logic.*  what a screen asks of the model
#   pchat.ui       the screens, as skiff nodes
#   progressive-chat  the program, in an SDL3 window, a browser or an APK
#
# Every dependency comes through cmake-everywhere: the machine's copy where
# it has one with what is needed, and a pinned source where it has not.
include(${CMAKE_CURRENT_LIST_DIR}/cmake/import-std.cmake)
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT COMMAND cme_declare_port AND NOT CMAKE_PROJECT_TOP_LEVEL_INCLUDES)
  set(CMAKE_PROJECT_TOP_LEVEL_INCLUDES ${CMAKE_CURRENT_LIST_DIR}/cmake/get_cme.cmake)
endif()

project(progressive-chat VERSION 0.1 LANGUAGES C CXX)

# What version this is, said as its number and the commit it was made of:
# one number alone says nothing about which build a tester has, and every
# build of 0.1 is a different one. Asked of git here because a build is of a
# checkout; say it where it is known already, with -DPCHAT_REVISION.
if(NOT PCHAT_REVISION)
  # git asked for as a program, and not find_package(Git): cmake-everywhere
  # answers find_package, and answering one before the ports are declared
  # loads its own list of them first -- "the first place that names a port
  # is the place it comes from" -- which made sdl3 come from the registry
  # instead of from this project, and with it a released SDL 3.4.16 with no
  # SDL3/SDL_nativeactivity.h in it at all.
  find_program(GIT_EXECUTABLE NAMES git)
  if(GIT_EXECUTABLE)
    execute_process(COMMAND "${GIT_EXECUTABLE}" rev-parse --short=12 HEAD
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
      OUTPUT_VARIABLE pchat_revision OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_QUIET)
    set(PCHAT_REVISION "${pchat_revision}" CACHE STRING
      "What version this is said to be of, after its number: the commit it was built from" FORCE)
  endif()
endif()
if(PCHAT_REVISION)
  set(PCHAT_VERSION_SAID "${PROJECT_VERSION}-${PCHAT_REVISION}")
else()
  set(PCHAT_VERSION_SAID "${PROJECT_VERSION}")
endif()
add_compile_definitions(PCHAT_VERSION_SAID="${PCHAT_VERSION_SAID}")

# With libstdc++, clang 23.1.4 at least: before it, clang with libstdc++'s
# std module found no operator| for a range adaptor closure another imported
# module had made first, and every pipe had to be written in the call form
# (CODE_STYLE.md). libc++ never had the trouble.
include(CheckCXXSymbolExists)
check_cxx_symbol_exists(__GLIBCXX__ "version" pchat_libstdcxx)
if(pchat_libstdcxx AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 23.1.4)
  message(FATAL_ERROR
    "progressive-chat with libstdc++ needs clang 23.1.4 or later; this is "
    "${CMAKE_CXX_COMPILER_VERSION}")
endif()

if(NOT COMMAND cme_provider)
  message(FATAL_ERROR
    "cmake-everywhere is not installed in this build, so nothing answers "
    "find_package. CMAKE_PROJECT_TOP_LEVEL_INCLUDES is only read by the "
    "top-level project() call: configure this directory, and not a "
    "subdirectory of it.")
endif()

# Keep direct requests distinct from requests made by the libraries themselves.
macro(pchat_find_package pchat_requested_package)
  set_property(GLOBAL APPEND PROPERTY PCHAT_ABOUT_PRIMARY_PACKAGES "${pchat_requested_package}")
  cme_find_package(${pchat_requested_package} ${ARGN})
endmacro()

# Native objects throughout the build: no link-time optimisation.
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE OFF)

set(pchat_desktop_default ON)
if(ANDROID)
  set(pchat_desktop_default OFF)
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
  include(cmake/android.cmake)
endif()
include(CMakeDependentOption)
cmake_dependent_option(PCHAT_TESTS "Build the tests" ${PROJECT_IS_TOP_LEVEL} "NOT ANDROID" OFF)
# The window: SDL3 and skiff, with Skia on GL.
option(PCHAT_UI "Build the window, and the progressive-chat program" ON)

# Full BMIs, in every build: a reduced BMI leaves out the definition of a
# function that is not inline -- also one whose deduced type is a class local
# to it, as every view's is -- and clang 23 then recursed without end looking
# up names over that class in a unit importing it through another module. A
# full BMI keeps the definitions. Kept outside a release build it made a
# release build a different program: one 0.1 built that way went as soon as
# it started. So nothing here is built as a release -- which is also what mux
# asks, asking Debug for all of its own builds and -O3 over it.
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-fno-modules-reduced-bmi>)
endif()

set(PCHAT_CONSTEXPR_STEPS 300000000 CACHE STRING
  "clang's -fconstexpr-steps for progressive-chat and the libraries under it")
# The flags that decide which copy of std a target is compiled against are
# given to what imports a library of modules, not to the whole build: the std
# target itself is built without them, so everything that imports a module is
# compiled against a std synthesized for its flags, one for all of them.
set(pchat_std_flags
  $<$<COMPILE_LANG_AND_ID:CXX,Clang,AppleClang>:-fconstexpr-steps=${PCHAT_CONSTEXPR_STEPS}>
  $<$<AND:$<COMPILE_LANG_AND_ID:CXX,Clang>,$<VERSION_GREATER_EQUAL:$<CXX_COMPILER_VERSION>,23>>:-fno-experimental-new-constant-interpreter>)

# Every library of the family, pinned to a commit. skiff draws the window and
# reads the settings' values; skiff-widgets has the controls a screen is made
# of; skiff-framework has the window itself (SDL3 makes it, Skia draws into
# it). None of them is in cmake-everywhere's registry yet, so each is said
# here as mux says its own.
set(pchat_skiff_pin 4ed6367b24f8d7d67b6b9b3afc62d15732cff5a0)
cme_declare_port(NAME skiff PROVIDES skiff
  GITHUB_REPOSITORY j4niwzis/skiff
  GIT_TAG ${pchat_skiff_pin}
  VERSION 0.1.0
  LICENSE AGPL-3.0-only)
cme_declare_port(NAME skiff-widgets PROVIDES skiff-widgets skiff_widgets
  GITHUB_REPOSITORY j4niwzis/skiff-widgets
  GIT_TAG d1bcbf6288b34fa58e1e8f4cfb12ea582080130a
  VERSION 0.1.0
  LICENSE AGPL-3.0-only)
cme_declare_port(NAME skiff-framework PROVIDES skiff-framework skiff_framework
  GITHUB_REPOSITORY j4niwzis/skiff-framework
  GIT_TAG 3b32853da648774e44087d4343ca42c75283b8cd
  VERSION 0.1.0
  LICENSE AGPL-3.0-only)

set(PCHAT_BROWSER OFF)
if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  set(PCHAT_BROWSER ON)
  set(PCHAT_WINDOW OFF)
endif()

# skiff's components, as progressive-chat draws with them: the system's fonts
# read from their directory, text shaped by HarfBuzz and laid out by
# Unicode's rules, GL where asked for.
option(PCHAT_GL "Draw on the GPU through OpenGL, besides in software" ON)
set(pchat_skiff_components fontmgr-directory text-shaping)
if(PCHAT_GL)
  list(APPEND pchat_skiff_components gl)
endif()
set(pchat_skiff_options)
if(PCHAT_BROWSER)
  list(APPEND pchat_skiff_options "SKIFF_WEB ON" "SKIFF_EMSCRIPTEN_THREADS ON")
endif()
list(APPEND pchat_skiff_options "SKIFF_WEB_DOM memory")
if(NOT PCHAT_UI)
  set(pchat_skiff_components)
  list(APPEND pchat_skiff_options "SKIFF_UI OFF")
endif()
cme_options(skiff ${pchat_skiff_options})
cme_options(skiff-framework "SKIFF_FRAMEWORK_APP OFF" "SKIFF_FRAMEWORK_SITE OFF" "SKIFF_FRAMEWORK_WINDOW ON" "SKIFF_FRAMEWORK_PAGE OFF")

# What is written on the wire and what is kept on the disk: read into values
# of its own once, where it comes in, and written out of them again -- nothing
# past it looks at a brace.
add_library(pchat_json)
target_sources(pchat_json PUBLIC FILE_SET CXX_MODULES BASE_DIRS src
  FILES src/json.cc)
target_compile_features(pchat_json PUBLIC cxx_std_26)
target_compile_options(pchat_json PUBLIC ${pchat_std_flags})

# The model, the settings and the palette: modules of their own, so that they
# build -- and are tested -- without a window.
add_library(pchat_core)
target_sources(pchat_core PUBLIC FILE_SET CXX_MODULES BASE_DIRS src
  FILES src/core.cc src/config.cc src/theme.cc src/theme_palette.cc
        src/logic/lists.cc src/logic/search.cc src/logic/drafts.cc
        src/logic/composer.cc src/logic/formatting.cc)
target_compile_features(pchat_core PUBLIC cxx_std_26)
target_compile_options(pchat_core PUBLIC ${pchat_std_flags})

# What a control asks the program for: said before the screens are, as they
# are built out of what asks it.
add_library(pchat_requests)
target_sources(pchat_requests PUBLIC FILE_SET CXX_MODULES BASE_DIRS src
  FILES src/app/requests.cc)
target_compile_features(pchat_requests PUBLIC cxx_std_26)
target_compile_options(pchat_requests PUBLIC ${pchat_std_flags})
target_link_libraries(pchat_requests PUBLIC pchat_core)

if(PCHAT_UI)
  pchat_find_package(skiff 0.1.0 COMPONENTS ${pchat_skiff_components} REQUIRED)
  pchat_find_package(SDL3 REQUIRED)
  pchat_find_package(skiff-widgets 0.1.0 REQUIRED)
  pchat_find_package(skiff-framework 0.1.0 REQUIRED)

  # What the platform gives the window -- its clipboard, notifications and
  # open dialogs -- in src/platform/, as source overlays. For foo.cc, a
  # matching foo_android.cc, foo_web.cc, foo_freedesktop.cc, foo_windows.cc
  # or foo_macos.cc replaces the common file on that platform; a suffixed file
  # with no common one stands alone, and one may name several platforms. No
  # source says #if about a platform.
  set(PCHAT_PLATFORM_VARIANT_PATTERN "android|web|wasi|freedesktop|windows|macos")
  if(ANDROID)
    set(PCHAT_PLATFORM_SUFFIX android)
  elseif(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    set(PCHAT_PLATFORM_SUFFIX web)
  elseif(CMAKE_SYSTEM_NAME STREQUAL "WASI")
    set(PCHAT_PLATFORM_SUFFIX wasi)
  elseif(UNIX AND NOT APPLE)
    set(PCHAT_PLATFORM_SUFFIX freedesktop)
  elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(PCHAT_PLATFORM_SUFFIX windows)
  elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(PCHAT_PLATFORM_SUFFIX macos)
  else()
    set(PCHAT_PLATFORM_SUFFIX "")
  endif()
  set(pchat_overlay_regex "_(${PCHAT_PLATFORM_VARIANT_PATTERN})(_(${PCHAT_PLATFORM_VARIANT_PATTERN}))*\\.cc$")
  file(GLOB pchat_platform_all CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/platform/*.cc")
  set(pchat_platform_common ${pchat_platform_all})
  list(FILTER pchat_platform_common EXCLUDE REGEX "${pchat_overlay_regex}")
  set(pchat_platform_sources)
  foreach(source IN LISTS pchat_platform_common)
    get_filename_component(source_dir "${source}" DIRECTORY)
    get_filename_component(source_name "${source}" NAME_WE)
    set(overlay "${source_dir}/${source_name}_${PCHAT_PLATFORM_SUFFIX}.cc")
    if(PCHAT_PLATFORM_SUFFIX AND EXISTS "${overlay}")
      list(APPEND pchat_platform_sources "${overlay}")
    else()
      list(APPEND pchat_platform_sources "${source}")
    endif()
  endforeach()
  foreach(source IN LISTS pchat_platform_all)
    get_filename_component(source_name "${source}" NAME)
    if(PCHAT_PLATFORM_SUFFIX AND source_name MATCHES "${pchat_overlay_regex}" AND
       source_name MATCHES "_${PCHAT_PLATFORM_SUFFIX}(_|\\.cc$)")
      list(APPEND pchat_platform_sources "${source}")
    endif()
  endforeach()
  if(PCHAT_PLATFORM_SUFFIX)
    file(GLOB_RECURSE pchat_platform_pieces CONFIGURE_DEPENDS
      "${CMAKE_CURRENT_SOURCE_DIR}/src/platform/${PCHAT_PLATFORM_SUFFIX}/*.cc")
    list(APPEND pchat_platform_sources ${pchat_platform_pieces})
  endif()
  list(REMOVE_DUPLICATES pchat_platform_sources)
  foreach(source IN LISTS pchat_platform_sources)
    get_filename_component(source_name "${source}" NAME)
    if(source_name MATCHES "${pchat_overlay_regex}")
      string(REGEX REPLACE "(_(${PCHAT_PLATFORM_VARIANT_PATTERN}))+\\.cc$" ".cc" common_name "${source_name}")
      list(REMOVE_ITEM pchat_platform_sources "${CMAKE_CURRENT_SOURCE_DIR}/src/platform/${common_name}")
    endif()
  endforeach()

  add_library(pchat_ui)
  target_sources(pchat_ui PUBLIC FILE_SET CXX_MODULES BASE_DIRS src
    FILES ${pchat_platform_sources} src/ui.cc
          src/ui/base.cc src/ui/theme.cc src/ui/icons.cc src/ui/controls.cc
          src/ui/drawer.cc src/ui/top_bar.cc src/ui/message_list.cc
          src/ui/composer.cc src/ui/settings.cc src/ui/appearance.cc
          src/ui/window.cc)
  target_compile_features(pchat_ui PUBLIC cxx_std_26)
  target_compile_options(pchat_ui PUBLIC ${pchat_std_flags})
  target_link_libraries(pchat_ui PUBLIC pchat_core pchat_requests SDL3::SDL3 skiff::skiff skiff::widgets skiff::window)

  # The program's own part: what fills the screens from the model, and what a
  # control asks for.
  add_library(pchat_app OBJECT)
  target_sources(pchat_app PUBLIC FILE_SET CXX_MODULES BASE_DIRS src
    FILES src/app/screens.cc src/app/sample.cc src/app/faces.cc)
  target_compile_features(pchat_app PUBLIC cxx_std_26)
  target_compile_options(pchat_app PUBLIC ${pchat_std_flags})
  target_link_libraries(pchat_app PUBLIC pchat_ui pchat_requests)

  # The program itself.
  if(PCHAT_BROWSER)
    add_executable(progressive-chat src/main.cc src/main_web.cc src/browser_host.cc)
    target_link_libraries(progressive-chat PRIVATE skiff::browser)
    include(cmake/browser.cmake)
  elseif(ANDROID)
    add_library(progressive-chat SHARED src/main.cc src/main_android.cc src/window_host.cc)
    target_link_options(progressive-chat PRIVATE ${PCHAT_ANDROID_LINK_OPTIONS})
    target_link_libraries(progressive-chat PRIVATE log)
    # The same library, packaged as what a phone installs.
    include(cmake/android-apk.cmake)
    pchat_add_android_apk(progressive-chat)
  else()
    add_executable(progressive-chat src/main.cc src/main_desktop.cc src/window_host.cc)
  endif()
  target_sources(progressive-chat PRIVATE FILE_SET CXX_MODULES BASE_DIRS src
    FILES src/app/program.cc)
  target_link_libraries(progressive-chat PRIVATE pchat_app pchat_ui pchat_core)
endif()

if(PCHAT_TESTS)
  enable_testing()
  add_subdirectory(test)
endif()

# One std module in the whole build. Where two copies of it -- or of any
# module of the family -- meet in one unit, clang 23 mixes their declarations
# up and crashes. So every library of modules in the build gets the same
# standard and flags as progressive-chat's own: one set of flags, one std,
# each module built once.
function(pchat_targets_under dir out)
  get_property(found DIRECTORY ${dir} PROPERTY BUILDSYSTEM_TARGETS)
  get_property(subdirectories DIRECTORY ${dir} PROPERTY SUBDIRECTORIES)
  foreach(subdirectory IN LISTS subdirectories)
    pchat_targets_under(${subdirectory} more)
    list(APPEND found ${more})
  endforeach()
  set(${out} ${found} PARENT_SCOPE)
endfunction()
pchat_targets_under(${CMAKE_CURRENT_SOURCE_DIR} pchat_all_targets)
foreach(target IN LISTS pchat_all_targets)
  get_target_property(module_sets ${target} CXX_MODULE_SETS)
  get_target_property(imported ${target} IMPORTED)
  get_target_property(kind ${target} TYPE)
  if(NOT imported AND kind MATCHES "^(EXECUTABLE|STATIC_LIBRARY|SHARED_LIBRARY|MODULE_LIBRARY|OBJECT_LIBRARY)$")
    set_target_properties(${target} PROPERTIES
      INTERPROCEDURAL_OPTIMIZATION OFF INTERPROCEDURAL_OPTIMIZATION_RELEASE OFF)
  endif()
  if(NOT imported AND (module_sets OR
      (target MATCHES "^pchat" AND kind MATCHES "^(EXECUTABLE|STATIC_LIBRARY|SHARED_LIBRARY|MODULE_LIBRARY|OBJECT_LIBRARY)$")))
    set_target_properties(${target} PROPERTIES
      CXX_STANDARD 26 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
  endif()
  if(module_sets AND NOT imported AND NOT target MATCHES "^pchat")
    target_compile_options(${target} PUBLIC ${pchat_std_flags})
  endif()
endforeach()
